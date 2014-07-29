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
#include <linux/module.h>

#define value 1000;

static int __init buzzer_init(void)
{
	int count = PIT_TICK_RATE / value;

	printk(KERN_INFO "buzzer: driver loaded\n");

	/* set buzzer */
	outb_p(0xB6, 0x43);
	/* select desired HZ */
	outb_p(count & 0xff, 0x42);
	outb((count >> 8) & 0xff, 0x42);

	/* start beep */
	outb_p(inb_p(0x61) | 3, 0x61);

	msleep(300);

	/* stop beep */
	outb(inb_p(0x61) & 0xFC, 0x61);

	return 0;
}

static void __exit buzzer_exit(void)
{
	printk(KERN_INFO "buzzer: driver unloaded\n");	
}

module_init(buzzer_init);
module_exit(buzzer_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Matteo Croce");
