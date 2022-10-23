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
	{ 0x92d5838e, "request_threaded_irq" },
	{ 0xc31ce947, "gpiod_to_irq" },
	{ 0x51122247, "gpiod_direction_input" },
	{ 0xf9383d23, "gpiod_direction_output" },
	{ 0xf7d5fff7, "gpio_to_desc" },
	{ 0x8360e9, "device_create" },
	{ 0x93c8fa59, "__class_create" },
	{ 0xa472d27f, "__register_chrdev" },
	{ 0x92997ed8, "_printk" },
	{ 0x3dcf1ffa, "__wake_up" },
	{ 0xec523f88, "hrtimer_start_range_ns" },
	{ 0xc4f0da12, "ktime_get_with_offset" },
	{ 0x98a2c4f9, "gpiod_put" },
	{ 0xc1514a3b, "free_irq" },
	{ 0x9d5a8178, "hrtimer_try_to_cancel" },
	{ 0x6bc3fbc0, "__unregister_chrdev" },
	{ 0x5e657d97, "class_destroy" },
	{ 0xde364748, "class_unregister" },
	{ 0xb33af203, "device_destroy" },
	{ 0xb7744793, "kmem_cache_alloc_trace" },
	{ 0xbe301f03, "kmalloc_caches" },
	{ 0x3ea1b6e4, "__stack_chk_fail" },
	{ 0x49970de8, "finish_wait" },
	{ 0x647af474, "prepare_to_wait_event" },
	{ 0x1000e51, "schedule" },
	{ 0xfe487975, "init_wait_entry" },
	{ 0xbb72d4fe, "__put_user_1" },
	{ 0x800473f, "__cond_resched" },
	{ 0x8f678b07, "__stack_chk_guard" },
	{ 0x37a0cba, "kfree" },
	{ 0x9e7b9e2c, "gpiod_set_value" },
	{ 0xb1ad28e0, "__gnu_mcount_nc" },
};

MODULE_INFO(depends, "");


MODULE_INFO(srcversion, "B4596EE66ABDE5B063A137A");
