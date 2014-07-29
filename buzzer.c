/*
 * Buzzer driver
 *
 * Copyright (C) 2014 Matteo Croce
 * 
 * usage: echo milliseconds frequency >/sys/devices/platform/pcspkr/buzzer
 * if frequency is omitted default is 1000 Hz
 * 
 * eg:
 * echo 100 440 >/sys/devices/platform/pcspkr/buzzer
 * echo 100 >/sys/devices/platform/pcspkr/buzzer
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
#include <linux/platform_device.h>

void buzz(int ms, int hz)
{
	int count = 0;

	if (hz < 20 || hz > 20000) {
		outb(inb_p(0x61) & 0xFC, 0x61);
		return;
	}

	count = PIT_TICK_RATE / hz;

	/* set buzzer */
	outb_p(0xB6, 0x43);
	/* select desired HZ */
	outb_p(count & 0xff, 0x42);
	outb((count >> 8) & 0xff, 0x42);

	/* start beep */
	outb_p(inb_p(0x61) | 3, 0x61);

	msleep(ms);

	/* stop beep */
	outb(inb_p(0x61) & 0xFC, 0x61);
}

static ssize_t sysfsbuzz(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	int ms;
	int hz = 1000;

	sscanf(buf, "%u %u", &ms, &hz);

	buzz(ms, hz);

	return count;
}

static DEVICE_ATTR(buzzer, 0222, NULL, sysfsbuzz);

static int buzzer_probe(struct platform_device *dev)
{
	int err = device_create_file(&dev->dev, &dev_attr_buzzer);

	if (err < 0)
		return err;

	printk(KERN_INFO "buzzer: driver loaded\n");

	return 0;
}

static int buzzer_remove(struct platform_device *dev)
{
	device_remove_file(&dev->dev, &dev_attr_buzzer);
	buzz(0, 0);
	printk(KERN_INFO "buzzer: driver unloaded\n");

	return 0;
}

static void buzzer_shutdown(struct platform_device *dev)
{
	buzz(0, 0);
}

static struct platform_driver buzzer_platform_driver = {
	.driver		= {
		.name	= "pcspkr",
		.owner	= THIS_MODULE,
	},
	.probe		= buzzer_probe,
	.remove		= buzzer_remove,
	.shutdown	= buzzer_shutdown,
};
module_platform_driver(buzzer_platform_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Matteo Croce");
