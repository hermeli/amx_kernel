/* vi: set sw=2 ts=2 tw=80: */
/******************************************************************************
 * ledout.h
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
#ifndef _LEDOUT_H
#define _LEDOUT_H

#ifdef __cplusplus
extern "C" {																																							
#endif

#define LEDOUT_MAJOR						62

// Driver data transfer structure
typedef struct _DRVMSG
{
	unsigned char	state;		  		// STATE Led state
	unsigned char	out1;		  			// OUT1 Led/Relais output
	unsigned char	out2;		  			// Out2 Led/Relais output
	unsigned char	out3;						// Out3 Led/Relais output
	unsigned char reset;					// Reset Button state
} DRVMSG;

// Bitmask definitions for LEDs, OUTs and RESET
#define OFF											0x00
#define RED											0x01
#define GREEN										0x02
#define ORANGE									0x03
#define REL_ON									GREEN
#define REL_OFF									OFF
#define RESET_INACTIVE					0x01
#define RESET_ACTIVE						0x00

// IOCTL codes
#define LEDOUT_IOC_MAGIC				'k'
#define IOCTL_LEDOUT_GET 				_IOWR(LEDOUT_IOC_MAGIC, 0, int)
#define IOCTL_LEDOUT_SET				_IOWR(LEDOUT_IOC_MAGIC, 1, int)

#ifdef __cplusplus
} /* extern "C"*/
#endif

#endif /*_LEDOUT_H*/
