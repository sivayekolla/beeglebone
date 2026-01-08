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
	{ 0x6079cf62, "_dev_info" },
	{ 0xa193b721, "devm_kmalloc" },
	{ 0x63c0221b, "spi_setup" },
	{ 0xa85d4575, "spi_write_then_read" },
	{ 0x8d830b43, "_dev_warn" },
	{ 0xfe15742c, "_dev_err" },
	{ 0xf0fdf6cb, "__stack_chk_fail" },
	{ 0x3a7e85b0, "driver_unregister" },
	{ 0xbdfb6dbb, "__fentry__" },
	{ 0xaee04e10, "__spi_register_driver" },
	{ 0x5b8239ca, "__x86_return_thunk" },
	{ 0xe2fd41e5, "module_layout" },
};

MODULE_INFO(depends, "");

MODULE_ALIAS("of:N*T*Cmycompany,my-spi-test");
MODULE_ALIAS("of:N*T*Cmycompany,my-spi-testC*");
MODULE_ALIAS("spi:my-spi-test");

MODULE_INFO(srcversion, "512AD4CFD9034C16FC4BC95");
