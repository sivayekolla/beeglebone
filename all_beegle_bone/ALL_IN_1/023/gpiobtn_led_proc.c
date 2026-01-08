#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>

#define PROC_NAME "gpiobtn_info"

/* Runtime state variables */
static int press_count = 0;
static int led_state = 0;
static int last_irq = -1;

/* proc read function */
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

static int __init gpiobtn_init(void)
{
    proc_create(PROC_NAME, 0444, NULL, &gpiobtn_proc_ops);
    pr_info("gpiobtn_led_proc: loaded (procfs only mode)\n");
    return 0;
}

static void __exit gpiobtn_exit(void)
{
    remove_proc_entry(PROC_NAME, NULL);
    pr_info("gpiobtn_led_proc: unloaded\n");
}

module_init(gpiobtn_init);
module_exit(gpiobtn_exit);

MODULE_LICENSE("GPL");
