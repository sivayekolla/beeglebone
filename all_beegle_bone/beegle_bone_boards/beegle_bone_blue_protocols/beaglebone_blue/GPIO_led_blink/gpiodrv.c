// SPDX-License-Identifier: GPL-2.0
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/gpio/consumer.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/of.h>
struct myled_data {
    struct gpio_desc *gpiod;  // handle for LED GPIO
    struct device *dev;
    struct class *cls;
};

// Show LED state (0 = off, 1 = on)
static ssize_t state_show(struct device *dev,
                          struct device_attribute *attr, char *buf)
{
    struct myled_data *data = dev_get_drvdata(dev);
    int val = gpiod_get_value(data->gpiod);
    return sprintf(buf, "%d\n", val);
}

// Set LED state
static ssize_t state_store(struct device *dev,
                           struct device_attribute *attr,
                           const char *buf, size_t count)
{
    struct myled_data *data = dev_get_drvdata(dev);
    int val;

    if (kstrtoint(buf, 0, &val))
        return -EINVAL;

    gpiod_set_value(data->gpiod, val ? 1 : 0);
    return count;
}

static DEVICE_ATTR_RW(state);

static int myled_probe(struct platform_device *pdev)
{
    struct myled_data *data;
    int ret;

    // Allocate driver data
    data = devm_kzalloc(&pdev->dev, sizeof(*data), GFP_KERNEL);
    if (!data)
        return -ENOMEM;

    platform_set_drvdata(pdev, data);

    // Get GPIO from device tree
    data->gpiod = devm_gpiod_get(&pdev->dev, NULL, GPIOD_OUT_LOW);
    if (IS_ERR(data->gpiod)) {
        dev_err(&pdev->dev, "Failed to get GPIO\n");
        return PTR_ERR(data->gpiod);
    }

    // Create /sys/class/myled/led0
    data->cls = class_create(THIS_MODULE, "myled");
    if (IS_ERR(data->cls))
        return PTR_ERR(data->cls);

    data->dev = device_create(data->cls, NULL, 0, data, "led0");
    if (IS_ERR(data->dev)) {
        class_destroy(data->cls);
        return PTR_ERR(data->dev);
    }

    ret = device_create_file(data->dev, &dev_attr_state);
    if (ret)
        return ret;

    dev_info(&pdev->dev, "USR3 LED driver loaded\n");
    return 0;
}

static int myled_remove(struct platform_device *pdev)
{
    struct myled_data *data = platform_get_drvdata(pdev);
    device_remove_file(data->dev, &dev_attr_state);
    device_destroy(data->cls, 0);
    class_destroy(data->cls);
    return 0;
}

static const struct of_device_id myled_of_match[] = {
    { .compatible = "bbblue,myled" },
    {}
};
MODULE_DEVICE_TABLE(of, myled_of_match);

static struct platform_driver myled_driver = {
    .driver = {
        .name = "myled_driver",
        .of_match_table = myled_of_match,
    },
    .probe = myled_probe,
    .remove = myled_remove,
};

module_platform_driver(myled_driver);

MODULE_LICENSE("GPL");

