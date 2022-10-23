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
#include "sonar.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("olesyaksyon");
MODULE_DESCRIPTION("Servo driver");
MODULE_VERSION("1.0");

static int trig_pin = -1;
static int echo_pin = -1;
static int trig_interval = 100000;
static int trig_duration = 2;

module_param(trig_pin, int, S_IRUGO);
MODULE_PARM_DESC(trig_pin,"Trigger pin number");
module_param(echo_pin, int, S_IRUGO);
MODULE_PARM_DESC(echo_pin,"Echo pin number");
module_param(trig_interval, int, 0660);
MODULE_PARM_DESC(trig_interval,"Servo trigger interval (usec)");
module_param(trig_duration, int, 0660);
MODULE_PARM_DESC(trig_duration,"Servo trigger duration (usec)");

static int majorNumber = -1;
static struct class*  sonar_class  = NULL;
static struct device* sonar_device = NULL;

static struct hrtimer* tx_low_timer = NULL;
static struct hrtimer* tx_high_timer = NULL;
static struct gpio_desc* trig_pin_desc = NULL;
static struct gpio_desc* echo_pin_desc = NULL;
static u32 rx_irq_number = 0xffff;
static DECLARE_WAIT_QUEUE_HEAD(rx_wq);

// The prototype functions for the character driver -- must come before the struct definition
static int     dev_open(struct inode *, struct file *);
static int     dev_release(struct inode *, struct file *);
static ssize_t dev_read(struct file *, char *, size_t, loff_t *);
 
static u64 trig_time = 0;
static int bus_number_opens = 0;
static struct file* opened_files[MAX_OPENED_FILES];
static u64 echo_delay = 0;

static struct file_operations fops =
{
   .open = dev_open,
   .read = dev_read,
   .write = 0,
   .release = dev_release,
   .llseek = 0
};

/* Trigger high timer callback */
static enum hrtimer_restart tx_high_callback(struct hrtimer *timer)
{
    trig_time = ktime_to_us(ktime_get_boottime());
    if (trig_duration) gpiod_set_value(trig_pin_desc, 0);
    hrtimer_try_to_cancel(tx_low_timer);
    if (trig_duration) hrtimer_start(tx_low_timer, ktime_set(0, trig_duration * 1000UL), HRTIMER_MODE_REL);
    hrtimer_start(tx_high_timer, ktime_set(0, trig_interval * 1000UL), HRTIMER_MODE_REL);
    return HRTIMER_NORESTART;
}

/* Trigger low timer callback */
static enum hrtimer_restart tx_low_callback(struct hrtimer *timer)
{
    gpiod_set_value(trig_pin_desc, 1);
    return HRTIMER_NORESTART;
}

/* IRQ fired every falling edge of echo pin */
static irq_handler_t echo_irq_handler(unsigned int irq,
    void *dev_id, struct pt_regs *regs)
{
    int i;
    u64 now = ktime_to_us(ktime_get_boottime());
    if (trig_time) {
        echo_delay = now - trig_time;
        // notify clients
        for (i = 0; i < bus_number_opens; i++) {
            sonar_file_t* rcf = (sonar_file_t*)opened_files[i]->private_data;
            rcf->read_pending = 1;
            wake_up_interruptible(&rx_wq);
        }
    }

    return (irq_handler_t) IRQ_HANDLED;
}

