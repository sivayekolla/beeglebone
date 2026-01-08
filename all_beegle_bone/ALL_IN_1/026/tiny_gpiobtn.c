#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/jiffies.h>

#define DEVICE_NAME "gpiobtn"

static int press_count = 0;
static unsigned long last_press = 0;
static int debounce_ms = 50;

static dev_t dev_num;
static struct cdev gpiobtn_cdev;
static struct class *gpiobtn_class;

/* Read: return press count */
static ssize_t gpiobtn_read(struct file *file, char __user *buf, size_t len, loff_t *off)
{
    char msg[32];
    int msg_len;
    
    if (*off > 0) return 0;
    
    msg_len = snprintf(msg, sizeof(msg), "Presses: %d\n", press_count);
    
    if (copy_to_user(buf, msg, msg_len))
        return -EFAULT;
    
    *off = msg_len;
    return msg_len;
}

/* Write: simulate press with debounce */
static ssize_t gpiobtn_write(struct file *file, const char __user *buf, size_t len, loff_t *off)
{
    char cmd;
    unsigned long now = jiffies;
    unsigned long debounce = msecs_to_jiffies(debounce_ms);
    
    if (copy_from_user(&cmd, buf, 1))
        return -EFAULT;
    
    if (cmd == '1') {
        /* Check debounce */
        if (time_before(now, last_press + debounce)) {
            printk("gpiobtn: Debounce ignored\n");
            return len;
        }
        
        last_press = now;
        press_count++;
        printk("gpiobtn: Press %d\n", press_count);
    }
    
    return len;
}

static struct file_operations fops = {
    .owner = THIS_MODULE,
    .read = gpiobtn_read,
    .write = gpiobtn_write,
};

/* Module init */
static int __init gpiobtn_init(void)
{
    int ret;
    
    ret = alloc_chrdev_region(&dev_num, 0, 1, DEVICE_NAME);
    if (ret < 0) return ret;
    
    cdev_init(&gpiobtn_cdev, &fops);
    ret = cdev_add(&gpiobtn_cdev, dev_num, 1);
    if (ret < 0) {
        unregister_chrdev_region(dev_num, 1);
        return ret;
    }
    
    /* Create class with single parameter */
    gpiobtn_class = class_create(DEVICE_NAME);
    if (IS_ERR(gpiobtn_class)) {
        cdev_del(&gpiobtn_cdev);
        unregister_chrdev_region(dev_num, 1);
        return PTR_ERR(gpiobtn_class);
    }
    
    /* Create device node */
    device_create(gpiobtn_class, NULL, dev_num, NULL, DEVICE_NAME);
    
    printk("gpiobtn: Loaded. Use: echo 1 > /dev/gpiobtn\n");
    return 0;
}

/* Module exit */
static void __exit gpiobtn_exit(void)
{
    device_destroy(gpiobtn_class, dev_num);
    class_destroy(gpiobtn_class);
    cdev_del(&gpiobtn_cdev);
    unregister_chrdev_region(dev_num, 1);
    
    printk("gpiobtn: Unloaded\n");
}

module_init(gpiobtn_init);
module_exit(gpiobtn_exit);

MODULE_LICENSE("GPL");
