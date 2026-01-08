#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/gpio/consumer.h>
#include <linux/timer.h>
#include <linux/jiffies.h>
#include <linux/pm.h>

/* Blink interval: 500 ms */
#define BLINK_DELAY msecs_to_jiffies(500)

/* -------- Driver private data -------- */
struct blinkled_dev {
    struct gpio_desc *led;
    struct timer_list blink_timer;
    int led_state;
    int suspended;
};

/* -------- Timer callback -------- */
/* Runs in softirq context */
static void blink_timer_cb(struct timer_list *t)
{
    struct blinkled_dev *bdev =
        from_timer(bdev, t, blink_timer);

    if (bdev->suspended)
        return;

    bdev->led_state = !bdev->led_state;
    gpiod_set_value(bdev->led, bdev->led_state);

    mod_timer(&bdev->blink_timer,
              jiffies + BLINK_DELAY);
}

/* -------- Suspend callback -------- */
static int blinkled_suspend(struct device *dev)
{
    struct blinkled_dev *bdev = dev_get_drvdata(dev);

    pr_info("[blinkled] Suspending...\n");

    bdev->suspended = 1;

    /* Stop blinking */
    del_timer_sync(&bdev->blink_timer);

    /* Turn OFF LED to save power */
    gpiod_set_value(bdev->led, 0);

    return 0;
}

/* -------- Resume callback -------- */
static int blinkled_resume(struct device *dev)
{
    struct blinkled_dev *bdev = dev_get_drvdata(dev);

    pr_info("[blinkled] Resuming...\n");

    bdev->suspended = 0;

    /* Restore LED state */
    gpiod_set_value(bdev->led, bdev->led_state);

    /* Restart blinking */
    mod_timer(&bdev->blink_timer,
              jiffies + BLINK_DELAY);

    return 0;
}

/* -------- Power management ops -------- */
static const struct dev_pm_ops blinkled_pm_ops = {
    .suspend = blinkled_suspend,
    .resume  = blinkled_resume,
};

/* -------- Probe -------- */
static int blinkled_probe(struct platform_device *pdev)
{
    struct blinkled_dev *bdev;

    pr_info("[blinkled] probe\n");

    bdev = devm_kzalloc(&pdev->dev,
                        sizeof(*bdev),
                        GFP_KERNEL);
    if (!bdev)
        return -ENOMEM;

    /* Get LED GPIO from device tree */
    bdev->led = devm_gpiod_get(&pdev->dev,
                               "led",
                               GPIOD_OUT_LOW);
    if (IS_ERR(bdev->led))
        return PTR_ERR(bdev->led);

    bdev->led_state = 0;
    bdev->suspended = 0;

    /* Setup kernel timer */
    timer_setup(&bdev->blink_timer,
                blink_timer_cb,
                0);

    mod_timer(&bdev->blink_timer,
              jiffies + BLINK_DELAY);

    platform_set_drvdata(pdev, bdev);

    pr_info("[blinkled] blinking started\n");
    return 0;
}

/* -------- Remove -------- */
static int blinkled_remove(struct platform_device *pdev)
{
    struct blinkled_dev *bdev =
        platform_get_drvdata(pdev);

    del_timer_sync(&bdev->blink_timer);
    gpiod_set_value(bdev->led, 0);

    pr_info("[blinkled] removed\n");
    return 0;
}

/* -------- Device Tree Match -------- */
static const struct of_device_id blinkled_of_ids[] = {
    { .compatible = "custom,blinkled" },
    { }
};
MODULE_DEVICE_TABLE(of, blinkled_of_ids);

/* -------- Platform Driver -------- */
static struct platform_driver blinkled_driver = {
    .probe  = blinkled_probe,
    .remove = blinkled_remove,
    .driver = {
        .name = "blinkled",
        .of_match_table = blinkled_of_ids,
        .pm = &blinkled_pm_ops,
    },
};

module_platform_driver(blinkled_driver);

MODULE_LICENSE("GPL");
