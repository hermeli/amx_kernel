/* vi: set sw=2 ts=2 tw=80: */
/******************************************************************************
 * icoc8.h
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
#ifndef _ICOC8_H
#define _ICOC8_H

#ifdef __cplusplus
extern "C" {																																							
#endif

#define ICOC8_MAJOR						63

// Driver data transfer structure
typedef struct _DRVMSG
{
	unsigned char ioback;					// ioback signal state
	unsigned char chipsel;				// chip-select used for following SPI transfer
} DRVMSG;

// IOCTL codes
#define ICOC8_MAGIC							'k'
#define IOCTL_ICOC8_STROBE 			_IOWR(ICOC8_MAGIC, 0, int)
#define IOCTL_ICOC8_GETIOBACK		_IOWR(ICOC8_MAGIC, 1, int)
#define IOCTL_ICOC8_SETCS				_IOWR(ICOC8_MAGIC, 2, int)

#ifdef __cplusplus
} /* extern "C"*/
#endif

#endif /*_ICOC8_H*/
