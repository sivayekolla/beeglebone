#include <linux/module.h>
#define INCLUDE_VERMAGIC
#include <linux/build-salt.h>
#include <linux/elfnote-lto.h>
#include <linux/export-internal.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

#ifdef CONFIG_UNWINDER_ORC
#include <asm/orc_header.h>
ORC_HEADER;
#endif

BUILD_SALT;
BUILD_LTO_INFO;

MODULE_INFO(vermagic, VERMAGIC_STRING);
MODULE_INFO(name, KBUILD_MODNAME);

__visible struct module __this_module
__section(".gnu.linkonce.this_module") = {
	.name = KBUILD_MODNAME,
	.init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
	.exit = cleanup_module,
#endif
	.arch = MODULE_ARCH_INIT,
};

#ifdef CONFIG_RETPOLINE
MODULE_INFO(retpoline, "Y");
#endif



static const struct modversion_info ____versions[]
__used __section("__versions") = {
	{ 0xc892ac3e, "cdev_del" },
	{ 0x6091b333, "unregister_chrdev_region" },
	{ 0x622e0173, "gpiod_put" },
	{ 0x13c49cc2, "_copy_from_user" },
	{ 0xc0c32ae5, "gpiod_set_value" },
	{ 0xf0fdf6cb, "__stack_chk_fail" },
	{ 0x5207f8c, "gpiod_get" },
	{ 0x61d82fc7, "gpiod_direction_output" },
	{ 0x825c5e7a, "gpiod_direction_input" },
	{ 0xe3ec2f2b, "alloc_chrdev_region" },
	{ 0x858c69be, "cdev_init" },
	{ 0x6b732375, "cdev_add" },
	{ 0x6ca9b86a, "class_create" },
	{ 0x3b69de06, "device_create" },
	{ 0x6079cf62, "_dev_info" },
	{ 0xfe15742c, "_dev_err" },
	{ 0xee934b24, "gpiod_get_value" },
	{ 0x3c3ff9fd, "sprintf" },
	{ 0xa916b694, "strnlen" },
	{ 0x88db9f48, "__check_object_size" },
	{ 0x6b10bee1, "_copy_to_user" },
	{ 0xcbd4898c, "fortify_panic" },
	{ 0x4a77885d, "platform_driver_unregister" },
	{ 0xbdfb6dbb, "__fentry__" },
	{ 0x122c3a7e, "_printk" },
	{ 0x5b8239ca, "__x86_return_thunk" },
	{ 0x23509fba, "__platform_driver_register" },
	{ 0x5b40b481, "device_destroy" },
	{ 0x75646747, "class_destroy" },
	{ 0xe2fd41e5, "module_layout" },
};

MODULE_INFO(depends, "");

MODULE_ALIAS("of:N*T*Cbbb-gpio-driver");
MODULE_ALIAS("of:N*T*Cbbb-gpio-driverC*");

MODULE_INFO(srcversion, "FC7DEA4FE152BA2F07421CC");
