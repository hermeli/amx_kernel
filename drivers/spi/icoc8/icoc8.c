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
#define BASE_AT91_PIOC	0xfffff800

#define PIO_PER					0x00
#define PIO_OER					0x10
#define PIO_ODR					0x14
#define PIO_SODR				0x30
#define PIO_CODR				0x34	
#define PIO_PDSR				0x3C

#define IOSTROBE				(1<<4)		// PIOA
#define IOBACK					(1<<5)		// PIOA

#define NPCS00					(1<<3)		// PIOA
#define NPCS01					(1<<11)		// PIOC
#define NPCS02					(1<<16)		// PIOC
#define NPCS03					(1<<17)		// PIOC

/*-----------------------------------------------------------------------------
 * Global register access pointer
 *---------------------------------------------------------------------------*/
static void * basePioA;
static void * basePioC;

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
			
			writel(IOSTROBE, basePioA+PIO_CODR);
			udelay(100);
			writel(IOSTROBE, basePioA+PIO_SODR);
			break;
	
		// **************** IOCTL_ICOC8_GETIOBACK ****************
		case IOCTL_ICOC8_GETIOBACK:
	
			drvMsg.ioback = ((readl(basePioA+PIO_PDSR)&IOBACK)>0); 
			ret=copy_to_user((void*)arg,&drvMsg, sizeof(drvMsg));
			break;

		// **************** IOCTL_ICOC8_SETCS ****************
		case IOCTL_ICOC8_SETCS:

			// get driver message structure from user space
			ret = copy_from_user(&drvMsg,(DRVMSG*)arg, sizeof(drvMsg));
			if(ret)	
				return -EFAULT;

			writel(NPCS00,(drvMsg.chipsel & 0x01)?basePioA+PIO_SODR:basePioA+PIO_CODR);
			writel(NPCS01,(drvMsg.chipsel & 0x02)?basePioC+PIO_SODR:basePioC+PIO_CODR);
			writel(NPCS02,(drvMsg.chipsel & 0x04)?basePioC+PIO_SODR:basePioC+PIO_CODR);
			writel(NPCS03,(drvMsg.chipsel & 0x08)?basePioC+PIO_SODR:basePioC+PIO_CODR);
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
	basePioA = ioremap_nocache(BASE_AT91_PIOA,0x200);
	basePioC = ioremap_nocache(BASE_AT91_PIOC,0x200);

	// Registering device node 
	res = register_chrdev(ICOC8_MAJOR, "icoc8", &icoc8_fops);
	if (res < 0) {
		printk("<0>icoc8: cannot register major number %d\n", ICOC8_MAJOR);
		return res;
	}

	// setup PIOA ports
	writel(IOSTROBE|IOBACK|NPCS00, basePioA+PIO_PER);
	writel(IOSTROBE|NPCS00, basePioA+PIO_OER);
	writel(IOBACK, basePioA+PIO_ODR);

	// setup PIOC ports
	writel(NPCS01|NPCS02|NPCS03, basePioC+PIO_PER);
	writel(NPCS01|NPCS02|NPCS03, basePioC+PIO_OER);

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

	/* unmap basePioA */
	iounmap(basePioA);
	
	printk("<0>icoc8: module removed\n");
}



