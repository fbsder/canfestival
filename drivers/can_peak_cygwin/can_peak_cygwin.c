/*
This file is part of CanFestival, a library implementing CanOpen Stack. 

Copyright (C): Edouard TISSERANT and Francis DUPIN

See COPYING file for copyrights details.

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <stddef.h>				/* for NULL */
#include <sys/ioctl.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/time.h>
#include <unistd.h>
#include <pthread.h>

/* driver pcan pci for Peak board */
//#include "libpcan.h"
//#include "pcan.h"

#include <applicfg.h>
#include "timer.h"
#include "can_driver.h"
#include "timers_driver.h"

#ifndef extra_PCAN_init_params
	#define extra_PCAN_init_params /**/
#else
	#define extra_PCAN_init_params\
		,getenv("PCANHwType") ? strtol(getenv("PCANHwType"),NULL,0):0\
		,getenv("PCANIO_Port") ? strtol(getenv("PCANIO_Port"),NULL,0):0\
		,getenv("PCANInterupt") ? strtol(getenv("PCANInterupt"),NULL,0):0
#endif

#ifdef PCAN2_HEADER_
#define MAX_NB_CAN_PORTS 2
#else
#define MAX_NB_CAN_PORTS 1
#endif

typedef struct
{
	char used;
	TASK_HANDLE receiveTask;
	CO_Data *d;
	s_BOARD *board;
} CANPort;

CANPort canports[MAX_NB_CAN_PORTS] = { {0,}, };

pthread_mutex_t PeakCan_mutex = PTHREAD_MUTEX_INITIALIZER;

// Define for rtr CAN message
#define CAN_INIT_TYPE_ST_RTR MSGTYPE_STANDARD | MSGTYPE_RTR

void
canInit (CAN_HANDLE fd0)
{
#ifdef PCAN2_HEADER_
	// if not the first handler
	if(canports != ((CANPort *) fd0))
		CAN2_Init (((CANPort *) fd0)->board->baudrate,
			  CAN_INIT_TYPE_ST extra_PCAN_init_params);
	else
#endif
		CAN_Init (((CANPort *) fd0)->board->baudrate,
			  CAN_INIT_TYPE_ST extra_PCAN_init_params);
}

/*********functions which permit to communicate with the board****************/
UNS8
canReceive (CAN_HANDLE fd0, Message * m)
{
	UNS8 data;
	TPCANMsg peakMsg;

	DWORD Res;

	// We read the queue looking for messages.
	// 
	pthread_mutex_lock (&PeakCan_mutex);

#ifdef PCAN2_HEADER_
	// if not the first handler
	if(canports != ((CANPort *) fd0))
		Res = CAN2_Read (&peakMsg);
	else
#endif
		Res = CAN_Read (&peakMsg);

	// A message was received
	// We process the message(s)
	// 
	if (Res == CAN_ERR_OK)
	{
		// if something different that 11bit or rtr... problem
		if (peakMsg.MSGTYPE & ~(MSGTYPE_STANDARD | MSGTYPE_RTR))
		{
			if (peakMsg.MSGTYPE == CAN_ERR_BUSOFF)
			{
				printf ("!!! Peak board read : re-init\n");
				canInit(fd0);
				usleep (10000);
			}

			// If status, return status if 29bit, return overrun
			pthread_mutex_unlock (&PeakCan_mutex);
			return peakMsg.MSGTYPE ==
				MSGTYPE_STATUS ? peakMsg.DATA[2] : CAN_ERR_OVERRUN;
		}
		m->cob_id.w = peakMsg.ID;
		if (peakMsg.MSGTYPE == CAN_INIT_TYPE_ST)	/* bits of MSGTYPE_ */
			m->rtr = 0;
		else
			m->rtr = 1;
		m->len = peakMsg.LEN;	/* count of data bytes (0..8) */
		for (data = 0; data < peakMsg.LEN; data++)
			m->data[data] = peakMsg.DATA[data];	/* data bytes, up to 8 */
	}
	pthread_mutex_unlock (&PeakCan_mutex);
	return Res;
}

