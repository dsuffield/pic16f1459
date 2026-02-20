/************************************************************************************\

  usbctrltrf.h - PIC16F1489 configuration

  (c) 2008-2016 Copyright Eckler Software

  Author: David Suffield, dsuffiel@ecklersoft.com

  History:

  See makefile.
  
\************************************************************************************/

#ifndef USBCTRLTRF_H
#define USBCTRLTRF_H

#include "typedefs.h"

/* Control Transfer States */
#define WAIT_SETUP          0
#define CTRL_TRF_NOP        0
#define CTRL_TRF_TX         1
#define CTRL_TRF_RX         2

/* USB PID: Token Types - See chapter 8 in the USB specification */
#define SETUP_TOKEN         0xd
#define OUT_TOKEN           0x1
#define IN_TOKEN            0x9

/* bmRequestType Definitions */
#define HOST_TO_DEV         0
#define DEV_TO_HOST         1

#define STANDARD            0x00
#define CLASS               0x01
#define VENDOR              0x02

#define RCPT_DEV            0
#define RCPT_INTF           1
#define RCPT_EP             2
#define RCPT_OTH            3

extern byte ctrl_trf_session_owner;

extern __near unsigned char *pSrc;
extern __near unsigned char *pDst;
extern WORD wCount;

void USBCtrlEPService(void);
void USBCtrlTrfTxService(void);
void USBCtrlTrfRxService(void);
void USBCtrlEPServiceComplete(void);
void USBPrepareForNextSetupTrf(void);

#endif //USBCTRLTRF_H
