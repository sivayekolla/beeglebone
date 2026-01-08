#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/gpio/consumer.h>

#define MAX_DEVICES 2
#define DRIVER_NAME "mydt_driver"

static const char *dt_overlay = 
"/dts-v1/;\n"
"/plugin/;\n"
"\n"
"/ {\n"
"    compatible = \"mycompany,test-board\";\n"
"    fragment@0 {\n"
"        target-path = \"/\";\n"
"        __overlay__ {\n"
"            mydt0: mydt@0 {\n"
"                compatible = \"mycompany,mydt\";\n"
"                label = \"mydt0\";\n"
"                status = \"okay\";\n"
"                led-gpios = <&gpio 17 0>;\n"
"                reg = <0>;\n"
"            };\n"
"            mydt1: mydt@1 {\n"
"                compatible = \"mycompany,mydt\";\n"
"                label = \"mydt1\";\n"
"                status = \"okay\";\n"
"                led-gpios = <&gpio 27 0>;\n"
"                reg = <1>;\n"
"            };\n"
"        };\n"
"    };\n"
"};\n";

struct mydt_device_data {
    struct cdev cdev;
    dev_t dev_num;
    struct device *device;
    struct gpio_desc *led_gpio;
    int id;
    char buffer[256];
    int buffer_len;
    int led_state;
};

struct mydt_driver_data {
    struct class *class;
    struct mydt_device_data devices[MAX_DEVICES];
    int major;
    struct platform_device *pdevs[MAX_DEVICES];
};

static struct mydt_driver_data *driver_data;

static int mydt_open(struct inode *inode, struct file *file) {
    struct mydt_device_data *dev_data;
    dev_data = container_of(inode->i_cdev, struct mydt_device_data, cdev);
    file->private_data = dev_data;
    pr_info("Device %d opened\n", dev_data->id);
    return 0;
}

static int mydt_release(struct inode *inode, struct file *file) {
    struct mydt_device_data *dev_data = file->private_data;
    pr_info("Device %d closed\n", dev_data->id);
    return 0;
}

static ssize_t mydt_read(struct file *file, char __user *user_buf, 
                        size_t len, loff_t *offset) {
    struct mydt_device_data *dev_data = file->private_data;
    char status_msg[300];
    int msg_len;
    
    if (*offset > 0) return 0;
    
    msg_len = snprintf(status_msg, sizeof(status_msg),
        "Device %d Status\n"
        "LED State: %d\n"
        "Buffer: %s\n"
        "Buffer Size: %d\n",
        dev_data->id,
        dev_data->led_state,
        dev_data->buffer,
        dev_data->buffer_len);
    
    msg_len = min(msg_len, (int)len);
    
    if (copy_to_user(user_buf, status_msg, msg_len)) {
        return -EFAULT;
    }
    
    *offset = msg_len;
    return msg_len;
}

static ssize_t mydt_write(struct file *file, const char __user *user_buf, 
                         size_t len, loff_t *offset) {
    struct mydt_device_data *dev_data = file->private_data;
    int bytes_to_write;
    
    bytes_to_write = min(len, (size_t)sizeof(dev_data->buffer) - 1);
    
    if (copy_from_user(dev_data->buffer, user_buf, bytes_to_write)) {
        return -EFAULT;
    }
    
    dev_data->buffer[bytes_to_write] = '\0';
    dev_data->buffer_len = bytes_to_write;
    
    if (strstr(dev_data->buffer, "ledon") && dev_data->led_gpio) {
        gpiod_set_value(dev_data->led_gpio, 1);
        dev_data->led_state = 1;
        pr_info("Device %d: LED ON\n", dev_data->id);
    } 
    else if (strstr(dev_data->buffer, "ledoff") && dev_data->led_gpio) {
        gpiod_set_value(dev_data->led_gpio, 0);
        dev_data->led_state = 0;
        pr_info("Device %d: LED OFF\n", dev_data->id);
    }
    else if (strstr(dev_data->buffer, "toggle") && dev_data->led_gpio) {
        dev_data->led_state = !dev_data->led_state;
        gpiod_set_value(dev_data->led_gpio, dev_data->led_state);
        pr_info("Device %d: LED TOGGLED\n", dev_data->id);
    }
    else {
        pr_info("Device %d: Data: %s\n", dev_data->id, dev_data->buffer);
    }
    
    return bytes_to_write;
}

static struct file_operations mydt_fops = {
    .owner = THIS_MODULE,
    .open = mydt_open,
    .release = mydt_release,
    .read = mydt_read,
    .write = mydt_write,
};

static char *mydt_devnode(const struct device *dev, umode_t *mode) {
    if (mode) *mode = 0666;
    return NULL;
}

static const struct of_device_id mydt_of_match[] = {
    { .compatible = "mycompany,mydt" },
    { }
};
MODULE_DEVICE_TABLE(of, mydt_of_match);

static const struct platform_device_id mydt_platform_ids[] = {
    { "mydt0", 0 },
    { "mydt1", 1 },
    { }
};
MODULE_DEVICE_TABLE(platform, mydt_platform_ids);