void
canReceiveLoop (CAN_HANDLE fd0)
{
	CO_Data *d = ((CANPort *) fd0)->d;
	Message m;
	while (((CANPort *) fd0)->used)
	{
		DWORD Res;
		if ((Res = canReceive (fd0, &m)) == CAN_ERR_OK)
		{
			EnterMutex ();
			canDispatch (d, &m);
			LeaveMutex ();
		}
		else
		{
			if (!
				(Res & CAN_ERR_QRCVEMPTY || Res & CAN_ERR_BUSLIGHT
				 || Res & CAN_ERR_BUSHEAVY))
			{
				printf ("canReceive returned error (%d)\n", Res);
			}
			usleep (1000);
		}
	}
}

/***************************************************************************/
UNS8
canSend (CAN_HANDLE fd0, Message * m)
{
	UNS8 data;
	TPCANMsg peakMsg;
	peakMsg.ID = m->cob_id.w;	/* 11/29 bit code */
	if (m->rtr == 0)
		peakMsg.MSGTYPE = CAN_INIT_TYPE_ST;	/* bits of MSGTYPE_ */
	else
	{
		peakMsg.MSGTYPE = CAN_INIT_TYPE_ST_RTR;	/* bits of MSGTYPE_ */
	}
	peakMsg.LEN = m->len;
	/* count of data bytes (0..8) */
	for (data = 0; data < m->len; data++)
		peakMsg.DATA[data] = m->data[data];	/* data bytes, up to 8 */
	errno = CAN_ERR_OK;
	do
	{
		pthread_mutex_lock (&PeakCan_mutex);
#ifdef PCAN2_HEADER_
	// if not the first handler
	if(canports != ((CANPort *) fd0))
		errno = CAN2_Write (&peakMsg);
	else
#endif
		errno = CAN_Write (&peakMsg);

		if (errno)
		{
			if (errno == CAN_ERR_BUSOFF)
			{
				printf ("!!! Peak board write : re-init\n");
				canInit(fd0);
				usleep (10000);
			}
			pthread_mutex_unlock (&PeakCan_mutex);
			usleep (100);
		}
		else
		{
			pthread_mutex_unlock (&PeakCan_mutex);
		}
	}
	while (errno != CAN_ERR_OK && ((CANPort *) fd0)->used);
	return 0;

}

/***************************************************************************/
CAN_HANDLE
canOpen (s_BOARD * board)
{
//  HANDLE fd0 = NULL;
	char busname[64];
	char *pEnd;
	int i;

	for (i = 0; i < MAX_NB_CAN_PORTS; i++)
	{
		if (!canports[i].used)
			break;
	}
	if (i == MAX_NB_CAN_PORTS)
	{
		fprintf (stderr, "Open failed.\n");
		fprintf (stderr,
				 "can_peak_win32.c: no more can port available with this pcan library\n");
		fprintf (stderr,
				 "can_peak_win32.c: please link another executable with another pcan lib\n");
		return NULL;
	}

	canports[i].used = 1;
	canports[i].board = board;
	canports[i].d = board->d;
	
	canInit((CANPort *) & canports[i]);
	
	CreateReceiveTask ((CANPort *) & canports[i], &canports[i].receiveTask);

	return (CANPort *) & canports[i];
}

/***************************************************************************/
int
canClose (CAN_HANDLE fd0)
{
	((CANPort *) fd0)->used = 0;
#ifdef PCAN2_HEADER_
	// if not the first handler
	if(canports != ((CANPort *) fd0))
		CAN2_Close ();
	else
#endif
		CAN_Close ();

	WaitReceiveTaskEnd (&((CANPort *) fd0)->receiveTask);
	return 0;
}

