#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>

#define PROC_NAME "writer"
#define MAX_LEN 100

static char message[MAX_LEN];

ssize_t writer_read(struct file *file, char __user *buf, size_t count, loff_t *ppos)
{
    return simple_read_from_buffer(buf, count, ppos, message, strlen(message));
}

ssize_t writer_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos)
{
    if (count > MAX_LEN - 1)
        count = MAX_LEN - 1;

    if (copy_from_user(message, buf, count))
        return -EFAULT;

    message[count] = '\0';  // Ensure null termination

    printk(KERN_INFO "Received from user: %s\n", message);
    return count;
}

static const struct proc_ops writer_fops = {
    .proc_read = writer_read,
    .proc_write = writer_write,
};

static int __init proc_write_init(void)
{
    proc_create(PROC_NAME, 0666, NULL, &writer_fops);
    printk(KERN_INFO "/proc/%s created. You can write to it!\n", PROC_NAME);
    return 0;
}

static void __exit proc_write_exit(void)
{
    remove_proc_entry(PROC_NAME, NULL);
    printk(KERN_INFO "/proc/%s removed.\n", PROC_NAME);
}

module_init(proc_write_init);
module_exit(proc_write_exit);


 
MODULE_LICENSE("GPL");
