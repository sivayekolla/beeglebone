#include <linux/module.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/irqflags.h>

static int __init my_init(void)
{
	pr_info("module is loaded on processor:%d\n", smp_processor_id());
	local_irq_disable();
	pr_info("interrupts disabled on processor:%d\n", smp_processor_id());
	mdelay(10000L);
	local_irq_enable();
	return 0;
}

static void __exit my_exit(void)
{
}

MODULE_LICENSE("GPL");
module_init(my_init);
module_exit(my_exit);
