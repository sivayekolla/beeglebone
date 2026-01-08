#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/workqueue.h>
#include <linux/jiffies.h>

#define IRQ_KBD 1
#define LONG_PRESS (2 * HZ)

static unsigned long press_time;
static int key_pressed;

static struct workqueue_struct *wq;
static struct work_struct work;

/* ---------- BOTTOM HALF ---------- */
void work_fn(struct work_struct *w)
{
    if (key_pressed &&
        time_after(jiffies, press_time + LONG_PRESS))
        printk(KERN_INFO "WORKQUEUE APP: LONG PRESS detected\n");
}

/* ---------- TOP HALF ---------- */
static irqreturn_t isr(int irq, void *dev)
{
    key_pressed = 1;
    press_time = jiffies;

    queue_work(wq, &work);
    return IRQ_HANDLED;
}

static int __init app_init(void)
{
    wq = create_singlethread_workqueue("kbd_wq");
    INIT_WORK(&work, work_fn);

    request_irq(IRQ_KBD, isr, IRQF_SHARED,
                "kbd_wq_app", (void *)isr);

    printk(KERN_INFO "WORKQUEUE APP loaded\n");
    return 0;
}

static void __exit app_exit(void)
{
    free_irq(IRQ_KBD, (void *)isr);
    flush_workqueue(wq);
    destroy_workqueue(wq);

    printk(KERN_INFO "WORKQUEUE APP unloaded\n");
}

module_init(app_init);
module_exit(app_exit);

MODULE_LICENSE("GPL");
