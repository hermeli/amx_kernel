/* vi: set sw=2 ts=2 tw=80: */
/******************************************************************************
 * ledout.c
 *
 * Driver for Access Manager MIFARE LEDs, OUTPUTs and RESET button
 *
 * Copyright (C) 2011 KABA AG, CC EAC
 *
 * Distributed under the terms of the GNU General Public License
 * This software may be used without warrany provided and
 * copyright statements are left intact.
 * Developer: Stefan Wyss - swyss@kbr.kaba.com
 *
 *****************************************************************************/
#define __NO_VERSION__
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/ioport.h>
#include <linux/delay.h>
#include <linux/fs.h>
#include <linux/types.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include <asm/system.h>
#include "ledout.h"

MODULE_AUTHOR("swyss@kbr.kaba.com");
MODULE_DESCRIPTION("Driver for Access Manager MIFARE LEDs and OUTPUTs");
MODULE_LICENSE("GPL");

/*-----------------------------------------------------------------------------
 * Private register addresses and bitmask definitions 
 *---------------------------------------------------------------------------*/
#define BASE_AT91_PIOC	0xfffff800
#define PIO_PER					0x00
#define PIO_OER					0x10
#define PIO_ODR					0x14
#define PIO_SODR				0x30
#define PIO_CODR				0x34	
#define PIO_PDSR				0x3C

#define STATE_RED				(1<<7)
#define OUT1_RED				(1<<21)
#define OUT2_RED				(1<<22)
#define OUT3_RED				(1<<23)

#define STATE_GREEN			(1<<9)
#define OUT1_GREEN			(1<<25)
#define OUT2_GREEN			(1<<26)
#define OUT3_GREEN			(1<<27)
#define RESET						(1<<14)

/*-----------------------------------------------------------------------------
 * Global register access pointer
 *---------------------------------------------------------------------------*/
static void * memBase;

/*-----------------------------------------------------------------------------
 * forward function declaration
 *---------------------------------------------------------------------------*/
void ledout_exit(void);
int ledout_init(void);
int ledout_ioctl(struct inode*, struct file* , unsigned int, unsigned long);
module_init(ledout_init);
module_exit(ledout_exit);

/*-----------------------------------------------------------------------------
 * file operations
 *---------------------------------------------------------------------------*/
struct file_operations ledout_fops = {
	owner:	THIS_MODULE,
	ioctl:	ledout_ioctl,
};

/*-----------------------------------------------------------------------------
 * ledout_ioctl()
 *---------------------------------------------------------------------------*/
int ledout_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg)
{
	DRVMSG drvMsg;
	int ret, regval;
	
	switch (cmd)
	{
		// **************** IOCTL_LEDOUT_GET ****************
		case IOCTL_LEDOUT_GET:
			regval = readl(memBase+PIO_PDSR);
			drvMsg.state = RED*((regval&STATE_RED)>0) + GREEN*((regval&STATE_GREEN)>0);
			drvMsg.out1 = RED*((regval&OUT1_RED)>0) + GREEN*((regval&OUT1_GREEN)>0);
			drvMsg.out2 = RED*((regval&OUT2_RED)>0) + GREEN*((regval&OUT2_GREEN)>0);
			drvMsg.out3 = RED*((regval&OUT3_RED)>0) + GREEN*((regval&OUT3_GREEN)>0);
			drvMsg.reset = ((readl(memBase+PIO_PDSR)&RESET)>0); 

			ret=copy_to_user((void*)arg,&drvMsg, sizeof(drvMsg));
		 	if (ret!=0){
				return -EFAULT;
			} 
			break;
					
		// **************** IOCTL_LEDOUT_SET ****************
		case IOCTL_LEDOUT_SET:

			// get led structure from user space
			ret = copy_from_user(&drvMsg,(DRVMSG*)arg, sizeof(drvMsg));
			if(ret)	
				return -EFAULT;

			writel(STATE_RED, (drvMsg.state&RED)?memBase+PIO_SODR:memBase+PIO_CODR);
			writel(STATE_GREEN, (drvMsg.state&GREEN)?memBase+PIO_SODR:memBase+PIO_CODR);
			writel(OUT1_RED, (drvMsg.out1&RED)?memBase+PIO_SODR:memBase+PIO_CODR);
			writel(OUT1_GREEN, (drvMsg.out1&GREEN)?memBase+PIO_SODR:memBase+PIO_CODR);
			writel(OUT2_RED, (drvMsg.out2&RED)?memBase+PIO_SODR:memBase+PIO_CODR);
			writel(OUT2_GREEN, (drvMsg.out2&GREEN)?memBase+PIO_SODR:memBase+PIO_CODR);
			writel(OUT3_RED, (drvMsg.out3&RED)?memBase+PIO_SODR:memBase+PIO_CODR);
			writel(OUT3_GREEN, (drvMsg.out3&GREEN)?memBase+PIO_SODR:memBase+PIO_CODR);
			break;

		default:
			return -EFAULT;
	}
	return 0;
}

/*-----------------------------------------------------------------------------
 * ledout_init()
 *---------------------------------------------------------------------------*/
int ledout_init(void) 
{
	int res;

	// register memory base 
	memBase = ioremap_nocache(BASE_AT91_PIOC,0x200);

	// Registering device node 
	res = register_chrdev(LEDOUT_MAJOR,"ledout", &ledout_fops);
	if (res < 0) {
		printk("<0>ledout: cannot register major number %d\n", LEDOUT_MAJOR);
		return res;
	}

	// set output port direction specifiers
	writel(STATE_RED|OUT1_RED|OUT2_RED|OUT3_RED|STATE_GREEN|OUT1_GREEN|OUT2_GREEN|OUT3_GREEN|RESET, memBase+PIO_PER);
	writel(STATE_RED|OUT1_RED|OUT2_RED|OUT3_RED|STATE_GREEN|OUT1_GREEN|OUT2_GREEN|OUT3_GREEN, memBase+PIO_OER);
	writel(RESET, memBase+PIO_ODR);

	// everything initialized
	printk("<0>ledout: module initialized\n");
	return 0;
}

/*-----------------------------------------------------------------------------
 * ledout_exit()
 *---------------------------------------------------------------------------*/
void ledout_exit(void) 
{
	unregister_chrdev(LEDOUT_MAJOR, "ledout");

	/* unmap memBase */
	iounmap(memBase);
	
	printk("<0>ledout: module removed\n");
}



