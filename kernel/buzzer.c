/*
 * Buzzer driver
 *
 * Copyright (C) 2014 Matteo Croce
 * based on linux/drivers/input/misc/pcspkr.c
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
#include <linux/i8253.h>
#include <linux/platform_device.h>

#define CONTROL_WORD_REG	0x43
#define COUNTER2		0x42
#define SPEAKER_PORT		0x61

static void buzz(unsigned ms, unsigned hz)
{
	unsigned long flags;
	u8 p61;

	raw_spin_lock_irqsave(&i8253_lock, flags);

	if (hz >= 20 && hz <= 20000) {
		unsigned count = PIT_TICK_RATE / hz;

		/* set buzzer
		* 0xB6
		* 1 0		Counter 2
		* 1 1		2xRead/2xWrite bits 0..7 then 8..15 of counter value
		* 0 1 1	Mode 3: Square Wave
		* 0		Counter is a 16 bit binary counter (0..65535)
		*/
		outb_p(0xB6, CONTROL_WORD_REG);

		/* select desired HZ with two writes in counter 2, port 42h */
		outb_p(count & 0xff, COUNTER2);
		outb_p((count >> 8) & 0xff, COUNTER2);

		/* start beep
		* set bit 0-1 (0: SPEAKER DATA; 1: OUT2) of GATE2 (port 61h)
		*/
		p61 = inb_p(SPEAKER_PORT);
		if ((p61 & 3) != 3)
			outb_p(p61 | 3, SPEAKER_PORT);
	}
	raw_spin_unlock_irqrestore(&i8253_lock, flags);

	msleep(ms);

	/* stop beep
	 * clear bit 0-1 of port 61h
	 */
	raw_spin_lock_irqsave(&i8253_lock, flags);

	p61 = inb_p(SPEAKER_PORT);
	if (p61 & 3)
		outb(p61 & 0xFC, SPEAKER_PORT);

	raw_spin_unlock_irqrestore(&i8253_lock, flags);
}

static ssize_t sysfsbuzz(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	unsigned ms;
	unsigned hz = 1000;

	sscanf(buf, "%u %u", &ms, &hz);

	if (ms <= 2000)
		buzz(ms, hz);

	return count;
}

static DEVICE_ATTR(buzzer, 0200, NULL, sysfsbuzz);

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
