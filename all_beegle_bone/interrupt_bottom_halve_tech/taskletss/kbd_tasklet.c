#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <asm/io.h>

#define KBD_IRQ 1

static unsigned char scancode;

/* NEW tasklet API */
static void kbd_tasklet_fn(struct tasklet_struct *t)
{
    printk(KERN_INFO "[TASKLET] Scancode = 0x%x\n", scancode);
}

DECLARE_TASKLET(kbd_tasklet, kbd_tasklet_fn);

/* IRQ handler */
static irqreturn_t kbd_irq_handler(int irq, void *dev_id)
{
    scancode = inb(0x60);
    tasklet_schedule(&kbd_tasklet);
    return IRQ_HANDLED;
}

/* Init */
static int __init kbd_tasklet_init(void)
{
    int ret;

    printk(KERN_INFO "Keyboard TASKLET module loaded\n");

    ret = request_irq(KBD_IRQ, kbd_irq_handler,
                      IRQF_SHARED, "kbd_tasklet", (void *)kbd_irq_handler);

    if (ret)
        printk(KERN_ERR "IRQ request failed\n");

    return ret;
}

/* Exit */
static void __exit kbd_tasklet_exit(void)
{
    free_irq(KBD_IRQ, (void *)kbd_irq_handler);
    tasklet_kill(&kbd_tasklet);
    printk(KERN_INFO "Keyboard TASKLET module unloaded\n");
}

module_init(kbd_tasklet_init);
module_exit(kbd_tasklet_exit);

MODULE_LICENSE("GPL");

