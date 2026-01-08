#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/interrupt.h>

/* Dummy IRQ number for demo */
#define IRQ_NO 11

dev_t dev = 0;
static struct class *dev_class;
static struct cdev etx_cdev;

/* ---------------- TASKLET ---------------- */

/* Tasklet handler (new API) */
static void tasklet_fn(struct tasklet_struct *t)
{
    printk(KERN_INFO "Tasklet executed (softirq context)\n");
}

/* Static tasklet declaration */
DECLARE_TASKLET(my_tasklet, tasklet_fn);

/* ---------------- FILE OPS ---------------- */

static int etx_open(struct inode *inode, struct file *file)
{
    printk(KERN_INFO "etx: Device opened\n");
    return 0;
}

static int etx_release(struct inode *inode, struct file *file)
{
    printk(KERN_INFO "etx: Device closed\n");
    return 0;
}

/* IRQ handler */
static irqreturn_t irq_handler(int irq, void *dev_id)
{
    printk(KERN_INFO "etx: IRQ %d occurred → scheduling tasklet\n", irq);
    tasklet_schedule(&my_tasklet);
    return IRQ_HANDLED;
}

/* Fake IRQ trigger */
static void simulate_irq(void)
{
    irq_handler(IRQ_NO, &etx_cdev);
}

static ssize_t etx_read(struct file *filp,
                        char __user *buf,
                        size_t len,
                        loff_t *off)
{
    printk(KERN_INFO "etx: Read called → simulating IRQ\n");
    simulate_irq();
    return 0;
}

static ssize_t etx_write(struct file *filp,
                         const char __user *buf,
                         size_t len,
                         loff_t *off)
{
    printk(KERN_INFO "etx: Write called\n");
    return len;
}

static struct file_operations fops = {
    .owner   = THIS_MODULE,
    .open    = etx_open,
    .release = etx_release,
    .read    = etx_read,
    .write   = etx_write,
};

/* ---------------- INIT ---------------- */

static int __init etx_driver_init(void)
{
    int ret;

    /* Allocate device number */
    ret = alloc_chrdev_region(&dev, 0, 1, "etx_device");
    if (ret < 0)
        return ret;

    printk(KERN_INFO "Major=%d Minor=%d\n", MAJOR(dev), MINOR(dev));

    /* Create cdev */
    cdev_init(&etx_cdev, &fops);
    cdev_add(&etx_cdev, dev, 1);

    /* Create class (NEW API) */
    dev_class = class_create("etx_class");
    if (IS_ERR(dev_class))
        goto r_class;

    /* Create device */
    if (IS_ERR(device_create(dev_class, NULL, dev, NULL, "etx_device")))
        goto r_device;

    /* Request IRQ */
    ret = request_irq(IRQ_NO, irq_handler, IRQF_SHARED,
                      "etx_device", &etx_cdev);
    if (ret)
        goto r_irq;

    printk(KERN_INFO "etx: Driver loaded (static tasklet)\n");
    return 0;

r_irq:
    device_destroy(dev_class, dev);
r_device:
    class_destroy(dev_class);
r_class:
    cdev_del(&etx_cdev);
    unregister_chrdev_region(dev, 1);
    return -1;
}

/* ---------------- EXIT ---------------- */

static void __exit etx_driver_exit(void)
{
    free_irq(IRQ_NO, &etx_cdev);
    tasklet_kill(&my_tasklet);
    device_destroy(dev_class, dev);
    class_destroy(dev_class);
    cdev_del(&etx_cdev);
    unregister_chrdev_region(dev, 1);

    printk(KERN_INFO "etx: Driver unloaded\n");
}

module_init(etx_driver_init);
module_exit(etx_driver_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("PREETHI");
MODULE_DESCRIPTION("Static Tasklet Driver (Linux 6.8 compatible)");

