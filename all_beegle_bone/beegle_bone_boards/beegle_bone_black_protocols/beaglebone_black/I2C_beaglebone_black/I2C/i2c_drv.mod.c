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
	{ 0x9268ea35, "i2c_smbus_read_byte_data" },
	{ 0x8d830b43, "_dev_warn" },
	{ 0x2d500514, "i2c_del_driver" },
	{ 0xbdfb6dbb, "__fentry__" },
	{ 0xf872ba5b, "i2c_register_driver" },
	{ 0x5b8239ca, "__x86_return_thunk" },
	{ 0xe2fd41e5, "module_layout" },
};

MODULE_INFO(depends, "");

MODULE_ALIAS("of:N*T*Cmycompany,my-i2c-test");
MODULE_ALIAS("of:N*T*Cmycompany,my-i2c-testC*");
MODULE_ALIAS("i2c:my-i2c-test");

MODULE_INFO(srcversion, "7B2A3507FE031D7162D05DC");
