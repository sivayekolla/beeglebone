#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/gpio/consumer.h>
#include <linux/printk.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("BBB GPIO Driver with dmesg logging");

#define DEVICE_NAME "bbb_gpio"
#define CLASS_NAME "bbb_gpio"

static int major_number;
static struct class* gpio_class = NULL;
static struct device* gpio_device = NULL;
static struct cdev gpio_cdev;

// GPIO structures
static struct gpio_desc *led_gpio = NULL;
static struct gpio_desc *button_gpio = NULL;

static int device_open(struct inode *inode, struct file *file)
{
    pr_info("BBB GPIO: Device opened by process %d\n", current->pid);
    return 0;
}

static int device_release(struct inode *inode, struct file *file)
{
    pr_info("BBB GPIO: Device closed\n");
    return 0;
}

static ssize_t device_read(struct file *filp, char *buffer, size_t length, loff_t *offset)
{
    char message[256];
    int button_state = 0;
    int bytes_to_copy;
    
    if (*offset > 0)
        return 0;
    
    // Read button state if available
    if (button_gpio && !IS_ERR(button_gpio)) {
        button_state = gpiod_get_value(button_gpio);
        pr_debug("BBB GPIO: Button state read: %d\n", button_state);
    }
    
    sprintf(message, "BBB GPIO Driver\nLED Control: Write '1' to turn ON, '0' to turn OFF\nButton State: %d\n", button_state);
    
    bytes_to_copy = strlen(message);
    if (length < bytes_to_copy)
        bytes_to_copy = length;
    
    if (copy_to_user(buffer, message, bytes_to_copy)) {
        pr_err("BBB GPIO: Failed to copy to user\n");
        return -EFAULT;
    }
    
    *offset += bytes_to_copy;
    pr_debug("BBB GPIO: Read %d bytes\n", bytes_to_copy);
    return bytes_to_copy;
}

static ssize_t device_write(struct file *filp, const char *buffer, size_t length, loff_t *offset)
{
    char command;
    
    if (length < 1) {
        pr_warn("BBB GPIO: Write command too short\n");
        return -EINVAL;
    }
    
    if (copy_from_user(&command, buffer, 1)) {
        pr_err("BBB GPIO: Failed to copy from user\n");
        return -EFAULT;
    }
    
    // Control LED if available
    if (led_gpio && !IS_ERR(led_gpio)) {
        if (command == '1') {
            gpiod_set_value(led_gpio, 1);
            pr_info("BBB GPIO: LED turned ON (command from process %d)\n", current->pid);
        } else if (command == '0') {
            gpiod_set_value(led_gpio, 0);
            pr_info("BBB GPIO: LED turned OFF (command from process %d)\n", current->pid);
        } else if (command == 't' || command == 'T') {
            // Toggle LED
            int current_state = gpiod_get_value(led_gpio);
            gpiod_set_value(led_gpio, !current_state);
            pr_info("BBB GPIO: LED toggled to %s\n", !current_state ? "ON" : "OFF");
        } else {
            pr_warn("BBB GPIO: Invalid command '%c'. Use '1', '0', or 't'\n", command);
        }
    } else {
        pr_warn("BBB GPIO: No LED GPIO configured\n");
    }
    
    pr_debug("BBB GPIO: Write completed, %zu bytes processed\n", length);
    return 1;
}

static struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = device_open,
    .release = device_release,
    .read = device_read,
    .write = device_write,
};

