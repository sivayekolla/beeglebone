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
MODULE_PARM_DESC(channel, "ADC channel to read (0-7)");

static dev_t devt;
static struct cdev myadc_cdev;
static struct class *myadc_class;
static char sysfs_path[SYSFS_PATH_LEN];

/* Device open */
static int myadc_open(struct inode *inode, struct file *file)
{
    return 0;
}

/* Device release */
static int myadc_release(struct inode *inode, struct file *file)
{
    return 0;
}

/* Device read */
static ssize_t myadc_read(struct file *filep, char __user *user_buf,
                          size_t count, loff_t *ppos)
{
    struct file *f = NULL;
    loff_t pos = 0;
    char kbuf[READ_BUF_LEN];
    ssize_t r;
    int val;
    int len;

    /* Open the sysfs ADC file */
    f = filp_open(sysfs_path, O_RDONLY, 0);
   

    memset(kbuf, 0, sizeof(kbuf));

    /* Read raw ADC value */
    r = kernel_read(f, kbuf, sizeof(kbuf) - 1, &pos);
    filp_close(f, NULL);

     /* Extract integer value */
    sscanf(kbuf, "%d", &val);
    /* Prepare output buffer */
    len = snprintf(kbuf, sizeof(kbuf), "%d\n", val);

  
    /* Copy to user space */
    if (copy_to_user(user_buf, kbuf, len))
        return -EFAULT;

    /* Reset file offset for repeated reads */
    *ppos = 0;

    return len;
}

/* File operations structure */
static const struct file_operations myadc_fops = {
    .owner = THIS_MODULE,
    .open = myadc_open,
    .release = myadc_release,
    .read = myadc_read,
};

/* Module initialization */
static int __init myadc_init(void)
{
    int ret;

    snprintf(sysfs_path, sizeof(sysfs_path),"/sys/bus/iio/devices/iio:device0/in_voltage%d_raw",channel);

    pr_info("myadc: using ADC channel %d\n", channel);
    pr_info("myadc: sysfs path = %s\n", sysfs_path);

    /* Allocate device number */
    ret = alloc_chrdev_region(&devt, 0, 1, DEVICE_NAME);
  

    cdev_init(&myadc_cdev, &myadc_fops);
    myadc_cdev.owner = THIS_MODULE;

    ret = cdev_add(&myadc_cdev, devt, 1);
    

    /* Create device class */
    myadc_class = class_create(THIS_MODULE, DEVICE_NAME);
  

    /* Create device */
    device_create(myadc_class, NULL, devt, NULL, DEVICE_NAME);

    pr_info("myadc driver loaded successfully\n");
    return 0;
}

/* Module exit */
static void __exit myadc_exit(void)
{
    device_destroy(myadc_class, devt);
    class_destroy(myadc_class);
    cdev_del(&myadc_cdev);
    unregister_chrdev_region(devt, 1);

    pr_info("myadc driver unloaded\n");
}

MODULE_LICENSE("GPL");

module_init(myadc_init);
module_exit(myadc_exit);
