#include <linux/kernel.h>      // printk(), KERN_INFO
#include <linux/init.h>        // module_init(), module_exit()
#include <linux/module.h>      // THIS_MODULE, module macros
#include <linux/kdev_t.h>      // dev_t type (major/minor)
#include <linux/fs.h>          // file_operations structure
#include <linux/cdev.h>        // cdev structure and APIs
#include <linux/device.h>      // class_create(), device_create()
#include <linux/slab.h>        // Kernel memory allocation
#include <linux/uaccess.h>     // copy_from_user(), copy_to_user()
#include <linux/workqueue.h>   // Workqueue APIs

/* =========================================================
 *           CHARACTER DEVICE VARIABLES
 * ========================================================= */

/* Stores device number (major + minor) */
static dev_t dev;

/* Device class pointer (visible in /sys/class/) */
static struct class *dev_class;

/* Character device structure */
static struct cdev etx_cdev;

/* Kernel buffer for read/write operations */
static char k_buf[100];

/* =========================================================
 *                  WORKQUEUE VARIABLES
 * ========================================================= */

/* Dedicated workqueue structure */
static struct workqueue_struct *own_workqueue;

/*
 * Workqueue handler function
 * This runs in process context (sleep allowed)
 */
static void workqueue_fn(struct work_struct *work)
{
    printk(KERN_INFO "own_wq: workqueue function executed\n");
}

/* Declare a work item and bind it to handler */
static DECLARE_WORK(work, workqueue_fn);

/* =========================================================
 *               FILE OPERATIONS FUNCTIONS
 * ========================================================= */

/* Called when device file is opened */
static int etx_open(struct inode *inode, struct file *file)
{
    printk(KERN_INFO "Device opened\n");
    return 0;
}

/* Called when device file is closed */
static int etx_release(struct inode *inode, struct file *file)
{
    printk(KERN_INFO "Device closed\n");
    return 0;
}

/*
 * Read operation
 * - Schedules work to workqueue
 * - Copies data from kernel buffer to userspace
 */
static ssize_t etx_read(struct file *filp,
                        char __user *buf,
                        size_t len,
                        loff_t *off)
{
    printk(KERN_INFO "Read called - scheduling workqueue\n");

    /* Schedule workqueue (bottom half) */
    queue_work(own_workqueue, &work);

    /* Copy data to user safely */
    return simple_read_from_buffer(buf, len, off,
                                   k_buf, strlen(k_buf));
}

/*
 * Write operation
 * - Copies data from userspace to kernel buffer
 * - Schedules workqueue
 */
static ssize_t etx_write(struct file *filp,
                         const char __user *buf,
                         size_t len,
                         loff_t *off)
{
    printk(KERN_INFO "Write called - scheduling workqueue\n");

    /* Prevent buffer overflow */
    if (len >= sizeof(k_buf))
        len = sizeof(k_buf) - 1;

    /* Copy data from user space */
    if (copy_from_user(k_buf, buf, len))
        return -EFAULT;

    /* Null-terminate string */
    k_buf[len] = '\0';

    /* Schedule workqueue */
    queue_work(own_workqueue, &work);

    return len;
}

/* =========================================================
 *               FILE OPERATIONS STRUCTURE
 * ========================================================= */

static struct file_operations fops = {
    .owner   = THIS_MODULE,   // Owner of this file ops
    .open    = etx_open,      // Open callback
    .release = etx_release,   // Close callback
    .read    = etx_read,      // Read callback
    .write   = etx_write,     // Write callback
};

/* =========================================================
 *               MODULE INITIALIZATION
 * ========================================================= */

static int __init etx_driver_init(void)
{
    /* Allocate major and minor numbers dynamically */
    if (alloc_chrdev_region(&dev, 0, 1, "etx_device") < 0) {
        printk(KERN_ERR "Failed to allocate device number\n");
        return -1;
    }

    printk(KERN_INFO "Major = %d Minor = %d\n",
           MAJOR(dev), MINOR(dev));

    /* Initialize character device */
    cdev_init(&etx_cdev, &fops);

    /* Add cdev to kernel */
    if (cdev_add(&etx_cdev, dev, 1) < 0) {
        printk(KERN_ERR "Failed to add cdev\n");
        unregister_chrdev_region(dev, 1);
        return -1;
    }

    /*
     * Create device class
     * New API (Linux 6.x):
     * class_create(const char *name)
     */
    dev_class = class_create("etx_class");
    if (IS_ERR(dev_class)) {
        printk(KERN_ERR "Failed to create device class\n");
        cdev_del(&etx_cdev);
        unregister_chrdev_region(dev, 1);
        return PTR_ERR(dev_class);
    }

    /* Create device file: /dev/etx_device */
    if (IS_ERR(device_create(dev_class, NULL,
                             dev, NULL,
                             "etx_device"))) {
        printk(KERN_ERR "Failed to create device\n");
        class_destroy(dev_class);
        cdev_del(&etx_cdev);
        unregister_chrdev_region(dev, 1);
        return -1;
    }

    /*
     * Create dedicated workqueue
     * WQ_UNBOUND allows work to run on any CPU
     */
    own_workqueue = alloc_workqueue("own_wq", WQ_UNBOUND, 0);
    if (!own_workqueue) {
        printk(KERN_ERR "Failed to create workqueue\n");
        device_destroy(dev_class, dev);
        class_destroy(dev_class);
        cdev_del(&etx_cdev);
        unregister_chrdev_region(dev, 1);
        return -ENOMEM;
    }

    printk(KERN_INFO "Workqueue driver loaded successfully\n");
    return 0;
}

/* =========================================================
 *               MODULE CLEANUP
 * ========================================================= */

static void __exit etx_driver_exit(void)
{
    /* Destroy workqueue first */
    destroy_workqueue(own_workqueue);

    /* Remove device file */
    device_destroy(dev_class, dev);

    /* Destroy class */
    class_destroy(dev_class);

    /* Remove cdev */
    cdev_del(&etx_cdev);

    /* Free major/minor numbers */
    unregister_chrdev_region(dev, 1);

    printk(KERN_INFO "Workqueue driver unloaded\n");
}

/* Register init and exit functions */
module_init(etx_driver_init);
module_exit(etx_driver_exit);

/* Module metadata */
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Jupiter");
MODULE_DESCRIPTION("Character Device Driver using Dedicated Workqueue (Bottom Half)");