static int bbb_gpio_probe(struct platform_device *pdev)
{
    struct device *dev = &pdev->dev;
    dev_t dev_num;
    int result;
    
    pr_info("BBB GPIO: Device Tree match found for %s\n", dev_name(dev));
    
    // Get GPIOs from device tree
    led_gpio = gpiod_get(dev, "led", GPIOD_OUT_LOW);
    if (IS_ERR(led_gpio)) {
        pr_info("BBB GPIO: No LED GPIO specified in DT, using USR3 LED as fallback\n");
        
        // Try to use USR3 LED as fallback
        led_gpio = gpiod_get(dev, "usr3", GPIOD_OUT_LOW);
        if (IS_ERR(led_gpio)) {
            pr_warn("BBB GPIO: No GPIOs available in DT\n");
            led_gpio = NULL;
        } else {
            pr_info("BBB GPIO: Using USR3 LED GPIO\n");
        }
    } else {
        pr_info("BBB GPIO: Got LED GPIO from DT\n");
    }
    
    button_gpio = gpiod_get(dev, "button", GPIOD_IN);
    if (IS_ERR(button_gpio)) {
        pr_info("BBB GPIO: No button GPIO specified in DT\n");
        button_gpio = NULL;
    } else {
        pr_info("BBB GPIO: Got button GPIO from DT\n");
    }
    
    // Allocate major number
    result = alloc_chrdev_region(&dev_num, 0, 1, DEVICE_NAME);
    if (result < 0) {
        pr_err("BBB GPIO: Failed to allocate device number\n");
        goto err_gpio;
    }
    major_number = MAJOR(dev_num);
    pr_info("BBB GPIO: Allocated major number %d\n", major_number);
    
    // Initialize character device
    cdev_init(&gpio_cdev, &fops);
    gpio_cdev.owner = THIS_MODULE;
    
    // Add character device to system
    result = cdev_add(&gpio_cdev, dev_num, 1);
    if (result < 0) {
        pr_err("BBB GPIO: Failed to add cdev\n");
        goto err_chrdev;
    }
    
    // Create device class
    gpio_class = class_create(CLASS_NAME);
    if (IS_ERR(gpio_class)) {
        pr_err("BBB GPIO: Failed to create class\n");
        result = PTR_ERR(gpio_class);
        goto err_cdev;
    }
    
    // Create device
    gpio_device = device_create(gpio_class, NULL, dev_num, NULL, DEVICE_NAME);
    if (IS_ERR(gpio_device)) {
        pr_err("BBB GPIO: Failed to create device\n");
        result = PTR_ERR(gpio_device);
        goto err_class;
    }
    
    pr_info("BBB GPIO: Driver loaded successfully\n");
    pr_info("BBB GPIO: Device created: /dev/%s\n", DEVICE_NAME);
    
    // Initial LED state
    if (led_gpio && !IS_ERR(led_gpio)) {
        gpiod_set_value(led_gpio, 0);
        pr_info("BBB GPIO: Initialized LED to OFF\n");
    }
    
    return 0;

err_class:
    class_destroy(gpio_class);
err_cdev:
    cdev_del(&gpio_cdev);
err_chrdev:
    unregister_chrdev_region(dev_num, 1);
err_gpio:
    if (led_gpio && !IS_ERR(led_gpio))
        gpiod_put(led_gpio);
    if (button_gpio && !IS_ERR(button_gpio))
        gpiod_put(button_gpio);
    return result;
}

static int bbb_gpio_remove(struct platform_device *pdev)
{
    dev_t dev_num = MKDEV(major_number, 0);
    
    pr_info("BBB GPIO: Removing driver\n");
    
    if (gpio_device)
        device_destroy(gpio_class, dev_num);
    
    if (gpio_class)
        class_destroy(gpio_class);
    
    cdev_del(&gpio_cdev);
    unregister_chrdev_region(dev_num, 1);
    
    if (led_gpio && !IS_ERR(led_gpio)) {
        gpiod_set_value(led_gpio, 0);  // Turn off LED before removing
        gpiod_put(led_gpio);
        pr_info("BBB GPIO: LED GPIO released\n");
    }
    
    if (button_gpio && !IS_ERR(button_gpio)) {
        gpiod_put(button_gpio);
        pr_info("BBB GPIO: Button GPIO released\n");
    }
    
    pr_info("BBB GPIO: Driver unloaded\n");
    
    return 0;
}

static const struct of_device_id bbb_gpio_of_match[] = {
    { .compatible = "bbb-gpio-driver", },
    { },
};
MODULE_DEVICE_TABLE(of, bbb_gpio_of_match);

static struct platform_driver bbb_gpio_driver = {
    .probe = bbb_gpio_probe,
    .remove = bbb_gpio_remove,
    .driver = {
        .name = "bbb_gpio_driver",
        .of_match_table = bbb_gpio_of_match,
        .owner = THIS_MODULE,
    },
};

static int __init bbb_gpio_init(void)
{
    pr_info("BBB GPIO: Driver initializing\n");
    return platform_driver_register(&bbb_gpio_driver);
}

static void __exit bbb_gpio_exit(void)
{
    pr_info("BBB GPIO: Driver exiting\n");
    platform_driver_unregister(&bbb_gpio_driver);
}

module_init(bbb_gpio_init);
module_exit(bbb_gpio_exit);
