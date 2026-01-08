#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/string.h>
#include <linux/fcntl.h>
#include <linux/kernel.h>

#define DEVICE_NAME "myadc"
#define DEFAULT_CHANNEL 0
#define SYSFS_PATH_LEN 128
#define READ_BUF_LEN 64

static int channel = DEFAULT_CHANNEL;
module_param(channel, int, 0444);
MODULE_PARM_DESC(channel, "ADC channel to read (0-6)");

static dev_t devt;
static struct cdev myadc_cdev;
static struct class *myadc_class;
static char sysfs_path[SYSFS_PATH_LEN];

/* open */
static int myadc_open(struct inode *inode, struct file *file)
{
    return 0;
}

/* release */
static int myadc_release(struct inode *inode, struct file *file)
{
    return 0;
}

/* read */
static ssize_t myadc_read(struct file *filep, char __user *user_buf,
                          size_t count, loff_t *ppos)
{
    struct file *f;
    loff_t pos = 0;
    char kbuf[READ_BUF_LEN];
    int val, len;
    ssize_t r;

    if (*ppos > 0)
        return 0;   // EOF behavior

    f = filp_open(sysfs_path, O_RDONLY, 0);
    if (IS_ERR(f))
        return PTR_ERR(f);

    memset(kbuf, 0, sizeof(kbuf));

    r = kernel_read(f, kbuf, sizeof(kbuf) - 1, &pos);
    filp_close(f, NULL);

    if (r < 0)
        return r;

    sscanf(kbuf, "%d", &val);
    len = snprintf(kbuf, sizeof(kbuf), "%d\n", val);

    if (copy_to_user(user_buf, kbuf, len))
        return -EFAULT;

    *ppos = len;
    return len;
}

static const struct file_operations myadc_fops = {
    .owner   = THIS_MODULE,
    .open    = myadc_open,
    .release = myadc_release,
    .read    = myadc_read,
};

static int __init myadc_init(void)
{
    int ret;

    snprintf(sysfs_path, sizeof(sysfs_path),
             "/sys/bus/iio/devices/iio:device0/in_voltage%d_raw",
             channel);

    pr_info("myadc: ADC channel = %d\n", channel);
    pr_info("myadc: sysfs path = %s\n", sysfs_path);

    ret = alloc_chrdev_region(&devt, 0, 1, DEVICE_NAME);
    if (ret < 0)
        return ret;

    cdev_init(&myadc_cdev, &myadc_fops);
    ret = cdev_add(&myadc_cdev, devt, 1);
    if (ret)
        goto err_cdev;

    myadc_class = class_create(THIS_MODULE, DEVICE_NAME);
    if (IS_ERR(myadc_class)) {
        ret = PTR_ERR(myadc_class);
        goto err_class;
    }

    device_create(myadc_class, NULL, devt, NULL, DEVICE_NAME);

    pr_info("myadc: driver loaded\n");
    return 0;

err_class:
    cdev_del(&myadc_cdev);
err_cdev:
    unregister_chrdev_region(devt, 1);
    return ret;
}

static void __exit myadc_exit(void)
{
    device_destroy(myadc_class, devt);
    class_destroy(myadc_class);
    cdev_del(&myadc_cdev);
    unregister_chrdev_region(devt, 1);

    pr_info("myadc: driver unloaded\n");
}

MODULE_LICENSE("GPL");



module_init(myadc_init);
module_exit(myadc_exit);

