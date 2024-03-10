#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include <linux/slab.h>
#include <linux/gpio.h>

#include <linux/io.h>

#define SONIC_MAJOR 225   // GPIO 25
#define SONIC_NAME "SONIC_DRIVER"

#define BCM2711_PERL_BASE 0xFE000000
#define GPIO_BASE (BCM2711_PERL_BASE + 0x200000)
#define GPIO_SIZE 256

char sonic_usage = 0;
static void* sonic_map;
volatile unsigned* sonic;
static char tmp_buf;

static int sonic_open(struct inode* minode, struct file* mfile)
{
    if (sonic_usage != 0)
        return -EBUSY;
    sonic_usage = 1;
    sonic_map = ioremap(GPIO_BASE, GPIO_SIZE);
    if (!sonic_map)
    {
        printk("error: mapping gpio memory");
        iounmap(sonic_map);
        return -EBUSY;
    }
    sonic = (volatile unsigned int*)sonic_map;

    // Configure GPIO pin as an input pin
    *(sonic + 2) &= ~(0x7 << (3 * 3));

    return 0;
}

static int sonic_release(struct inode* minode, struct file* mfile)
{
    sonic_usage = 0;
    if (sonic)
        iounmap(sonic);

    return 0;
}

static int sonic_read(struct file* mfile, char* gdata, size_t length, loff_t* off_what)
{
    int result;
    int distance;

    // Generate trigger signal
    gpio_set_value(/* GPIO pin number for trigger */, 1);
    udelay(/* Delay for trigger pulse duration */);
    gpio_set_value(/* GPIO pin number for trigger */, 0);

    // Wait for echo signal
    while (gpio_get_value(/* GPIO pin number for echo */) == 0) {
        // Add timeout handling if needed
    }

    // Measure echo duration
    unsigned long start_time = jiffies;
    while (gpio_get_value(/* GPIO pin number for echo */) == 1) {
        // Add timeout handling if needed
    }
    unsigned long end_time = jiffies;

    // Calculate distance from echo duration
    // Assume the speed of sound is approximately 343 m/s
    // and the echo travels back and forth
    unsigned long duration = end_time - start_time;
    distance = (duration * 34300) / (HZ * 2);  // Convert to centimeters

    if (distance >= 20) {
        printk("Alert: Distance is %d cm. Object detected!\n", distance);
    }

    printk("sonic_read: distance = %d cm\n", distance);

    result = copy_to_user((void*)gdata, &distance, sizeof(distance));
    if (result < 0)
    {
        printk("error: copy_to_user()");
        return result;
    }
    return sizeof(distance);
}

static struct file_operations sonic_fops =
{
   .owner = THIS_MODULE,
   .open = sonic_open,
   .release = sonic_release,
   .read = sonic_read,
};

static int sonic_init(void)
{
    int result;
    result = register_chrdev

    (SONIC_MAJOR, SONIC_NAME, &sonic_fops);
    if (result < 0)
    {
        printk(KERN_WARNING "Can't get any major!\n");
        return result;
    }
    printk("SONIC module uploaded.\n");
    return 0;
}

static void sonic_exit(void)
{
    unregister_chrdev(SONIC_MAJOR, SONIC_NAME);
    printk("SONIC module removed.\n");
}

module_init(sonic_init);
module_exit(sonic_exit);