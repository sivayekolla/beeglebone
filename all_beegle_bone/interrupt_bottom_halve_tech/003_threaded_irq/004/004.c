#include <linux/module.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/sched.h>
#include <linux/irqflags.h>

#define SHARED_IRQ 1
static int irq = SHARED_IRQ, my_dev_id, irq_counter = 0;
module_param(irq, int, S_IRUGO);

void is_irq_disabled(void)
{
        if (irqs_disabled())
                pr_info("IRQ Disabled\n");
        else
                pr_info("IRQ Enabled\n");
}


static irqreturn_t my_interrupt(int irq, void *dev_id)
{
	is_irq_disabled();
	return IRQ_WAKE_THREAD;
}

static irqreturn_t my_threaded_interrupt(int irq, void *dev_id)
{
	pr_info("COMM:%s\t PID:%d\n", current->comm, current->pid);
	is_irq_disabled();
	return IRQ_NONE;
}

static int __init my_init(void)
{
	int ret;
	ret =  (request_threaded_irq(irq, 
			my_interrupt, my_threaded_interrupt,
			IRQF_SHARED, "my_interrupt", &my_dev_id)); 
	
	if (ret != 0)
	{
	
		pr_info("Failed to reserve irq %d, ret:%d\n", irq, ret);
		return -1;
	}
	pr_info("Successfully loading ISR handler\n");
	return 0;
}

static void __exit my_exit(void)
{
	synchronize_irq(irq);
	free_irq(irq, &my_dev_id);
	pr_info("Successfully unloading,  irq_counter = %d\n", irq_counter);
}

MODULE_LICENSE("GPL");
module_init(my_init);
module_exit(my_exit);