/* Function to free all resources */
static void sonar_free(void)
{
    if (!IS_ERR_OR_NULL(sonar_class)) {
        // remove the device
        device_destroy(sonar_class, MKDEV(majorNumber, DEV_MINOR));
        // unregister the device class
        class_unregister(sonar_class);
        // remove the device class
        class_destroy(sonar_class);
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

    // free IRQ
    if (rx_irq_number != 0xffff)
        free_irq(rx_irq_number, NULL);

    // free pins
    if (!IS_ERR_OR_NULL(trig_pin_desc))
        gpiod_put(trig_pin_desc);
    if (!IS_ERR_OR_NULL(echo_pin_desc))
        gpiod_put(echo_pin_desc);
}

/* Function to init the module */
static __init int sonar_init(void)
{
    int r;

    if (trig_pin < 0) {
        printk(KERN_ERR "sonar: You must specify sonar trigger pin\n");
        return -EINVAL;
    }

    if (echo_pin < 0) {
        printk(KERN_ERR "sonar: You must specify sonar echo pin\n");
        return -EINVAL;
    }

    // register character device and request major number
    majorNumber = register_chrdev(0, DEVICE_BUS, &fops);
    if (majorNumber < 0) {
        printk(KERN_ERR "sonar: failed to register a major number\n");
        sonar_free();
        return -EPERM;
    }
    // register the device class
    sonar_class = class_create(THIS_MODULE, CLASS_NAME);
    if (IS_ERR(sonar_class)) {
        printk(KERN_ERR "sonar: failed to register device class: %ld\n", PTR_ERR(sonar_class));
        sonar_free();
        return -EPERM;
    }
    // register the device driver
    sonar_device = device_create(sonar_class, NULL, MKDEV(majorNumber, DEV_MINOR), NULL, DEVICE_NAME);
    if (IS_ERR(sonar_device)) {
        printk(KERN_ERR "sonar: failed to create the TX device: %ld\n", PTR_ERR(sonar_device));
        sonar_free();
        return -EPERM;
    }

    // prepare pin for trigger
    trig_pin_desc = gpio_to_desc(trig_pin);
    if (IS_ERR(trig_pin_desc)) {
        printk(KERN_ERR "sonar: pin gpiod_request error: %ld\n", PTR_ERR(trig_pin_desc));
        sonar_free();
        return -EPERM;
    }
    // output
    gpiod_direction_output(trig_pin_desc, 0);

    // prepare pin for echo
    echo_pin_desc = gpio_to_desc(echo_pin);
    if (IS_ERR(echo_pin_desc)) {
        printk(KERN_ERR "sonar: pin gpiod_request error: %ld\n", PTR_ERR(trig_pin_desc));
        sonar_free();
        return -EPERM;
    }
    // output
    gpiod_direction_input(echo_pin_desc);

    // IRQ
    rx_irq_number = gpiod_to_irq(echo_pin_desc);
    r = request_irq(
        // The interrupt number requested
        rx_irq_number,
        // The pointer to the handler function below */
        (irq_handler_t) echo_irq_handler,
        /* Interrupt on falling edge */
        IRQF_TRIGGER_FALLING,
        /* Used in /proc/interrupts to identify the owner */
        "sonar_echo_handler",
        NULL);
    if (r) {
        printk(KERN_ERR "sonar: IRQ request error\n");
        sonar_free();
        return -1;
    }

    // allocate and init timers
    tx_low_timer = kzalloc(sizeof(struct hrtimer), GFP_KERNEL);
    if (!tx_low_timer) {
        printk(KERN_ERR "sonar: can't allocate memory for timer\n");
        sonar_free();
        return -ENOMEM;
    }
    hrtimer_init(tx_low_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
    tx_low_timer->function = tx_low_callback;
    // and second one
    tx_high_timer = kzalloc(sizeof(struct hrtimer), GFP_KERNEL);
    if (!tx_low_timer) {
        printk(KERN_ERR "sonar: can't allocate memory for timer\n");
        sonar_free();
        return -ENOMEM;
    }
    hrtimer_init(tx_high_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
    tx_high_timer->function = tx_high_callback;
    hrtimer_start(tx_high_timer, ktime_set(0, trig_interval * 1000UL), HRTIMER_MODE_REL);

    printk(KERN_INFO "sonar: driver started\n");
    return 0;
}

/* Function to unload module */
static void __exit sonar_exit(void)
{
    sonar_free();
    printk(KERN_INFO "sonar: driver stopped\n");
}

static int dev_open(struct inode *inodep, struct file *filep)
{
    sonar_file_t *sf;
    if (bus_number_opens >= MAX_OPENED_FILES)
        return -EMFILE;
    filep->private_data = kzalloc(sizeof(sonar_file_t), GFP_KERNEL);
    if (!filep->private_data)
        return -ENOMEM;
    sf = (sonar_file_t*)filep->private_data;
    sf->id = bus_number_opens;
    sf->read_pos = 0;
    sf->read_value = 0;
    opened_files[bus_number_opens] = filep;
    bus_number_opens++;
    return 0;
}
 
static ssize_t dev_read(struct file *filep, char *buffer, size_t len, loff_t *offset)
{
    int numlen = 0;
    int i, j;
    char c;
    ssize_t r;
    sonar_file_t *sf = (sonar_file_t*)filep->private_data;

    // no data yet
    if (!sf->read_pending) {
        if (filep->f_flags & O_NONBLOCK)
            return -EAGAIN;
        if (wait_event_interruptible(rx_wq, sf->read_pending))
            return -ERESTARTSYS;
    }

    // remember current value
    if (sf->read_pos == 0) sf->read_value = echo_delay;

    // get length of value in chars
    i = sf->read_value;
    do {
        i /= 10;
        numlen++;
    } while (i);

    for (r = 0; sf->read_pos <= numlen && r < len; r++, sf->read_pos++, buffer++) {
        if (sf->read_pos < numlen) {
            j = sf->read_value;
            for (i = 1; i < numlen - sf->read_pos; i++)
            {
                j /= 10;
            }
            c = '0' + (j % 10);
        } else {
            sf->read_pending = 0;
            c = '\n';            
        }
        put_user(c, buffer);
    }
    if (sf->read_pos >= numlen) sf->read_pos = 0;

    return r;
}

static int dev_release(struct inode *inodep, struct file *filep)
{
    int id;
    bus_number_opens--;
    id = ((sonar_file_t*)filep->private_data)->id;
    opened_files[id] = opened_files[bus_number_opens];
    ((sonar_file_t*)opened_files[id]->private_data)->id = id;
    kfree(filep->private_data);
    return 0;
}

module_init(sonar_init);
module_exit(sonar_exit);
