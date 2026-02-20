/************************************************************************************\

  usbcfg.h - PIC16F1489 configuration

  (c) 2008-2016 Copyright Eckler Software

  Author: David Suffield, dsuffiel@ecklersoft.com

  History:

  See makefile.
  
\************************************************************************************/
#ifndef _USBCFG_H
#define _USBCFG_H

#define MAX_NUM_INT             1   // For tracking Alternate Setting

/*
 * MUID = Microchip USB Class ID
 * Used to identify which of the USB classes owns the current
 * session of control transfer over EP0
 */
#define MUID_NULL               0
#define MUID_USB9               1
#define MUID_RT                 2

/** E N D P O I N T S  A L L O C A T I O N **************************/

#define EP0_BUFF_SIZE 8   // 8, 16, 32, or 64
#define RT_EP 1
#define RT_INTF_ID 0x00
#define RT_UEP UEP1
#define RT_BD_OUT bdt_data.ep_bd_pairs[RT_EP].ep_bd_out
#define RT_BD_IN bdt_data.ep_bd_pairs[RT_EP].ep_bd_in
#define RT_EP_SIZE 64
#define RT_EP_OUT EP01_OUT
#define RT_EP_IN EP01_IN
#define MAX_EP_NUMBER 1  // EP1

#define MAX_CD_NUMBER 2   /* max configuration descriptor, [0] is unused */
#define MAX_SD_NUMBER 4   /* max string descriptor */

//#define OS_VENDOR_CODE 'a' // We use a printable ASCII character for convenience
//#define OS_VENDOR_CODE 'b' // release with Microsoft OS descriptors.
//#define OS_VENDOR_CODE 'c' // release with xc8 v1.38 compilier, OUTPUT0-1, suspend.
//#define OS_VENDOR_CODE 'd' // release with INPUT0 and OUTPUT0 swapped for TIMER1 support. INPUT0 frequency counter.
#define OS_VENDOR_CODE 'e' // release with INPUT0 sync_start support.

/* See /opt/microchip/xc8/v1.38/include/pic16f1459.h for following register definitions. */

#define LED_TRIS  TRISBbits.TRISB7
#define LED_PIN   LATBbits.LATB7
//#define PP_INPUT0_TRIS TRISBbits.TRISB4
//#define PP_INPUT0_PIN PORTBbits.RB4
//#define PP_INPUT0_WPUB WPUBbits.WPUB4
#define PP_INPUT0_TRIS TRISAbits.TRISA4
#define PP_INPUT0_PIN PORTAbits.RA4
#define PP_INPUT0_WPUB WPUAbits.WPUA4
#define PP_INPUT1_TRIS TRISAbits.TRISA5
#define PP_INPUT1_PIN PORTAbits.RA5
#define PP_INPUT1_WPUB WPUAbits.WPUA5
#define PP_INPUT2_TRIS TRISBbits.TRISB6
#define PP_INPUT2_PIN PORTBbits.RB6
#define PP_INPUT2_WPUB WPUBbits.WPUB6
//#define PP_OUTPUT0_TRIS TRISAbits.TRISA5
//#define PP_OUTPUT0_PIN LATAbits.LATA5
#define PP_OUTPUT0_TRIS TRISBbits.TRISB4
#define PP_OUTPUT0_PIN LATBbits.LATB4
#define PP_OUTPUT1_TRIS TRISBbits.TRISB5
#define PP_OUTPUT1_PIN LATBbits.LATB5
#define UCFG_VAL _UPUEN|_FSEN

/* 
 * We don't really need a USB bus sense unless our application needs to detect when 
 * the USB is plugged in. This is for self-power devices which can run without the USB 
 * connected (like an mp3 player for example). 
 *
 * Simulate bus sense with a bit (WPUB4) that is always be set high.
 */
#define USB_BUS_SENSE_PIN PP_INPUT0_WPUB

#endif //_USBCFG_H
