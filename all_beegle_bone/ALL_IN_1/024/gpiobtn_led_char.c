#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/ioctl.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>

#define DEVICE_NAME "gpiobtn"
#define CLASS_NAME  "gpiobtnclass"

/* IOCTL commands */
#define GPIOBTN_MAGIC 'G'
#define GPIOBTN_RESET _IO(GPIOBTN_MAGIC, 0)

/* Runtime variables */
static int press_count = 0;
static int led_state = 0;
static int last_irq = -1;

/* Character device structures */
static dev_t dev_num;
static struct cdev gpiobtn_cdev;
static struct class *gpiobtn_devclass;
static struct device *gpiobtn_device;

/* ---------------- Procfs Section ---------------- */
#define PROC_NAME "gpiobtn_info"

static int gpiobtn_proc_show(struct seq_file *m, void *v)
{
    seq_printf(m, "GPIO Button Driver Status\n");
    seq_printf(m, "-------------------------\n");
    seq_printf(m, "press_count : %d\n", press_count);
    seq_printf(m, "led_state   : %d\n", led_state);
    seq_printf(m, "last_irq    : %d\n", last_irq);
    return 0;
}

static int gpiobtn_proc_open(struct inode *inode, struct file *file)
{
    return single_open(file, gpiobtn_proc_show, NULL);
}

static const struct proc_ops gpiobtn_proc_ops = {
    .proc_open    = gpiobtn_proc_open,
    .proc_read    = seq_read,
    .proc_lseek   = seq_lseek,
    .proc_release = single_release,
};

/* ---------------- Character Device Section ---------------- */
static int gpiobtn_open(struct inode *inode, struct file *file)
{
    pr_info("[gpiobtn] Device opened\n");
    return 0;
}

static int gpiobtn_release(struct inode *inode, struct file *file)
{
    pr_info("[gpiobtn] Device closed\n");
    return 0;
}

static ssize_t gpiobtn_read(struct file *file, char __user *buf, size_t len, loff_t *off)
{
    char msg[64];
    int msg_len;

    msg_len = snprintf(msg, sizeof(msg), "Press Count: %d\n", press_count);

    if (*off >= msg_len)
        return 0;

    if (copy_to_user(buf, msg, msg_len))
        return -EFAULT;

    *off += msg_len;
    return msg_len;
}

static ssize_t gpiobtn_write(struct file *file, const char __user *buf, size_t len, loff_t *off)
{
    char input[8];

    if (len > sizeof(input) - 1)
        len = sizeof(input) - 1;

    if (copy_from_user(input, buf, len))
        return -EFAULT;

    input[len] = '\0';

    if (input[0] == '1') {
        led_state = !led_state;
        pr_info("[gpiobtn] LED toggled via write(): %s\n", led_state ? "ON" : "OFF");
        /* On BeagleBone: gpiod_set_value(led_gpiod, led_state); */
    }

    return len;
}

static long gpiobtn_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    switch (cmd) {
        case GPIOBTN_RESET:
            press_count = 0;
            pr_info("[gpiobtn] Counter reset via ioctl()\n");
            break;
        default:
            return -EINVAL;
    }
    return 0;
}

static struct file_operations fops = {
    .owner          = THIS_MODULE,
    .open           = gpiobtn_open,
    .release        = gpiobtn_release,
    .read           = gpiobtn_read,
    .write          = gpiobtn_write,
    .unlocked_ioctl = gpiobtn_ioctl,
};

/* ---------------- Module Init/Exit ---------------- */
static int __init gpiobtn_init(void)
{
    int ret;

    /* Create procfs entry */
    if (!proc_create(PROC_NAME, 0444, NULL, &gpiobtn_proc_ops)) {
        pr_err("Failed to create procfs entry\n");
        return -ENOMEM;
    }

    /* Allocate major/minor numbers */
    ret = alloc_chrdev_region(&dev_num, 0, 1, DEVICE_NAME);
    if (ret < 0) {
        pr_err("Failed to allocate char device number\n");
        remove_proc_entry(PROC_NAME, NULL);
        return ret;
    }

    /* Initialize cdev */
    cdev_init(&gpiobtn_cdev, &fops);
    ret = cdev_add(&gpiobtn_cdev, dev_num, 1);
    if (ret < 0) {
        pr_err("Failed to add cdev\n");
        unregister_chrdev_region(dev_num, 1);
        remove_proc_entry(PROC_NAME, NULL);
        return ret;
    }

    /* Create device class - FIXED: Removed THIS_MODULE parameter */
    gpiobtn_devclass = class_create(CLASS_NAME);
    if (IS_ERR(gpiobtn_devclass)) {
        pr_err("Failed to create device class\n");
        cdev_del(&gpiobtn_cdev);
        unregister_chrdev_region(dev_num, 1);
        remove_proc_entry(PROC_NAME, NULL);
        return PTR_ERR(gpiobtn_devclass);
    }

    /* Create device node /dev/gpiobtn */
    gpiobtn_device = device_create(gpiobtn_devclass, NULL, dev_num, NULL, DEVICE_NAME);
    if (IS_ERR(gpiobtn_device)) {
        pr_err("Failed to create device node\n");
        class_destroy(gpiobtn_devclass);
        cdev_del(&gpiobtn_cdev);
        unregister_chrdev_region(dev_num, 1);
        remove_proc_entry(PROC_NAME, NULL);
        return PTR_ERR(gpiobtn_device);
    }

    pr_info("[gpiobtn] Module loaded successfully\n");
    pr_info("[gpiobtn] Device available at: /dev/%s\n", DEVICE_NAME);
    pr_info("[gpiobtn] Procfs info at: /proc/%s\n", PROC_NAME);
    return 0;
}

static void __exit gpiobtn_exit(void)
{
    device_destroy(gpiobtn_devclass, dev_num);
    class_destroy(gpiobtn_devclass);
    cdev_del(&gpiobtn_cdev);
    unregister_chrdev_region(dev_num, 1);

    remove_proc_entry(PROC_NAME, NULL);

    pr_info("[gpiobtn] Module unloaded\n");
}

module_init(gpiobtn_init);
module_exit(gpiobtn_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Shiva");
MODULE_DESCRIPTION("GPIO Button LED Character Driver (PC Safe)");
