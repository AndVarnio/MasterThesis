#include <linux/module.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

MODULE_INFO(vermagic, VERMAGIC_STRING);

__visible struct module __this_module
__attribute__((section(".gnu.linkonce.this_module"))) = {
	.name = KBUILD_MODNAME,
	.init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
	.exit = cleanup_module,
#endif
	.arch = MODULE_ARCH_INIT,
};

static const struct modversion_info ____versions[]
__used
__attribute__((section("__versions"))) = {
	{ 0x1bfd809d, __VMLINUX_SYMBOL_STR(module_layout) },
	{ 0xd616908a, __VMLINUX_SYMBOL_STR(class_unregister) },
	{ 0x68ebbd2a, __VMLINUX_SYMBOL_STR(device_destroy) },
	{ 0x3abb26b0, __VMLINUX_SYMBOL_STR(ioremap_wc) },
	{ 0x80b1cd59, __VMLINUX_SYMBOL_STR(class_destroy) },
	{ 0x3f32f282, __VMLINUX_SYMBOL_STR(device_create) },
	{ 0x6bc3fbc0, __VMLINUX_SYMBOL_STR(__unregister_chrdev) },
	{ 0xd4d7708e, __VMLINUX_SYMBOL_STR(__class_create) },
	{ 0x17b7ceb0, __VMLINUX_SYMBOL_STR(__register_chrdev) },
	{ 0xf4fa543b, __VMLINUX_SYMBOL_STR(arm_copy_to_user) },
	{ 0x97255bdf, __VMLINUX_SYMBOL_STR(strlen) },
	{ 0x91715312, __VMLINUX_SYMBOL_STR(sprintf) },
	{ 0x5af0a89a, __VMLINUX_SYMBOL_STR(remap_pfn_range) },
	{ 0xefd6cf06, __VMLINUX_SYMBOL_STR(__aeabi_unwind_cpp_pr0) },
	{ 0x27e1a049, __VMLINUX_SYMBOL_STR(printk) },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=";


MODULE_INFO(srcversion, "6CFF8F3235A933A835EF01B");
