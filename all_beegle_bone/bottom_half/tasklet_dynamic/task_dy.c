#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/interrupt.h>

/* -------------------------------------------------
 * NOTE:
 * IRQ 11 is used ONLY for demonstration.
 * On real hardware this may not exist.
 * We simulate IRQ from read() to show flow.
 * ------------------------------------------------- */
#define IRQ_NO 11

/* ---------- Global Variables ---------- */
static dev_t dev;                  /* Device number */
static struct class *dev_class;    /* Device class */
static struct cdev etx_cdev;       /* Character device */

/* ---------- Dynamic Tasklet ---------- */
static struct tasklet_struct my_tasklet;

/*
 * Tasklet function
 * Runs in softirq context (cannot sleep)
 */
static void tasklet_fn(unsigned long data)
{
    printk(KERN_INFO "etx: Dynamic tasklet executed, data=%lu\n", data);
}

/* ---------- File Operations ---------- */

/* Device open */
static int etx_open(struct inode *inode, struct file *file)
{
    printk(KERN_INFO "etx: Device opened\n");
    return 0;
}

/* Device close */
static int etx_release(struct inode *inode, struct file *file)
{
    printk(KERN_INFO "etx: Device closed\n");
    return 0;
}

/*
 * IRQ handler (Top Half)
 * Must be fast and non-blocking
 */
static irqreturn_t irq_handler(int irq, void *dev_id)
{
    printk(KERN_INFO "etx: IRQ %d occurred, scheduling tasklet\n", irq);

    /* Schedule bottom half */
    tasklet_schedule(&my_tasklet);

    return IRQ_HANDLED;
}

/*
 * Simulate IRQ for testing
 * (since we don't have real hardware IRQ)
 */
static void simulate_irq(void)
{
    irq_handler(IRQ_NO, &etx_cdev);
}

/*
 * Read operation
 * Triggers simulated IRQ
 */
static ssize_t etx_read(struct file *filp,
                        char __user *buf,
                        size_t len,
                        loff_t *off)
{
    printk(KERN_INFO "etx: Read called, simulating IRQ\n");

    simulate_irq();

    return 0;
}

/* Write operation */
static ssize_t etx_write(struct file *filp,
                         const char __user *buf,
                         size_t len,
                         loff_t *off)
{
    printk(KERN_INFO "etx: Write called\n");
    return len;
}

/* File operations structure */
static struct file_operations fops = {
    .owner   = THIS_MODULE,
    .open    = etx_open,
    .release = etx_release,
    .read    = etx_read,
    .write   = etx_write,
};

/* ---------- Module Initialization ---------- */
static int __init etx_driver_init(void)
{
    int ret;

    /* Allocate major/minor numbers */
    ret = alloc_chrdev_region(&dev, 0, 1, "etx_device");
    if (ret < 0) {
        printk(KERN_ERR "etx: Failed to allocate device number\n");
        return ret;
    }

    printk(KERN_INFO "etx: Major=%d Minor=%d\n",
           MAJOR(dev), MINOR(dev));

    /* Initialize and add cdev */
    cdev_init(&etx_cdev, &fops);
    ret = cdev_add(&etx_cdev, dev, 1);
    if (ret) {
        printk(KERN_ERR "etx: cdev_add failed\n");
        unregister_chrdev_region(dev, 1);
        return ret;
    }

    /*
     * Create device class
     * Linux 6.x API: class_create(const char *name)
     */
    dev_class = class_create("etx_class");
    if (IS_ERR(dev_class)) {
        printk(KERN_ERR "etx: Failed to create class\n");
        cdev_del(&etx_cdev);
        unregister_chrdev_region(dev, 1);
        return PTR_ERR(dev_class);
    }

    /* Create device node: /dev/etx_device */
    if (IS_ERR(device_create(dev_class, NULL, dev,
                             NULL, "etx_device"))) {
        printk(KERN_ERR "etx: Failed to create device\n");
        class_destroy(dev_class);
        cdev_del(&etx_cdev);
        unregister_chrdev_region(dev, 1);
        return -EINVAL;
    }

    /*
     * Initialize dynamic tasklet
     * data = 123 (passed to tasklet_fn)
     */
    tasklet_init(&my_tasklet, tasklet_fn, 123);

    /*
     * Request IRQ
     * IRQF_SHARED requires dev_id to be unique
     */
    ret = request_irq(IRQ_NO, irq_handler,
                      IRQF_SHARED,
                      "etx_device",
                      &etx_cdev);

    if (ret) {
        printk(KERN_ERR "etx: Cannot register IRQ %d\n", IRQ_NO);
        tasklet_kill(&my_tasklet);
        device_destroy(dev_class, dev);
        class_destroy(dev_class);
        cdev_del(&etx_cdev);
        unregister_chrdev_region(dev, 1);
        return ret;
    }

    printk(KERN_INFO "etx: Driver loaded (IRQ + Dynamic Tasklet)\n");
    return 0;
}

/* ---------- Module Exit ---------- */
static void __exit etx_driver_exit(void)
{
    /* Free IRQ */
    free_irq(IRQ_NO, &etx_cdev);

    /* Kill tasklet (waits if running) */
    tasklet_kill(&my_tasklet);

    /* Cleanup device */
    device_destroy(dev_class, dev);
    class_destroy(dev_class);
    cdev_del(&etx_cdev);
    unregister_chrdev_region(dev, 1);

    printk(KERN_INFO "etx: Driver unloaded\n");
}

module_init(etx_driver_init);
module_exit(etx_driver_exit);

/* ---------- Module Info ---------- */
MODULE_LICENSE("GPL");
