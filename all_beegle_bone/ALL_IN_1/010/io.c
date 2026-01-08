#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/mutex.h>
#include <linux/moduleparam.h>

#define MSG_LEN 100
#define MAX_MSGS 10

static char device_name[20] = "enhchardev";
static int major;
static int msg_count = 0;
static char msg_buffer[MAX_MSGS][MSG_LEN];
static int read_index = 0, write_index = 0;
static DEFINE_MUTEX(buffer_lock);

// module parameters
module_param_string(device_name, device_name, sizeof(device_name), 0660);
MODULE_PARM_DESC(device_name, "Name of the device");

static int dev_open(struct inode *inode, struct file *file)
{
    printk(KERN_INFO "[%s] Device opened\n", device_name);
    return 0;
}

static int dev_release(struct inode *inode, struct file *file)
{
    printk(KERN_INFO "[%s] Device closed\n", device_name);
    return 0;
}

static ssize_t dev_write(struct file *file, const char __user *buf, size_t len, loff_t *offset)
{
    if (len > MSG_LEN - 1)
        len = MSG_LEN - 1;

    mutex_lock(&buffer_lock);

    if (msg_count >= MAX_MSGS) {
        printk(KERN_WARNING "[%s] Buffer full, dropping message\n", device_name);
        mutex_unlock(&buffer_lock);
        return -ENOMEM;
    }

    if (copy_from_user(msg_buffer[write_index], buf, len)) {
        mutex_unlock(&buffer_lock);
        return -EFAULT;
    }

    msg_buffer[write_index][len] = '\0';
    write_index = (write_index + 1) % MAX_MSGS;
    msg_count++;

    mutex_unlock(&buffer_lock);
    return len;
}

static ssize_t dev_read(struct file *file, char __user *buf, size_t len, loff_t *offset)
{
    ssize_t ret;

    mutex_lock(&buffer_lock);

    if (msg_count == 0) {
        mutex_unlock(&buffer_lock);
        return 0; // nothing to read
    }

    ret = simple_read_from_buffer(buf, len, offset, msg_buffer[read_index], strlen(msg_buffer[read_index]));
    read_index = (read_index + 1) % MAX_MSGS;
    msg_count--;

    mutex_unlock(&buffer_lock);
    return ret;
}

static struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = dev_open,
    .read = dev_read,
    .write = dev_write,
    .release = dev_release,
};

static int __init char_enhanced_init(void)
{
    major = register_chrdev(0, device_name, &fops);
    if (major < 0) {
        printk(KERN_ALERT "Failed to register device\n");
        return major;
    }

    mutex_init(&buffer_lock);
    printk(KERN_INFO "[%s] registered with major %d\n", device_name, major);
    return 0;
}

static void __exit char_enhanced_exit(void)
{
    unregister_chrdev(major, device_name);
    printk(KERN_INFO "[%s] unregistered\n", device_name);
}

module_init(char_enhanced_init);
module_exit(char_enhanced_exit);


 
MODULE_LICENSE("GPL");
