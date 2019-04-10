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
	{ 0x548cb12f, __VMLINUX_SYMBOL_STR(cdev_del) },
	{ 0xd616908a, __VMLINUX_SYMBOL_STR(class_unregister) },
	{ 0x68ebbd2a, __VMLINUX_SYMBOL_STR(device_destroy) },
	{ 0x5dcf6341, __VMLINUX_SYMBOL_STR(outer_cache) },
	{ 0x59d29dab, __VMLINUX_SYMBOL_STR(v7_flush_kern_dcache_area) },
	{ 0x33c07135, __VMLINUX_SYMBOL_STR(remap_pfn_range) },
	{ 0xc65537d0, __VMLINUX_SYMBOL_STR(memremap) },
	{ 0xe2d5255a, __VMLINUX_SYMBOL_STR(strcmp) },
	{ 0x80b1cd59, __VMLINUX_SYMBOL_STR(class_destroy) },
	{ 0x3f32f282, __VMLINUX_SYMBOL_STR(device_create) },
	{ 0x6bc3fbc0, __VMLINUX_SYMBOL_STR(__unregister_chrdev) },
	{ 0xd4d7708e, __VMLINUX_SYMBOL_STR(__class_create) },
	{ 0xddba3b0b, __VMLINUX_SYMBOL_STR(__register_chrdev) },
	{ 0xb960247c, __VMLINUX_SYMBOL_STR(dev_err) },
	{ 0x5620977e, __VMLINUX_SYMBOL_STR(cdev_add) },
	{ 0xce9cc39c, __VMLINUX_SYMBOL_STR(cdev_init) },
	{ 0x61651be, __VMLINUX_SYMBOL_STR(strcat) },
	{ 0x5f754e5a, __VMLINUX_SYMBOL_STR(memset) },
	{ 0x27e1a049, __VMLINUX_SYMBOL_STR(printk) },
	{ 0xefd6cf06, __VMLINUX_SYMBOL_STR(__aeabi_unwind_cpp_pr0) },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=";

