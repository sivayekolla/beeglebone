#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/wait.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/mutex.h>

#define DEVICE_NAME "waitqdev"
#define CLASS_NAME "waitclass"
#define BUF_LEN 256

static dev_t dev_num;
static struct class *waitq_class;
static struct device *waitq_device;
static struct cdev waitq_cdev;

static char buffer[BUF_LEN];
static int data_available = 0;
static DECLARE_WAIT_QUEUE_HEAD(wq);
static DEFINE_MUTEX(lock);

static int dev_open(struct inode *inode, struct file *file)
{
    printk(KERN_INFO "[%s] opened\n", DEVICE_NAME);
    return 0;
}

static int dev_release(struct inode *inode, struct file *file)
{
    printk(KERN_INFO "[%s] closed\n", DEVICE_NAME);
    return 0;
}

static ssize_t dev_read(struct file *file, char __user *buf, size_t len, loff_t *off)
{
    int ret;

    if (file->f_flags & O_NONBLOCK) {
        if (!data_available)
            return -EAGAIN;
    } else {
        wait_event_interruptible(wq, data_available);
    }

    mutex_lock(&lock);
    ret = simple_read_from_buffer(buf, len, off, buffer, strlen(buffer));
    data_available = 0;
    mutex_unlock(&lock);

    return ret;
}

static ssize_t dev_write(struct file *file, const char __user *buf, size_t len, loff_t *off)
{
    int ret;

    if (len > BUF_LEN - 1)
        len = BUF_LEN - 1;

    mutex_lock(&lock);
    ret = copy_from_user(buffer, buf, len);
    buffer[len] = '\0';
    data_available = 1;
    mutex_unlock(&lock);

    wake_up_interruptible(&wq);
    printk(KERN_INFO "[%s] received: %s\n", DEVICE_NAME, buffer);
    return len;
}

static struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = dev_open,
    .release = dev_release,
    .read = dev_read,
    .write = dev_write
};

static int __init waitq_init(void)
{
    int ret;

    ret = alloc_chrdev_region(&dev_num, 0, 1, DEVICE_NAME);
    if (ret < 0)
        return ret;

    cdev_init(&waitq_cdev, &fops);
    cdev_add(&waitq_cdev, dev_num, 1);

    waitq_class = class_create(THIS_MODULE, CLASS_NAME);
    waitq_device = device_create(waitq_class, NULL, dev_num, NULL, DEVICE_NAME);

    printk(KERN_INFO "[%s] driver loaded\n", DEVICE_NAME);
    return 0;
}

static void __exit waitq_exit(void)
{
    device_destroy(waitq_class, dev_num);
    class_destroy(waitq_class);
    cdev_del(&waitq_cdev);
    unregister_chrdev_region(dev_num, 1);
    printk(KERN_INFO "[%s] unloaded\n", DEVICE_NAME);
}

module_init(waitq_init);
module_exit(waitq_exit);


 
MODULE_LICENSE("GPL");
