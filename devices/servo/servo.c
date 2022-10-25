#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/ktime.h>
#include <linux/hrtimer.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/string.h>
#include "servo.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Alexey 'Cluster' Avdyukhin");
MODULE_DESCRIPTION("Servo driver");
MODULE_VERSION("1.0");

static int pin = -1;
static int value = 0;

module_param(pin, int, S_IRUGO);
MODULE_PARM_DESC(pin,"Servo pin number");
module_param(value, int, 0660);
MODULE_PARM_DESC(value,"Servo value");

static int majorNumber = -1;
static struct class*  servo_class  = NULL;
static struct device* rc_device = NULL;

static struct hrtimer* tx_low_timer = NULL;
static struct hrtimer* tx_high_timer = NULL;
static struct gpio_desc* pin_desc = NULL;
static DECLARE_WAIT_QUEUE_HEAD(rx_wq);

// The prototype functions for the character driver -- must come before the struct definition
static int     dev_open(struct inode *, struct file *);
static int     dev_release(struct inode *, struct file *);
static ssize_t dev_write(struct file *, const char *, size_t, loff_t *);
// static loff_t  dev_llseek(struct file *file,loff_t offset, int orig);
 
static struct file_operations fops =
{
   .open = dev_open,
   .read = 0,
   .write = dev_write,
   .release = dev_release,
   .llseek = 0
};

/* TX high timer callback */
static enum hrtimer_restart tx_high_callback(struct hrtimer *timer)
{
    if (value) gpiod_set_value(pin_desc, 1);
    hrtimer_try_to_cancel(tx_low_timer);
    if (value) hrtimer_start(tx_low_timer, ktime_set(0, value * 1000UL), HRTIMER_MODE_REL);
    hrtimer_start(tx_high_timer, ktime_set(0, PERIOD * 1000UL), HRTIMER_MODE_REL);
    return HRTIMER_NORESTART;
}

/* TX low timer callback */
static enum hrtimer_restart tx_low_callback(struct hrtimer *timer)
{
    gpiod_set_value(pin_desc, 0);
    value = 0;
    return HRTIMER_NORESTART;
}

/* Function to free all resources */
static void servo_free(void)
{
    if (!IS_ERR_OR_NULL(servo_class)) {
        // remove the device
        device_destroy(servo_class, MKDEV(majorNumber, DEV_MINOR));
        // unregister the device class
        class_unregister(servo_class);
        // remove the device class
        class_destroy(servo_class);
    }
    // unregister the major number
    if (majorNumber >= 0) {
        unregister_chrdev(majorNumber, DEVICE_BUS);
    }

    if (tx_low_timer) {
        hrtimer_try_to_cancel(tx_low_timer);
        kfree(tx_low_timer);
    }

    if (tx_high_timer) {
        hrtimer_try_to_cancel(tx_high_timer);
        kfree(tx_high_timer);
    }

    // free pin
    if (!IS_ERR_OR_NULL(pin_desc))
        gpiod_put(pin_desc);
}

/* Function to init the module */
static __init int servo_init(void)
{
    if (pin < 0) {
        printk(KERN_ERR "servo: You must specify servo pin\n");
        return -EINVAL;
    }

    // register character device and request major number
    majorNumber = register_chrdev(0, DEVICE_BUS, &fops);
    if (majorNumber < 0) {
        printk(KERN_ERR "servo: failed to register a major number\n");
        servo_free();
        return -EPERM;
    }
    // register the device class
    servo_class = class_create(THIS_MODULE, CLASS_NAME);
    if (IS_ERR(servo_class)) {
        printk(KERN_ERR "servo: failed to register device class: %ld\n", PTR_ERR(servo_class));
        servo_free();
        return -EPERM;
    }
    // register the device driver
    rc_device = device_create(servo_class, NULL, MKDEV(majorNumber, DEV_MINOR), NULL, DEVICE_NAME);
    if (IS_ERR(rc_device)) {
        printk(KERN_ERR "servo: failed to create the TX device: %ld\n", PTR_ERR(rc_device));
        servo_free();
        return -EPERM;
    }

    // prepare pin for the receiver
    pin_desc = gpio_to_desc(pin);
    if (IS_ERR(pin_desc)) {
        printk(KERN_ERR "servo: pin gpiod_request error: %ld\n", PTR_ERR(pin_desc));
        servo_free();
        return -EPERM;
    }
    // output
    gpiod_direction_output(pin_desc, 0);

    // allocate and init timers
    tx_low_timer = kzalloc(sizeof(struct hrtimer), GFP_KERNEL);
    if (!tx_low_timer) {
        printk(KERN_ERR "servo: can't allocate memory for timer\n");
        servo_free();
        return -ENOMEM;
    }
    hrtimer_init(tx_low_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
    tx_low_timer->function = tx_low_callback;
    // and second one
    tx_high_timer = kzalloc(sizeof(struct hrtimer), GFP_KERNEL);
    if (!tx_low_timer) {
        printk(KERN_ERR "servo: can't allocate memory for timer\n");
        servo_free();
        return -ENOMEM;
    }
    hrtimer_init(tx_high_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
    tx_high_timer->function = tx_high_callback;
    hrtimer_start(tx_high_timer, ktime_set(0, PERIOD * 1000UL), HRTIMER_MODE_REL);

    printk(KERN_INFO "servo: driver started\n");
    return 0;
}

/* Function to unload module */
static void __exit servo_exit(void)
{
    servo_free();
    printk(KERN_INFO "servo: driver stopped\n");
}

static int dev_open(struct inode *inodep, struct file *filep)
{
    servo_file_t *sf;
    filep->private_data = kzalloc(sizeof(servo_file_t), GFP_KERNEL);
    if (!filep->private_data)
        return -ENOMEM;
    sf = (servo_file_t*)filep->private_data;
    sf->read_pos = 0;
    return 0;
}


static ssize_t dev_write(struct file *filep, const char *buffer, size_t len, loff_t *offset)
{
    ssize_t r;
    char c;
    servo_file_t *sf = (servo_file_t*)filep->private_data;

    for (r = 0; r < len; r++, buffer++) {
        if (get_user(c, buffer) || r > 4) {
            return -EFAULT;
        }
        sf->buffer[sf->read_pos] = c;
        sf->read_pos += 1;
    }
    if (r == 4) sf->read_pos = 0;
    value = *(u32 *)sf->buffer;
    printk(KERN_INFO "value changed to %d", value);
    return r;
}

/*static ssize_t dev_write(struct file *filep, const char *buffer, size_t len, loff_t *offset)
{
    ssize_t r;
    char c;
    servo_file_t *sf = (servo_file_t*)filep->private_data;

    for (r = 0; r < len; r++, buffer++) {
        if (get_user(c, buffer) || r > 4) {
            return -EFAULT;
        }
        sf->buffer[r] = c;
    }
    value = *(u32 *)sf->buffer;
    printk(KERN_INFO "value changed to %d", value);
    return r;
}*/

static int dev_release(struct inode *inodep, struct file *filep)
{
    kfree(filep->private_data);
    return 0;
}

module_init(servo_init);
module_exit(servo_exit);
