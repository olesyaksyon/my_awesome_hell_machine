#include <linux/module.h>
#define INCLUDE_VERMAGIC
#include <linux/build-salt.h>
#include <linux/elfnote-lto.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

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
	{ 0x826b8746, "module_layout" },
	{ 0x344281e4, "param_ops_int" },
	{ 0xa362bf8f, "hrtimer_init" },
	{ 0xf9383d23, "gpiod_direction_output" },
	{ 0xf7d5fff7, "gpio_to_desc" },
	{ 0x8360e9, "device_create" },
	{ 0x93c8fa59, "__class_create" },
	{ 0xa472d27f, "__register_chrdev" },
	{ 0xec523f88, "hrtimer_start_range_ns" },
	{ 0x98a2c4f9, "gpiod_put" },
	{ 0x9d5a8178, "hrtimer_try_to_cancel" },
	{ 0x6bc3fbc0, "__unregister_chrdev" },
	{ 0x5e657d97, "class_destroy" },
	{ 0xde364748, "class_unregister" },
	{ 0xb33af203, "device_destroy" },
	{ 0xb7744793, "kmem_cache_alloc_trace" },
	{ 0xbe301f03, "kmalloc_caches" },
	{ 0x37a0cba, "kfree" },
	{ 0x9e7b9e2c, "gpiod_set_value" },
	{ 0x92997ed8, "_printk" },
	{ 0x28118cb6, "__get_user_1" },
	{ 0xbb72d4fe, "__put_user_1" },
	{ 0xb1ad28e0, "__gnu_mcount_nc" },
};

MODULE_INFO(depends, "");


MODULE_INFO(srcversion, "4ADC163AD29C556C005408B");
