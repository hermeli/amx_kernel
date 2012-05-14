/* vi: set sw=2 ts=2 tw=80: */
/******************************************************************************
 * amx.h
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
#ifndef _AMX_DRIVER_H
#define _AMX_DRIVER_H

#ifdef __cplusplus
extern "C" {																																							
#endif

#define AMX_MAJOR	61

#define AML 'L'
#define AMM	'M'

// Driver data transfer structure
typedef struct _DRVMSG
{
	unsigned char	nres;			// Legic/Security chip reset*
	unsigned char	txrdy;			// Legic TX_READY
	char 			board;			// 'L' => AM-L oder 'M' => AM-M
} DRVMSG;

// Bitmask definitions for PINS
#define HIGH		0x01
#define LOW			0x00

// IOCTL codes
#define AMX_IOC_MAGIC				'k'
#define IOCTL_AMX_GET 				_IOWR(AMX_IOC_MAGIC, 0, int)
#define IOCTL_AMX_SET				_IOWR(AMX_IOC_MAGIC, 1, int)

#ifdef __cplusplus
} /* extern "C"*/
#endif

#endif /*_AMX_DRIVER_H*/
