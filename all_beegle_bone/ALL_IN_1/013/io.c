#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/device.h>
#include <linux/cdev.h>

#define DEVICE_NAME "mydev"
#define CLASS_NAME "multi"
#define NUM_DEVICES 3
#define BUF_SIZE 256

MODULE_LICENSE("GPL");
MODULE_AUTHOR("You");
MODULE_DESCRIPTION("Multiple minor devices with one driver");

static dev_t dev_base;
static struct class *my_class;
struct mydev_t {
    int index;
    char buffer[BUF_SIZE];
    struct cdev cdev;
};
static struct mydev_t my_devices[NUM_DEVICES];

static int dev_open(struct inode *inode, struct file *file)
{
    struct mydev_t *dev = container_of(inode->i_cdev, struct mydev_t, cdev);
    file->private_data = dev;
    printk(KERN_INFO "[mydev%d] opened\n", dev->index);
    return 0;
}

static int dev_release(struct inode *inode, struct file *file)
{
    struct mydev_t *dev = file->private_data;
    printk(KERN_INFO "[mydev%d] closed\n", dev->index);
    return 0;
}

static ssize_t dev_read(struct file *file, char __user *buf, size_t len, loff_t *off)
{
    struct mydev_t *dev = file->private_data;
    return simple_read_from_buffer(buf, len, off, dev->buffer, strlen(dev->buffer));
}

static ssize_t dev_write(struct file *file, const char __user *buf, size_t len, loff_t *off)
{
    struct mydev_t *dev = file->private_data;
    if (len > BUF_SIZE - 1)
        len = BUF_SIZE - 1;

    if (copy_from_user(dev->buffer, buf, len))
        return -EFAULT;

    dev->buffer[len] = '\0';
    printk(KERN_INFO "[mydev%d] wrote: %s\n", dev->index, dev->buffer);
    return len;
}

static struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = dev_open,
    .read = dev_read,
    .write = dev_write,
    .release = dev_release,
};

static int __init multi_dev_init(void)
{
    int ret, i;
    dev_t devno;
    struct device *dev;

    // Allocate device numbers dynamically
    ret = alloc_chrdev_region(&dev_base, 0, NUM_DEVICES, DEVICE_NAME);
    if (ret < 0) {
        printk(KERN_ALERT "alloc_chrdev_region failed\n");
        return ret;
    }

    // Create a class for the devices
    my_class = class_create(THIS_MODULE, CLASS_NAME);  // Pass ONLY the class name
    if (IS_ERR(my_class)) {
        unregister_chrdev_region(dev_base, NUM_DEVICES);
        printk(KERN_ALERT "class_create failed\n");
        return PTR_ERR(my_class);
    }

    // Loop through and initialize each device
    for (i = 0; i < NUM_DEVICES; i++) {
        my_devices[i].index = i;
        devno = MKDEV(MAJOR(dev_base), i);

        cdev_init(&my_devices[i].cdev, &fops);
        my_devices[i].cdev.owner = THIS_MODULE;
        ret = cdev_add(&my_devices[i].cdev, devno, 1);
        if (ret < 0) {
            printk(KERN_ALERT "cdev_add failed for %d\n", i);
            continue;
        }

        // Create device node in /dev/
        dev = device_create(my_class, NULL, devno, NULL, "%s%d", DEVICE_NAME, i);
        if (IS_ERR(dev)) {
            printk(KERN_ALERT "device_create failed for %d\n", i);
            cdev_del(&my_devices[i].cdev);
            continue;
        }

        printk(KERN_INFO "/dev/%s%d created\n", DEVICE_NAME, i);
    }

    return 0;
}

static void __exit multi_dev_exit(void)
{
    int i;
    for (i = 0; i < NUM_DEVICES; i++) {
        device_destroy(my_class, MKDEV(MAJOR(dev_base), i));
        cdev_del(&my_devices[i].cdev);
    }
    class_destroy(my_class);
    unregister_chrdev_region(dev_base, NUM_DEVICES);
    printk(KERN_INFO "multi_dev unloaded\n");
}

module_init(multi_dev_init);
module_exit(multi_dev_exit);

