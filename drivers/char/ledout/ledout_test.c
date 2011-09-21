/* vi: set sw=2 ts=2 tw=80: */
/******************************************************************************
 * ledout_test.c
 *
 * Testprogram for Access Manager MIFARE LEDs, OUTPUTs and RESET button driver
 *
 * Copyright (C) 2011 KABA AG, CC EAC
 *
 * Distributed under the terms of the GNU General Public License
 * This software may be used without warrany provided and
 * copyright statements are left intact.
 * Developer: Stefan Wyss - swyss@kbr.kaba.com
 *
 *****************************************************************************/
#include <stdio.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include "ledout.h"

/*-----------------------------------------------------------------------------
 * main program function 
 *---------------------------------------------------------------------------*/
int main(void)
{
	unsigned int data;
	int fd, ret, i;
	char devname[20] = "/dev/ledout";
	DRVMSG drvMsg;

	printf("Simple LEDOUT driver test program\r\n");

	// *** open the device node ***
	fd = open((char *)devname,O_RDWR);
	if(fd < 0) {
		printf("Error opening %s\n",devname);
		return -1;
	}
	
	// *** set LED's - disco time! *** 
	
	// Attention: Please note that signals out1..out3 are shared among LEDs
  // and RELAIS. I.e. setting GREEN is the same as setting REL_ON on out1. 
	// Setting OFF is the same as setting REL_OFF on out1.   

	for (i=0; i<20; i++)
	{
		drvMsg.state =(i+OFF)%4;
		drvMsg.out1 = (i+RED)%4;
		drvMsg.out2 = (i+GREEN)%4;
		drvMsg.out3 = (i+ORANGE)%4;
		ret = ioctl(fd, IOCTL_LEDOUT_SET, &drvMsg);
		if (ret != 0)
		{
			printf("ioctl IOCTL_LEDOUT_SET returned error 0x%X\n", ret);
			return -1;
		}
		usleep(200000);
	}
	
	// *** get LED's state ***  
	ret = ioctl(fd, IOCTL_LEDOUT_GET, &drvMsg);
	if (ret != 0)
	{
		printf("ioctl IOCTL_LEDOUT_GET returned error 0x%X\n", ret);
		return -1;
	}
	
	if ( (drvMsg.state != ORANGE)||(drvMsg.out1 != OFF)||(drvMsg.out2 != RED)||(drvMsg.out3 != GREEN) )
	{
		printf("error: wrong LED return values:!\n");
		printf("state=%d out1=%d out2=%d out3=%d (0=off/1=red/2=green/3=orange)\n",drvMsg.state,drvMsg.out1,drvMsg.out2,drvMsg.out3);
		return -1;
	} 
	printf("-> LED state test:\t\t\t[OK]\n"); 

	// *** get reset button state *** 
	ret = ioctl(fd, IOCTL_LEDOUT_GET, &drvMsg);
	if (ret != 0)
	{
		printf("ioctl IOCTL_LEDOUT_GET returned error 0x%X\n", ret);
		return -1;
	}
	if (drvMsg.reset == RESET_ACTIVE)
	{
		printf("-> reset button test:\t\t\t[FAIL]\n"); 
		return -1;
	}

	printf("Please press and hold the reset button!\n");
	for (i=0; i<60; i++)
	{
		ret = ioctl(fd, IOCTL_LEDOUT_GET, &drvMsg);
		if (ret != 0)
		{
			printf("ioctl IOCTL_LEDOUT_GET returned error 0x%X\n", ret);
			return -1;
		}
		if (drvMsg.reset == RESET_ACTIVE)
		{
			printf("-> reset button test:\t\t\t[OK]\n"); 
			break;
		}
		sleep(1);
	}

	// *** set RELAIS on *** 
	drvMsg.out1 = REL_ON;
	drvMsg.out2 = REL_ON;
	drvMsg.out3 = REL_ON;
	ret = ioctl(fd, IOCTL_LEDOUT_SET, &drvMsg);
	if (ret != 0)
	{
		printf("ioctl IOCTL_LEDOUT_SET returned error 0x%X\n", ret);
		return -1;
	}

	close(fd);
	return 0;
}
