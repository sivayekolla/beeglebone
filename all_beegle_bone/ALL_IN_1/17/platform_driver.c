#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/of.h>  // Added for Device Tree support

#define MAX_DEVICES 2
#define DEVICE_NAME "my_platform_device"

// Per-device private data
struct my_device_data {
    struct cdev cdev;
    dev_t dev_num;
    struct device *device;
    int id;
    char buffer[256];
    int buffer_len;
};

// Platform driver data
struct my_driver_data {
    struct class *class;
    struct my_device_data devices[MAX_DEVICES];
    int major;
    struct platform_device *pdevs[MAX_DEVICES];
};

static struct my_driver_data *driver_data;

// File operations
static int my_open(struct inode *inode, struct file *file) {
    struct my_device_data *dev_data;
    
    dev_data = container_of(inode->i_cdev, struct my_device_data, cdev);
    file->private_data = dev_data;
    
    pr_info("Device %d opened\n", dev_data->id);
    return 0;
}

static int my_release(struct inode *inode, struct file *file) {
    struct my_device_data *dev_data = file->private_data;
    
    pr_info("Device %d closed\n", dev_data->id);
    return 0;
}

static ssize_t my_read(struct file *file, char __user *user_buf, size_t len, loff_t *offset) {
    struct my_device_data *dev_data = file->private_data;
    ssize_t bytes_to_read;
    
    if (*offset >= dev_data->buffer_len) {
        return 0;
    }
    
    bytes_to_read = min(len, (size_t)(dev_data->buffer_len - *offset));
    
    if (copy_to_user(user_buf, dev_data->buffer + *offset, bytes_to_read)) {
        return -EFAULT;
    }
    
    *offset += bytes_to_read;
    pr_info("Read %zd bytes from device %d\n", bytes_to_read, dev_data->id);
    return bytes_to_read;
}

static ssize_t my_write(struct file *file, const char __user *user_buf, size_t len, loff_t *offset) {
    struct my_device_data *dev_data = file->private_data;
    ssize_t bytes_to_write;
    
    bytes_to_write = min(len, (size_t)sizeof(dev_data->buffer) - 1);
    
    if (copy_from_user(dev_data->buffer, user_buf, bytes_to_write)) {
        return -EFAULT;
    }
    
    dev_data->buffer[bytes_to_write] = '\0';
    dev_data->buffer_len = bytes_to_write;
    
    pr_info("Wrote %zd bytes to device %d: %s\n", bytes_to_write, dev_data->id, dev_data->buffer);
    return bytes_to_write;
}

static struct file_operations my_fops = {
    .owner = THIS_MODULE,
    .open = my_open,
    .release = my_release,
    .read = my_read,
    .write = my_write,
};

// Permission callback
static char *my_devnode(const struct device *dev, umode_t *mode) {
    if (mode) {
        *mode = 0666;
    }
    return NULL;
}

// Platform driver probe
static int my_platform_probe(struct platform_device *pdev) {
    struct my_device_data *dev_data;
    int i;
    int ret;
    dev_t dev_num;
    
    pr_info("Platform device probe called for: %s\n", pdev->name);
    
    // Determine device index from name
    if (!strcmp(pdev->name, "mydevice0"))
        i = 0;
    else if (!strcmp(pdev->name, "mydevice1"))
        i = 1;
    else {
        pr_err("Unknown device name: %s\n", pdev->name);
        return -ENODEV;
    }
    
    dev_data = &driver_data->devices[i];
    dev_data->id = i;
    dev_data->buffer_len = 0;
    snprintf(dev_data->buffer, sizeof(dev_data->buffer), "Initial data for device %d\n", i);
    dev_data->buffer_len = strlen(dev_data->buffer);
    
    // Allocate device number
    dev_num = MKDEV(driver_data->major, i);
    dev_data->dev_num = dev_num;
    
    // Initialize and add cdev
    cdev_init(&dev_data->cdev, &my_fops);
    dev_data->cdev.owner = THIS_MODULE;
    
    ret = cdev_add(&dev_data->cdev, dev_num, 1);
    if (ret) {
        pr_err("Failed to add cdev for device %d\n", i);
        return ret;
    }
    
    // Create device node
    dev_data->device = device_create(driver_data->class, &pdev->dev, dev_num, 
                                     NULL, "myplatform%d", i);
    if (IS_ERR(dev_data->device)) {
        cdev_del(&dev_data->cdev);
        pr_err("Failed to create device node for device %d\n", i);
        return PTR_ERR(dev_data->device);
    }
    
    platform_set_drvdata(pdev, dev_data);
    pr_info("Device %d probed successfully\n", i);
    return 0;
}

