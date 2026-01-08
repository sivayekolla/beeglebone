#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/types.h>

#define MAX_DEVICES 5

static dev_t dev_number;
static struct cdev my_cdev[MAX_DEVICES];
static struct class *my_class;
static struct device *devices[MAX_DEVICES];

static int my_open(struct inode *inode, struct file *file) {
    pr_info("Device opened (minor: %d)\n", iminor(inode));
    return 0;
}

static int my_release(struct inode *inode, struct file *file) {
    pr_info("Device closed (minor: %d)\n", iminor(inode));
    return 0;
}

static ssize_t my_read(struct file *file, char __user *buffer, size_t len, loff_t *offset) {
    pr_info("Device read (minor: %d)\n", iminor(file->f_inode));
    return 0;
}

static ssize_t my_write(struct file *file, const char __user *buffer, size_t len, loff_t *offset) {
    pr_info("Device written to (minor: %d)\n", iminor(file->f_inode));
    return len;
}

static struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = my_open,
    .release = my_release,
    .read = my_read,
    .write = my_write,
};

static int __init my_driver_init(void) {
    int ret, i;

    ret = alloc_chrdev_region(&dev_number, 0, MAX_DEVICES, "my_pseudo_driver");
    if (ret < 0) {
        pr_err("Failed to allocate device numbers\n");
        return ret;
    }

    pr_info("Allocated major number: %d\n", MAJOR(dev_number));


    my_class = class_create("my_pseudo_class");
    if (IS_ERR(my_class)) {
        unregister_chrdev_region(dev_number, MAX_DEVICES);
        pr_err("Failed to create class\n");
        return PTR_ERR(my_class);
    }

    for (i = 0; i < MAX_DEVICES; i++) {

        devices[i] = device_create(my_class, NULL, MKDEV(MAJOR(dev_number), i), NULL, "mydevice%d", i);
        if (IS_ERR(devices[i])) {
            pr_err("Failed to create device %d\n", i);
            ret = PTR_ERR(devices[i]);
            goto error;
        }


        cdev_init(&my_cdev[i], &fops);
        ret = cdev_add(&my_cdev[i], MKDEV(MAJOR(dev_number), i), 1);
        if (ret) {
            pr_err("Failed to add cdev for device %d\n", i);
            device_destroy(my_class, MKDEV(MAJOR(dev_number), i));
            goto error;
        }
    }

    pr_info("Pseudo character driver initialized with %d devices\n", MAX_DEVICES);
    return 0;

error:

    for (; i >= 0; i--) {
        if (i < MAX_DEVICES && !IS_ERR(devices[i])) {
            cdev_del(&my_cdev[i]);
            device_destroy(my_class, MKDEV(MAJOR(dev_number), i));
        }
    }
    class_destroy(my_class);
    unregister_chrdev_region(dev_number, MAX_DEVICES);
    return ret;
}

static void __exit my_driver_exit(void) {
    int i;

    for (i = 0; i < MAX_DEVICES; i++) {
        cdev_del(&my_cdev[i]);
        device_destroy(my_class, MKDEV(MAJOR(dev_number), i));
    }

    class_destroy(my_class);
    unregister_chrdev_region(dev_number, MAX_DEVICES);

    pr_info("Pseudo character driver cleaned up\n");
}

module_init(my_driver_init);
module_exit(my_driver_exit);

MODULE_LICENSE("GPL");