static int mydt_platform_probe(struct platform_device *pdev) {
    struct mydt_device_data *dev_data;
    struct device_node *node = pdev->dev.of_node;
    int i, ret;
    dev_t dev_num;
    u32 reg_val;
    
    pr_info("Probe: %s\n", pdev->name);
    
    if (node) {
        ret = of_property_read_u32(node, "reg", &reg_val);
        if (!ret) i = reg_val;
        else i = 0;
    } else {
        if (strstr(pdev->name, "mydt0")) i = 0;
        else if (strstr(pdev->name, "mydt1")) i = 1;
        else i = 0;
    }
    
    if (i >= MAX_DEVICES) return -EINVAL;
    
    dev_data = &driver_data->devices[i];
    dev_data->id = i;
    
    snprintf(dev_data->buffer, sizeof(dev_data->buffer),
             "Device %d ready\n", i);
    dev_data->buffer_len = strlen(dev_data->buffer);
    dev_data->led_state = 0;
    
    if (node) {
        dev_data->led_gpio = gpiod_get(&pdev->dev, "led", GPIOD_OUT_LOW);
        if (IS_ERR(dev_data->led_gpio)) {
            dev_data->led_gpio = NULL;
        }
    } else {
        dev_data->led_gpio = NULL;
    }
    
    dev_num = MKDEV(driver_data->major, i);
    dev_data->dev_num = dev_num;
    
    cdev_init(&dev_data->cdev, &mydt_fops);
    dev_data->cdev.owner = THIS_MODULE;
    
    ret = cdev_add(&dev_data->cdev, dev_num, 1);
    if (ret) {
        if (dev_data->led_gpio) gpiod_put(dev_data->led_gpio);
        return ret;
    }
    
    dev_data->device = device_create(driver_data->class, &pdev->dev, 
                                     dev_num, NULL, "mydt%d", i);
    if (IS_ERR(dev_data->device)) {
        cdev_del(&dev_data->cdev);
        if (dev_data->led_gpio) gpiod_put(dev_data->led_gpio);
        return PTR_ERR(dev_data->device);
    }
    
    platform_set_drvdata(pdev, dev_data);
    pr_info("Device %d ready\n", i);
    return 0;
}

static int mydt_platform_remove(struct platform_device *pdev) {
    struct mydt_device_data *dev_data = platform_get_drvdata(pdev);
    
    if (dev_data->led_gpio) {
        gpiod_set_value(dev_data->led_gpio, 0);
        gpiod_put(dev_data->led_gpio);
    }
    
    device_destroy(driver_data->class, dev_data->dev_num);
    cdev_del(&dev_data->cdev);
    
    pr_info("Device %d removed\n", dev_data->id);
    return 0;
}

static struct platform_driver mydt_platform_driver = {
    .probe = mydt_platform_probe,
    .remove = mydt_platform_remove,
    .driver = {
        .name = DRIVER_NAME,
        .owner = THIS_MODULE,
        .of_match_table = mydt_of_match,
    },
    .id_table = mydt_platform_ids,
};

static int create_platform_devices(void) {
    int i, ret;
    
    for (i = 0; i < MAX_DEVICES; i++) {
        char dev_name[20];
        snprintf(dev_name, sizeof(dev_name), "mydt%d", i);
        
        driver_data->pdevs[i] = platform_device_alloc(dev_name, i);
        if (!driver_data->pdevs[i]) goto error;
        
        ret = platform_device_add(driver_data->pdevs[i]);
        if (ret) {
            platform_device_put(driver_data->pdevs[i]);
            goto error;
        }
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
        }
    }
}

static int __init mydt_driver_init(void) {
    int ret;
    
    pr_info("DT Platform Driver Init\n");
    printk(KERN_INFO "DT Overlay:\n%s\n", dt_overlay);
    
    driver_data = kzalloc(sizeof(*driver_data), GFP_KERNEL);
    if (!driver_data) return -ENOMEM;
    
    ret = alloc_chrdev_region(&driver_data->devices[0].dev_num, 0, 
                              MAX_DEVICES, "mydt");
    if (ret < 0) {
        kfree(driver_data);
        return ret;
    }
    
    driver_data->major = MAJOR(driver_data->devices[0].dev_num);
    pr_info("Major: %d\n", driver_data->major);
    
    driver_data->class = class_create("mydt_class");
    if (IS_ERR(driver_data->class)) {
        unregister_chrdev_region(driver_data->devices[0].dev_num, MAX_DEVICES);
        kfree(driver_data);
        return PTR_ERR(driver_data->class);
    }
    
    driver_data->class->devnode = mydt_devnode;
    
    ret = platform_driver_register(&mydt_platform_driver);
    if (ret) {
        class_destroy(driver_data->class);
        unregister_chrdev_region(driver_data->devices[0].dev_num, MAX_DEVICES);
        kfree(driver_data);
        return ret;
    }
    
    ret = create_platform_devices();
    if (ret) {
        platform_driver_unregister(&mydt_platform_driver);
        class_destroy(driver_data->class);
        unregister_chrdev_region(driver_data->devices[0].dev_num, MAX_DEVICES);
        kfree(driver_data);
        return ret;
    }
    
    pr_info("Driver ready. Devices: /dev/mydt0, /dev/mydt1\n");
    return 0;
}

static void __exit mydt_driver_exit(void) {
    pr_info("DT Platform Driver Exit\n");
    
    remove_platform_devices();
    platform_driver_unregister(&mydt_platform_driver);
    
    if (driver_data) {
        class_destroy(driver_data->class);
        unregister_chrdev_region(driver_data->devices[0].dev_num, MAX_DEVICES);
        kfree(driver_data);
    }
    
    pr_info("Driver removed\n");
}

module_init(mydt_driver_init);
module_exit(mydt_driver_exit);

MODULE_LICENSE("GPL");