// Platform driver remove
static int my_platform_remove(struct platform_device *pdev) {
    struct my_device_data *dev_data = platform_get_drvdata(pdev);
    
    pr_info("Device %d removed\n", dev_data->id);
    device_destroy(driver_data->class, dev_data->dev_num);
    cdev_del(&dev_data->cdev);
    
    return 0;
}

// Platform device IDs
static const struct platform_device_id my_platform_ids[] = {
    { "mydevice0", 0 },
    { "mydevice1", 1 },
    { }
};
MODULE_DEVICE_TABLE(platform, my_platform_ids);

// Platform driver structure
static struct platform_driver my_platform_driver = {
    .probe = my_platform_probe,
    .remove = my_platform_remove,
    .driver = {
        .name = DEVICE_NAME,
        .owner = THIS_MODULE,
    },
    .id_table = my_platform_ids,
};

// Create platform devices internally
static int create_platform_devices(void) {
    int i, ret;
    
    for (i = 0; i < MAX_DEVICES; i++) {
        char dev_name[20];
        snprintf(dev_name, sizeof(dev_name), "mydevice%d", i);
        
        driver_data->pdevs[i] = platform_device_alloc(dev_name, i);
        if (!driver_data->pdevs[i]) {
            pr_err("Failed to allocate platform device %d\n", i);
            goto error;
        }
        
        ret = platform_device_add(driver_data->pdevs[i]);
        if (ret) {
            platform_device_put(driver_data->pdevs[i]);
            pr_err("Failed to add platform device %d\n", i);
            goto error;
        }
        
        pr_info("Created platform device: %s\n", dev_name);
    }
    
    return 0;

error:
    while (--i >= 0) {
        platform_device_del(driver_data->pdevs[i]);
        platform_device_put(driver_data->pdevs[i]);
    }
    return -ENOMEM;
}

static void remove_platform_devices(void) {
    int i;
    
    for (i = 0; i < MAX_DEVICES; i++) {
        if (driver_data->pdevs[i]) {
            platform_device_del(driver_data->pdevs[i]);
            platform_device_put(driver_data->pdevs[i]);
            pr_info("Removed platform device %d\n", i);
        }
    }
}

// Module initialization
static int __init my_platform_init(void) {
    int ret;
    
    pr_info("Platform driver module init\n");
    
    // Allocate driver data
    driver_data = kzalloc(sizeof(*driver_data), GFP_KERNEL);
    if (!driver_data) {
        return -ENOMEM;
    }
    
    // Allocate major number
    ret = alloc_chrdev_region(&driver_data->devices[0].dev_num, 0, MAX_DEVICES, "myplatform");
    if (ret < 0) {
        kfree(driver_data);
        pr_err("Failed to allocate device numbers\n");
        return ret;
    }
    
    driver_data->major = MAJOR(driver_data->devices[0].dev_num);
    pr_info("Allocated major number: %d\n", driver_data->major);
    
    // Create class - FIXED: Only one argument in newer kernels
    driver_data->class = class_create("myplatform_class");
    if (IS_ERR(driver_data->class)) {
        unregister_chrdev_region(driver_data->devices[0].dev_num, MAX_DEVICES);
        kfree(driver_data);
        pr_err("Failed to create class\n");
        return PTR_ERR(driver_data->class);
    }
    
    driver_data->class->devnode = my_devnode;
    
    // Register platform driver
    ret = platform_driver_register(&my_platform_driver);
    if (ret) {
        class_destroy(driver_data->class);
        unregister_chrdev_region(driver_data->devices[0].dev_num, MAX_DEVICES);
        kfree(driver_data);
        pr_err("Failed to register platform driver\n");
        return ret;
    }
    
    // Create platform devices
    ret = create_platform_devices();
    if (ret) {
        platform_driver_unregister(&my_platform_driver);
        class_destroy(driver_data->class);
        unregister_chrdev_region(driver_data->devices[0].dev_num, MAX_DEVICES);
        kfree(driver_data);
        return ret;
    }
    
    pr_info("Platform driver with %d devices initialized successfully\n", MAX_DEVICES);
    return 0;
}

// Module exit
static void __exit my_platform_exit(void) {
    pr_info("Platform driver module exit\n");
    
    // Remove platform devices first
    remove_platform_devices();
    
    // Unregister platform driver
    platform_driver_unregister(&my_platform_driver);
    
    // Cleanup
    if (driver_data) {
        class_destroy(driver_data->class);
        unregister_chrdev_region(driver_data->devices[0].dev_num, MAX_DEVICES);
        kfree(driver_data);
    }
    
    pr_info("Platform driver unregistered\n");
}

module_init(my_platform_init);
module_exit(my_platform_exit);

MODULE_LICENSE("GPL");
