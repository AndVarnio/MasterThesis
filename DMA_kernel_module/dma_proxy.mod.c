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
	{ 0x19fdf3e6, __VMLINUX_SYMBOL_STR(param_ops_int) },
	{ 0x37a0cba, __VMLINUX_SYMBOL_STR(kfree) },
	{ 0xab6ec41f, __VMLINUX_SYMBOL_STR(dma_release_channel) },
	{ 0x68ebbd2a, __VMLINUX_SYMBOL_STR(device_destroy) },
	{ 0x2ccd96a5, __VMLINUX_SYMBOL_STR(mem_map) },
	{ 0xb717ec16, __VMLINUX_SYMBOL_STR(arm_dma_ops) },
	{ 0x7cf9099, __VMLINUX_SYMBOL_STR(wait_for_completion_timeout) },
	{ 0x275ef902, __VMLINUX_SYMBOL_STR(__init_waitqueue_head) },
	{ 0xf88c3301, __VMLINUX_SYMBOL_STR(sg_init_table) },
	{ 0x5d85bc21, __VMLINUX_SYMBOL_STR(kmalloc_caches) },
	{ 0xf92e10c0, __VMLINUX_SYMBOL_STR(dmam_alloc_coherent) },
	{ 0xc158634, __VMLINUX_SYMBOL_STR(dma_supported) },
	{ 0x80b1cd59, __VMLINUX_SYMBOL_STR(class_destroy) },
	{ 0x3f32f282, __VMLINUX_SYMBOL_STR(device_create) },
	{ 0x61651be, __VMLINUX_SYMBOL_STR(strcat) },
	{ 0x548cb12f, __VMLINUX_SYMBOL_STR(cdev_del) },
	{ 0xd4d7708e, __VMLINUX_SYMBOL_STR(__class_create) },
	{ 0x27e1a049, __VMLINUX_SYMBOL_STR(printk) },
	{ 0x14d44f8c, __VMLINUX_SYMBOL_STR(kmem_cache_alloc) },
	{ 0x7485e15e, __VMLINUX_SYMBOL_STR(unregister_chrdev_region) },
	{ 0x5620977e, __VMLINUX_SYMBOL_STR(cdev_add) },
	{ 0xce9cc39c, __VMLINUX_SYMBOL_STR(cdev_init) },
	{ 0xb960247c, __VMLINUX_SYMBOL_STR(dev_err) },
	{ 0x29537c9e, __VMLINUX_SYMBOL_STR(alloc_chrdev_region) },
	{ 0x5f754e5a, __VMLINUX_SYMBOL_STR(memset) },
	{ 0xbe1a8a91, __VMLINUX_SYMBOL_STR(__dma_request_channel) },
	{ 0x676bbc0f, __VMLINUX_SYMBOL_STR(_set_bit) },
	{ 0xd4669fad, __VMLINUX_SYMBOL_STR(complete) },
	{ 0xd6155279, __VMLINUX_SYMBOL_STR(dma_common_mmap) },
	{ 0x33c07135, __VMLINUX_SYMBOL_STR(remap_pfn_range) },
	{ 0xefd6cf06, __VMLINUX_SYMBOL_STR(__aeabi_unwind_cpp_pr0) },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=";

