/* vi: set sw=2 ts=2 tw=80: */
/******************************************************************************
 * icoc8.c
 *
 * Driver for Access Manager MIFARE IC8 & OC8 peripheral support
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
#include "icoc8.h"

MODULE_AUTHOR("swyss@kbr.kaba.com");
MODULE_DESCRIPTION("Driver for Access Manager MIFARE OC8 peripheral support");
MODULE_LICENSE("GPL");

/*-----------------------------------------------------------------------------
 * Private register addresses and bitmask definitions 
 *---------------------------------------------------------------------------*/
#define BASE_AT91_PIOA	0xfffff400
#define PIO_PER					0x00
#define PIO_OER					0x10
#define PIO_ODR					0x14
#define PIO_SODR				0x30
#define PIO_CODR				0x34	
#define PIO_PDSR				0x3C

#define IOSTROBE				(1<<4)
#define IOBACK					(1<<5)

/*-----------------------------------------------------------------------------
 * Global register access pointer
 *---------------------------------------------------------------------------*/
static void * memBase;

/*-----------------------------------------------------------------------------
 * forward function declaration
 *---------------------------------------------------------------------------*/
void icoc8_exit(void);
int icoc8_init(void);
int icoc8_ioctl(struct inode*, struct file* , unsigned int, unsigned long);
module_init(icoc8_init);
module_exit(icoc8_exit);

/*-----------------------------------------------------------------------------
 * file operations
 *---------------------------------------------------------------------------*/
struct file_operations icoc8_fops = {
	owner:	THIS_MODULE,
	ioctl:	icoc8_ioctl,
};

/*-----------------------------------------------------------------------------
 * icoc8_ioctl()
 *---------------------------------------------------------------------------*/
int icoc8_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg)
{
	DRVMSG drvMsg;
	int ret, regval;
	
	switch (cmd)
	{
		// **************** IOCTL_ICOC8_STROBE ****************
		case IOCTL_ICOC8_STROBE:
			
			writel(IOSTROBE, memBase+PIO_CODR);
			udelay(100);
			writel(IOSTROBE, memBase+PIO_SODR);
			break;
	
		// **************** IOCTL_ICOC8_GETIOBACK ****************
		case IOCTL_ICOC8_GETIOBACK:
	
			drvMsg.ioback = ((readl(memBase+PIO_PDSR)&IOBACK)>0); 
			ret=copy_to_user((void*)arg,&drvMsg, sizeof(drvMsg));
			break;

		default:
			return -EFAULT;
	}
	return 0;
}

/*-----------------------------------------------------------------------------
 * icoc8_init()
 *---------------------------------------------------------------------------*/
int icoc8_init(void) 
{
	int res;

	// register memory base 
	memBase = ioremap_nocache(BASE_AT91_PIOA,0x200);

	// Registering device node 
	res = register_chrdev(ICOC8_MAJOR, "icoc8", &icoc8_fops);
	if (res < 0) {
		printk("<0>icoc8: cannot register major number %d\n", ICOC8_MAJOR);
		return res;
	}

	// setup IO ports
	writel(IOSTROBE, memBase+PIO_PER);
	writel(IOSTROBE, memBase+PIO_OER);
	writel(IOBACK, memBase+PIO_PER);
	writel(IOBACK, memBase+PIO_ODR);

	// everything initialized
	printk("<0>icoc8: module initialized\n");
	return 0;
}

/*-----------------------------------------------------------------------------
 * icoc8_exit()
 *---------------------------------------------------------------------------*/
void icoc8_exit(void) 
{
	unregister_chrdev(ICOC8_MAJOR, "icoc8");

	/* unmap memBase */
	iounmap(memBase);
	
	printk("<0>icoc8: module removed\n");
}



