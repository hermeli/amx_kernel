/* vi: set sw=2 ts=2 tw=80: */
/******************************************************************************
 * amx.c
 *
 * Driver for Access Manager LEGIC/MIFARE system (special functions)
 *
 * Copyright (C) 2012 KABA AG, MIC AWM
 *
 * Distributed under the terms of the GNU General Public License
 * This software may be used without warrany provided and
 * copyright statements are left intact.
 * Developer: Stefan Wyss - stefan.wyss@kaba.com
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
#include "amx.h"

MODULE_AUTHOR("stefan.wyss@kaba.com");
MODULE_DESCRIPTION("Driver for Access Manager system");
MODULE_LICENSE("GPL");

/*-----------------------------------------------------------------------------
 * Private register addresses and bitmask definitions 
 *---------------------------------------------------------------------------*/
#define AT91_BASE_PIOC			0xfffff800
#define PIO_PER					0x00
#define PIO_PSR					0x08

#define PIO_OER					0x10
#define PIO_ODR					0x14
#define PIO_OSR					0x18
#define PIO_SODR				0x30
#define PIO_CODR				0x34	
#define PIO_ODSR				0x38
#define PIO_PDSR				0x3C
#define PIO_MDSR				0x58
#define PIO_PUER				0x64

#define LG_NRES					(1<<30)
#define SC_NRES					(1<<31)
#define LG_TXRDY				(1<<13)

#define AT91_BASE_PMC			0xfffffc00
#define PMC_SCER				0x00
#define ID_PIOC   				((unsigned int)  4) // Parallel IO Controller C	
/*-----------------------------------------------------------------------------
 * Global register access pointer
 *---------------------------------------------------------------------------*/
static void * piocBase;
static void * pmcBase;
static unsigned char board=0;

/*-----------------------------------------------------------------------------
 * forward function declaration
 *---------------------------------------------------------------------------*/
void amx_exit(void);
int amx_init(void);
int amx_ioctl(struct inode*, struct file* , unsigned int, unsigned long);
module_init(amx_init);
module_exit(amx_exit);

/*-----------------------------------------------------------------------------
 * file operations
 *---------------------------------------------------------------------------*/
struct file_operations amx_fops = {
	owner:	THIS_MODULE,
	ioctl:	amx_ioctl,
};

/*-----------------------------------------------------------------------------
 * amx_ioctl()
 *---------------------------------------------------------------------------*/
int amx_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg)
{
	DRVMSG drvMsg;
	int ret;
	
	// set output port direction specifiers
	writel(LG_NRES|LG_TXRDY, piocBase+PIO_PER);	// PIO enable
	writel(LG_NRES, piocBase+PIO_OER);			// OUTPUT enable
	writel(LG_TXRDY, piocBase+PIO_ODR);			// OUTPUT disable
	writel(1<<ID_PIOC,pmcBase+PMC_SCER);		// enable PIOC clock
	
	switch (cmd)
	{
		
		// **************** IOCTL_AMX_GET ****************
		case IOCTL_AMX_GET:		
			
			if (board == AML)
			{
				drvMsg.nres = ((readl(piocBase+PIO_PDSR)&LG_NRES)>0);
				drvMsg.txrdy = ((readl(piocBase+PIO_PDSR)&LG_TXRDY)>0);
			} 
			else // AMM
			{
				drvMsg.nres = ((readl(piocBase+PIO_PDSR)&SC_NRES)>0);
			}
			ret=copy_to_user((void*)arg,&drvMsg, sizeof(drvMsg));
			if (ret!=0)
				return -EFAULT;
			break;
					
		// **************** IOCTL_AMX_SET ****************
		case IOCTL_AMX_SET:

			// get structure from user space
			ret = copy_from_user(&drvMsg,(DRVMSG*)arg, sizeof(drvMsg));
			if(ret)	
				return -EFAULT;

			if(board == AML)
				writel(LG_NRES,(drvMsg.nres)?piocBase+PIO_SODR:piocBase+PIO_CODR);
			else
				writel(SC_NRES,(drvMsg.nres)?piocBase+PIO_SODR:piocBase+PIO_CODR);
			break;

		default:
			return -EFAULT;
	}
			
	return 0;
}

/*-----------------------------------------------------------------------------
 * amx_init()
 *---------------------------------------------------------------------------*/
int amx_init(void) 
{
	int res;

	// register memory base 
	piocBase = ioremap_nocache(AT91_BASE_PIOC,0x200);
	pmcBase = ioremap_nocache(AT91_BASE_PMC,0x200);

	// Registering device node 
	res = register_chrdev(AMX_MAJOR,"amx", &amx_fops);
	if (res < 0) {
		printk("<0>amx: cannot register major number %d\n", AMX_MAJOR);
		return res;
	}

	// set output port direction specifiers
	writel(LG_NRES|LG_TXRDY, piocBase+PIO_PER);	// PIO enable
	writel(LG_NRES, piocBase+PIO_OER);			// OUTPUT enable
	writel(LG_TXRDY, piocBase+PIO_ODR);			// OUTPUT disable
	writel(1<<ID_PIOC,pmcBase+PMC_SCER);		// enable PIOC clock

	// detect AMx board/sytem type
	// 
	// Pin:			Value:	Board:					Signal:			Remarks
	// -------------------------------------------------------------------------------------------
	// [PIOC-31]	= 0		Access Manager MIFARE	SC_NRES = 0		Pull-Down an Security Chip
	//				= 1		Access Manger LEGIC		n.A.			Internal Pull-Up CPU
	// [PIOC-30]	= 0		Access Manager LEGIC	LG_NRES = 0		Pull-Down an Legic Chip	
	//				= 1		n.A. 
	writel(LG_NRES|SC_NRES,piocBase+PIO_PUER);		// pull-up enable
	writel(LG_NRES|SC_NRES,piocBase+PIO_ODR);		// output disable
	udelay(1000);

	if (readl(piocBase+PIO_PDSR)&SC_NRES)
	{
		if (readl(piocBase+PIO_PDSR)&LG_NRES)
			panic("board type not supported!\n");	
		else
			board = AML;			
	} 
	else
		board = AMM;			
		
	// everything initialized
	if (board==AMM)
		printk("<0>amx: module initialized - board type is AM-M\n");
	else
		printk("<0>amx: module initialized - board type is AM-L\n");
	
	return 0;
}

/*-----------------------------------------------------------------------------
 * amx_exit()
 *---------------------------------------------------------------------------*/
void amx_exit(void) 
{
	unregister_chrdev(AMX_MAJOR, "amx");

	/* unmap piocBase */
	iounmap(piocBase);
	iounmap(pmcBase);
	
	printk("<0>amx: module removed\n");
}



