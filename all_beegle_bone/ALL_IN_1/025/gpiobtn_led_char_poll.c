#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/wait.h>
#include <linux/poll.h>

#define DEVICE_NAME "gpiobtn"

/* Global variables */
static int press_count = 0;
static int data_ready = 0;
static DECLARE_WAIT_QUEUE_HEAD(gpiobtn_waitq);

/* Device structures */
static dev_t dev_num;
static struct cdev gpiobtn_cdev;
static struct class *gpiobtn_devclass;

/* Device operations */
static int gpiobtn_open(struct inode *inode, struct file *file)
{
    printk(KERN_INFO "gpiobtn: Device opened\n");
    return 0;
}

static int gpiobtn_release(struct inode *inode, struct file *file)
{
    printk(KERN_INFO "gpiobtn: Device closed\n");
    return 0;
}

static ssize_t gpiobtn_read(struct file *file, char __user *buf, 
                           size_t len, loff_t *off)
{
    char msg[64];
    int msg_len;
    
    if (*off > 0) return 0;
    
    /* Wait for data if not ready */
    if (wait_event_interruptible(gpiobtn_waitq, data_ready))
        return -ERESTARTSYS;
    
    /* Create message */
    msg_len = snprintf(msg, sizeof(msg), "Button pressed! Count: %d\n", press_count);
    
    /* Clear flag */
    data_ready = 0;
    
    /* Copy to user */
    if (copy_to_user(buf, msg, msg_len))
        return -EFAULT;
    
    *off = msg_len;
    return msg_len;
}

static ssize_t gpiobtn_write(struct file *file, const char __user *buf,
                            size_t len, loff_t *off)
{
    char cmd;
    
    if (copy_from_user(&cmd, buf, 1))
        return -EFAULT;
    
    /* '1' simulates button press */
    if (cmd == '1') {
        press_count++;
        data_ready = 1;
        wake_up_interruptible(&gpiobtn_waitq);
        printk(KERN_INFO "gpiobtn: Button press simulated\n");
    }
    
    return 1;
}

static __poll_t gpiobtn_poll(struct file *file, poll_table *wait)
{
    poll_wait(file, &gpiobtn_waitq, wait);
    
    if (data_ready)
        return POLLIN | POLLRDNORM;
    
    return 0;
}

static struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = gpiobtn_open,
    .release = gpiobtn_release,
    .read = gpiobtn_read,
    .write = gpiobtn_write,
    .poll = gpiobtn_poll,
};

static int __init gpiobtn_init(void)
{
    int ret;
    
    /* Allocate device number */
    ret = alloc_chrdev_region(&dev_num, 0, 1, DEVICE_NAME);
    if (ret < 0) {
        printk(KERN_ERR "gpiobtn: Failed to allocate device number\n");
        return ret;
    }
    
    /* Initialize character device */
    cdev_init(&gpiobtn_cdev, &fops);
    ret = cdev_add(&gpiobtn_cdev, dev_num, 1);
    if (ret < 0) {
        unregister_chrdev_region(dev_num, 1);
        printk(KERN_ERR "gpiobtn: Failed to add cdev\n");
        return ret;
    }
    
    /* Create device class */
    gpiobtn_devclass = class_create(DEVICE_NAME);
    if (IS_ERR(gpiobtn_devclass)) {
        cdev_del(&gpiobtn_cdev);
        unregister_chrdev_region(dev_num, 1);
        printk(KERN_ERR "gpiobtn: Failed to create class\n");
        return PTR_ERR(gpiobtn_devclass);
    }
    
    /* Create device node */
    device_create(gpiobtn_devclass, NULL, dev_num, NULL, DEVICE_NAME);
    
    printk(KERN_INFO "gpiobtn: Module loaded. Use:\n");
    printk(KERN_INFO "  echo 1 > /dev/gpiobtn  # Simulate press\n");
    printk(KERN_INFO "  cat /dev/gpiobtn       # Wait for press\n");
    
    return 0;
}

static void __exit gpiobtn_exit(void)
{
    device_destroy(gpiobtn_devclass, dev_num);
    class_destroy(gpiobtn_devclass);
    cdev_del(&gpiobtn_cdev);
    unregister_chrdev_region(dev_num, 1);
    
    printk(KERN_INFO "gpiobtn: Module unloaded\n");
}

module_init(gpiobtn_init);
module_exit(gpiobtn_exit);

MODULE_LICENSE("GPL");
