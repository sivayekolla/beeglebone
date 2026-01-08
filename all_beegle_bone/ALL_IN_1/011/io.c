#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/mutex.h>
#include <linux/ioctl.h>

#define DEVICE_NAME "ioctldev"
#define BUFFER_SIZE 256
#define MAJOR_NUM 240

#define IOCTL_RESET_BUFFER _IO(MAJOR_NUM, 0)
#define IOCTL_GET_COUNT    _IOR(MAJOR_NUM, 1, int)

static char buffer[BUFFER_SIZE];
static int write_count = 0;
static DEFINE_MUTEX(buffer_lock);

static int dev_open(struct inode *inode, struct file *file)
{
    printk(KERN_INFO "[%s] Device opened\n", DEVICE_NAME);
    return 0;
}

static int dev_release(struct inode *inode, struct file *file)
{
    printk(KERN_INFO "[%s] Device closed\n", DEVICE_NAME);
    return 0;
}

static ssize_t dev_write(struct file *file, const char __user *buf, size_t len, loff_t *off)
{
    if (len > BUFFER_SIZE - 1)
        len = BUFFER_SIZE - 1;

    mutex_lock(&buffer_lock);

    if (copy_from_user(buffer, buf, len)) {
        mutex_unlock(&buffer_lock);
        return -EFAULT;
    }

    buffer[len] = '\0';
    write_count++;

    printk(KERN_INFO "[%s] Received: %s\n", DEVICE_NAME, buffer);

    mutex_unlock(&buffer_lock);
    return len;
}

static ssize_t dev_read(struct file *file, char __user *buf, size_t len, loff_t *off)
{
    return simple_read_from_buffer(buf, len, off, buffer, strlen(buffer));
}

static long dev_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    switch (cmd) {
        case IOCTL_RESET_BUFFER:
            mutex_lock(&buffer_lock);
            memset(buffer, 0, BUFFER_SIZE);
            write_count = 0;
            mutex_unlock(&buffer_lock);
            printk(KERN_INFO "[%s] Buffer reset\n", DEVICE_NAME);
            break;

        case IOCTL_GET_COUNT:
            if (copy_to_user((int *)arg, &write_count, sizeof(write_count)))
                return -EFAULT;
            printk(KERN_INFO "[%s] Sent write_count = %d\n", DEVICE_NAME, write_count);
            break;

        default:
            return -EINVAL;
    }
    return 0;
}

static struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = dev_open,
    .read = dev_read,
    .write = dev_write,
    .unlocked_ioctl = dev_ioctl,
    .release = dev_release,
};

static int __init ioctl_driver_init(void)
{
    int ret = register_chrdev(MAJOR_NUM, DEVICE_NAME, &fops);
    if (ret < 0) {
        printk(KERN_ALERT "Failed to register device\n");
        return ret;
    }

    mutex_init(&buffer_lock);
    printk(KERN_INFO "[%s] registered with major %d\n", DEVICE_NAME, MAJOR_NUM);
    return 0;
}

static void __exit ioctl_driver_exit(void)
{
    unregister_chrdev(MAJOR_NUM, DEVICE_NAME);
    printk(KERN_INFO "[%s] unregistered\n", DEVICE_NAME);
}

module_init(ioctl_driver_init);
module_exit(ioctl_driver_exit);


 
MODULE_LICENSE("GPL");
