/*
 * Buzzer driver.
 *
 * Copyright (C) 2014 Matteo Croce
 */

/*
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published by
 * the Free Software Foundation.
 */
#include <linux/delay.h>
#include <linux/timex.h>
#include <linux/module.h>
#include <linux/sysfs.h>
#include <linux/device.h>

static ssize_t buzz(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	int ms;
	int hz = 1000;
	int c;

	if(strchr(buf, ' '))
		sscanf(buf, "%u %u", &ms, &hz);
	else {
		//kstrtoul(buf, 0, ms);
		sscanf(buf, "%u", &ms);
	}
	c = PIT_TICK_RATE / hz;

	/* set buzzer */
	outb_p(0xB6, 0x43);
	/* select desired HZ */
	outb_p(c & 0xff, 0x42);
	outb((c >> 8) & 0xff, 0x42);

	/* start beep */
	outb_p(inb_p(0x61) | 3, 0x61);

	msleep(ms);

	/* stop beep */
	outb(inb_p(0x61) & 0xFC, 0x61);

	printk("buzz: %dms %dhz\n", ms, hz);

	return count;
}

static struct class *buzzer_class;
static struct device *dev;

static DEVICE_ATTR(buzzer, 0222, NULL, buzz);

static int __init buzzer_init(void)
{
	int err;

	buzzer_class = class_create(THIS_MODULE, "buzzer");
	if(!buzzer_class)
		return -1;

	dev = device_create(buzzer_class, NULL, 260, NULL, "buzzer");
	if(!dev)
		return -1;

	err = device_create_file(dev, &dev_attr_buzzer);
	if (err < 0)
		return err;

	printk(KERN_INFO "buzzer: driver loaded\n");

	return 0;
}

static void __exit buzzer_exit(void)
{
	device_remove_file(dev, &dev_attr_buzzer);
	device_destroy(buzzer_class, 260);
	class_destroy(buzzer_class);

	printk(KERN_INFO "buzzer: driver unloaded\n");	
}

module_init(buzzer_init);
module_exit(buzzer_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Matteo Croce");
