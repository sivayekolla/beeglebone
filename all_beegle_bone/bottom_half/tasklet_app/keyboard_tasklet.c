#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <linux/jiffies.h>
#include <linux/spinlock.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <asm/io.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Jupiter");
MODULE_DESCRIPTION("Keyboard double press detection using tasklet + procfs");

#define KBD_DATA_REG  0x60
#define KBD_RELEASE   0x80
#define IRQ_KBD       1

#define PROC_NAME "kbd_tasklet_event"
#define DOUBLE_MS 150

static int dev_id = 0xAB;

/* Scancode → ASCII */
static const unsigned char kbdus[128] = {
    0,27,'1','2','3','4','5','6','7','8',
    '9','0','-','=','\b','\t','q','w','e','r',
    't','y','u','i','o','p','[',']','\n',0,
    'a','s','d','f','g','h','j','k','l',';',
    '\'','`',0,'\\','z','x','c','v','b','n',
    'm',',','.','/',0,'*',0,' ',0
};

/* ---------- shared data ---------- */
static char proc_buf[128];
static int proc_len;
static DEFINE_SPINLOCK(proc_lock);

/* double press state */
static char prev_key;
static unsigned long prev_time;

/* ---------- PROCFS READ ---------- */
static ssize_t proc_read(struct file *file,
                         char __user *ubuf,
                         size_t count,
                         loff_t *ppos)
{
    int ret;

    if (*ppos > 0)
        return 0;

    spin_lock(&proc_lock);
    ret = copy_to_user(ubuf, proc_buf, proc_len);
    spin_unlock(&proc_lock);

    if (ret)
        return -EFAULT;

    *ppos = proc_len;
    return proc_len;
}

static const struct proc_ops proc_fops = {
    .proc_read = proc_read,
};

/* ---------- TASKLET ---------- */
static void kbd_tasklet_fn(struct tasklet_struct *t);

DECLARE_TASKLET(kbd_tasklet, kbd_tasklet_fn);

static void kbd_tasklet_fn(struct tasklet_struct *t)
{
    unsigned long now = jiffies;
    char msg[64];

    if (prev_key) {
        if (time_before(now, prev_time + msecs_to_jiffies(DOUBLE_MS))) {
            snprintf(msg, sizeof(msg),
                     "DOUBLE PRESS: %c\n", prev_key);
        } else {
            snprintf(msg, sizeof(msg),
                     "KEY PRESS: %c\n", prev_key);
        }

        spin_lock(&proc_lock);
        proc_len = snprintf(proc_buf, sizeof(proc_buf), "%s", msg);
        spin_unlock(&proc_lock);

        prev_key = 0;
    }
}

/* ---------- IRQ HANDLER ---------- */
static irqreturn_t kbd_irq(int irq, void *dev)
{
    unsigned char sc;
    char ch;

    sc = inb(KBD_DATA_REG);

    if (sc & KBD_RELEASE)
        return IRQ_HANDLED;

    ch = kbdus[sc];
    if (!ch)
        return IRQ_HANDLED;

    if (!prev_key) {
        prev_key = ch;
        prev_time = jiffies;
    }

    tasklet_schedule(&kbd_tasklet);
    return IRQ_HANDLED;
}

/* ---------- INIT / EXIT ---------- */
static int __init kbd_init(void)
{
    proc_create(PROC_NAME, 0444, NULL, &proc_fops);

    if (request_irq(IRQ_KBD, kbd_irq,
                    IRQF_SHARED,
                    "kbd_tasklet",
                    &dev_id)) {
        pr_err("IRQ request failed\n");
        return -1;
    }

    pr_info("kbd_tasklet loaded\n");
    return 0;
}

static void __exit kbd_exit(void)
{
    tasklet_kill(&kbd_tasklet);
    free_irq(IRQ_KBD, &dev_id);
    remove_proc_entry(PROC_NAME, NULL);
    pr_info("kbd_tasklet unloaded\n");
}

module_init(kbd_init);
module_exit(kbd_exit);

