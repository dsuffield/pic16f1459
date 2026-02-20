/************************************************************************************\

  usb.h - PIC16F1489 usb to parallel port step/direction driver board

  (c) 2008-2016 Copyright Eckler Software

  Author: David Suffield, dsuffiel@ecklersoft.com

  History:

  See makefile.
  
\************************************************************************************/
#ifndef USB_H
#define USB_H

#include <xc.h>      // CCI definitions
#include "usbcfg.h"
#include "usbdsc.h"
#include "usbdrv.h"
#include "usbctrltrf.h"
#include "usb9.h"

void rt_check_request(void);
void rt_init_ep(void);
void blink();

#endif //USB_H
