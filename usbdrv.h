/************************************************************************************\

  usbdrv.h - PIC16Fxxxx USB driver

  (c) 2008-2016 Copyright Eckler Software

  Author: David Suffield, dsuffiel@ecklersoft.com

  History:

  See makefile.
  
\************************************************************************************/

#ifndef _USBDRV_H
#define _USBDRV_H

#define Sleep()  asm("sleep")

/* Buffer Descriptor Status Register Initialization Parameters */
#define _BSTALL     0x04                //Buffer Stall enable
#define _DTSEN      0x08                //Data Toggle Synch enable
#define _DAT0       0x00                //DATA0 packet expected next
#define _DAT1       0x40                //DATA1 packet expected next
#define _DTSMASK    0x40                //DTS Mask
#define _USIE       0x80                //SIE owns buffer
#define _UCPU       0x00                //CPU owns buffer

/* UEPn bits */
#define _EPSTALL                0x01
#define _EPINEN                 0x02
#define _EPOUTEN                0x04
#define _EPCONDIS               0x08
#define _EPHSHK                 0x10

/* UCFG bits */
#define _PPB0                   0x01
#define _PPB1                   0x02
#define _FSEN                   0x04
#define _UPUEN                  0x10
#define _UTEYE                  0x80

/* USB Device States - To be used with [byte usb_device_state] */
#define DETACHED_STATE          0
#define ATTACHED_STATE          1
#define POWERED_STATE           2
#define DEFAULT_STATE           3
#define ADR_PENDING_STATE       4
#define ADDRESS_STATE           5
#define CONFIGURED_STATE        6

/* Memory Types for Control Transfer - used in USB_DEVICE_STATUS */
#define _RAM 0
#define _ROM 1

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

/* UEPn Initialization Parameters */
#define EP_CTRL     _EPINEN|_EPOUTEN  // Cfg Control pipe for this ep
#define EP_OUT      _EPCONDIS|_EPOUTEN // Cfg OUT only pipe for this ep
#define EP_IN       _EPCONDIS|_EPINEN  // Cfg IN only pipe for this ep
#define EP_OUT_IN   _EPCONDIS|_EPOUTEN|_EPINEN  // Cfg both OUT & IN pipes for this ep
#define HSHK_EN     _EPHSHK        // Enable handshake packet
                                    // Handshake should be disable for isoch

/******************************************************************************
 * Standard Request Codes
 * USB 2.0 Spec Ref Table 9-4
 *****************************************************************************/
#define GET_STATUS  0
#define CLR_FEATURE 1
#define SET_FEATURE 3
#define SET_ADR     5
#define GET_DSC     6
#define SET_DSC     7
#define GET_CFG     8
#define SET_CFG     9
#define GET_INTF    10
#define SET_INTF    11
#define SYNCH_FRAME 12

/* Standard Feature Selectors */
#define DEVICE_REMOTE_WAKEUP    0x01
#define ENDPOINT_HALT           0x00
                                   
/******************************************************************************
 * USB - PICmicro Endpoint Definitions
 * PICmicro EP Address Format: X:EP3:EP2:EP1:EP0:DIR:PPBI:X
 * This is used when checking the value read from USTAT
 *
 * NOTE: These definitions are not used in the descriptors.
 * EP addresses used in the descriptors have different format and
 * are defined in: "system\usb\usbdefs\usbdefs_std_dsc.h"
 *****************************************************************************/
#define OUT         0
#define IN          1

#define EP00_OUT    ((0x00<<3)|(OUT<<2))
#define EP00_IN     ((0x00<<3)|(IN<<2))
#define EP01_OUT    ((0x01<<3)|(OUT<<2))
#define EP01_IN     ((0x01<<3)|(IN<<2))

/******************************************************************************
 * Macro:           void mInitializeUSBDriver(void)
 *
 * Overview:        Configures the USB module, definition of UCFG_VAL can be
 *                  found in autofiles\usbcfg.h
 *
 *                  This register determines: USB Speed, On-chip pull-up
 *                  resistor selection, On-chip tranceiver selection, bus
 *                  eye pattern generation mode, Ping-pong buffering mode
 *                  selection.
 *****************************************************************************/
#define mInitializeUSBDriver() {UCFG=UCFG_VAL; usb_device_state=DETACHED_STATE; usb_stat._byte=0; usb_active_cfg=0;}

/******************************************************************************
 * Macro:           void mDisableEP1to15(void)
 *
 * Overview:        This macro disables all endpoints except EP0.
 *                  This macro should be called when the host sends a RESET
 *                  signal or a SET_CONFIGURATION request.
 *
 * Note:            PIC16F1459 has 8 bi-directional endpoints, PIC18F2455 has 16. 
 *****************************************************************************/
#define mDisableEP1to7()       ClearArray((byte*)&UEP1,7);
//#define mDisableEP1to15()       ClearArray((uint8_t*)&UEP1,15);

/******************************************************************************
 * CTRL_TRF_SETUP:
 *
 * Every setup packet has 8 bytes.
 * However, the buffer size has to equal the EP0_BUFF_SIZE value specified
 * in autofiles\usbcfg.h
 * The value of EP0_BUFF_SIZE can be 8, 16, 32, or 64.
 *
 * First 8 bytes are defined to be directly addressable to improve speed
 * and reduce code size.
 * Bytes beyond the 8th byte have to be accessed using indirect addressing.
 *****************************************************************************/
typedef union _CTRL_TRF_SETUP
{
    /** Array for indirect addressing ****************************************/
    struct
    {
        byte _byte[EP0_BUFF_SIZE];
    };
    
    /** Standard Device Requests *********************************************/
    struct
    {
        byte bmRequestType;
        byte bRequest;    
        word wValue;
        word wIndex;
        word wLength;
    };
    struct
    {
        unsigned :8;
        unsigned :8;
        WORD W_Value;
        WORD W_Index;
        WORD W_Length;
    };
    struct
    {
        unsigned Recipient:5;           //Device,Interface,Endpoint,Other
        unsigned RequestType:2;         //Standard,Class,Vendor,Reserved
        unsigned DataDir:1;             //Host-to-device,Device-to-host
        unsigned :8;
        byte bFeature;                  //DEVICE_REMOTE_WAKEUP,ENDPOINT_HALT
        unsigned :8;
        unsigned :8;
        unsigned :8;
        unsigned :8;
        unsigned :8;
    };
    struct
    {
        unsigned :8;
        unsigned :8;
        byte bDscIndex;                 //For Configuration and String DSC Only
        byte bDscType;                  //Device,Configuration,String
        word wLangID;                   //Language ID
        unsigned :8;
        unsigned :8;
    };
    struct
    {
        unsigned :8;
        unsigned :8;
        BYTE bDevADR;                   //Device Address 0-127
        byte bDevADRH;                  //Must equal zero
        unsigned :8;
        unsigned :8;
        unsigned :8;
        unsigned :8;
    };
    struct
    {
        unsigned :8;
        unsigned :8;
        byte bCfgValue;                 //Configuration Value 0-255
        byte bCfgRSD;                   //Must equal zero (Reserved)
        unsigned :8;
        unsigned :8;
        unsigned :8;
        unsigned :8;
    };
    struct
    {
        unsigned :8;
        unsigned :8;
        byte bAltID;                    //Alternate Setting Value 0-255
        byte bAltID_H;                  //Must equal zero
        byte bIntfID;                   //Interface Number Value 0-255
        byte bIntfID_H;                 //Must equal zero
        unsigned :8;
        unsigned :8;
    };
    struct
    {
        unsigned :8;
        unsigned :8;
        unsigned :8;
        unsigned :8;
        byte bEPID;                     //Endpoint ID (Number & Direction)
        byte bEPID_H;                   //Must equal zero
        unsigned :8;
        unsigned :8;
    };
    struct
    {
        unsigned :8;
        unsigned :8;
        unsigned :8;
        unsigned :8;
        unsigned EPNum:4;               //Endpoint Number 0-15
        unsigned :3;
        unsigned EPDir:1;               //Endpoint Direction: 0-OUT, 1-IN
        unsigned :8;
        unsigned :8;
        unsigned :8;
    };
    /** End: Standard Device Requests ****************************************/
    
} CTRL_TRF_SETUP;

/******************************************************************************
 * CTRL_TRF_DATA:
 *
 * Buffer size has to equal the EP0_BUFF_SIZE value specified
 * in autofiles\usbcfg.h
 * The value of EP0_BUFF_SIZE can be 8, 16, 32, or 64.
 *
 * First 8 bytes are defined to be directly addressable to improve speed
 * and reduce code size.
 * Bytes beyond the 8th byte have to be accessed using indirect addressing.
 *****************************************************************************/
typedef union _CTRL_TRF_DATA
{
    /** Array for indirect addressing ****************************************/
    struct
    {
        byte _byte[EP0_BUFF_SIZE];
    };
    
    /** First 8-byte direct addressing ***************************************/
    struct
    {
        byte _byte0;
        byte _byte1;
        byte _byte2;
        byte _byte3;
        byte _byte4;
        byte _byte5;
        byte _byte6;
        byte _byte7;
    };
    struct
    {
        word _word0;
        word _word1;
        word _word2;
        word _word3;
    };

} CTRL_TRF_DATA;

typedef union _USB_DEVICE_STATUS
{
    byte _byte;
    struct
    {
        unsigned RemoteWakeup:1;// [0]Disabled [1]Enabled: See usbdrv.c,usb9.c
        unsigned ctrl_trf_mem:1;// [0]RAM      [1]ROM
    };
} USB_DEVICE_STATUS;

typedef union _BD_STAT
{
    byte _byte;
    struct{
        unsigned BC8:1;
        unsigned BC9:1;
        unsigned BSTALL:1;              //Buffer Stall Enable
        unsigned DTSEN:1;               //Data Toggle Synch Enable
        unsigned :1;              
        unsigned :1;              
        unsigned DTS:1;                 //Data Toggle Synch Value
        unsigned UOWN:1;                //USB Ownership
    };
    struct{
        unsigned :2;
        unsigned PID:4;                 //Packet Identifier
        unsigned :2;
    };
} BD_STAT;                              //Buffer Descriptor Status Register

typedef union _BDT
{
    struct
    {
        BD_STAT Stat;
        byte Cnt;
        byte ADRL;                      //Buffer Address Low
        byte ADRH;                      //Buffer Address High
    };
    struct
    {
        unsigned :8;
        unsigned :8;
        __near byte * ADR;                      //Buffer Address
    };
} BDT;                                  //Buffer Descriptor Table
typedef struct bdt_pair {
    BDT ep_bd_out;     //Endpoint BD Out
    BDT ep_bd_in;      //Endpoint BD In
} BDTPAIR;                               //Buffer Descriptor pair

typedef struct bdt_data {
	BDTPAIR ep_bd_pairs[MAX_EP_NUMBER + 1];

/******************************************************************************
 * Section B: EP0 Buffer Space
 ******************************************************************************
 * - Two buffer areas are defined:
 *
 *   A. CTRL_TRF_SETUP
 *      - Size = EP0_BUFF_SIZE as defined in autofilesSbcfg.h
 *      - Detailed data structure allows direct adddressing of bits and bytes.
 *
 *   B. CTRL_TRF_DATA
 *      - Size = EP0_BUFF_SIZE as defined in autofilesSbcfg.h
 *      - Data structure allows direct adddressing of the first 8 bytes.
 *
 * - Both data types are defined in systemSbSbdefsSbdefs_ep0_buff.h
 *****************************************************************************/
	CTRL_TRF_SETUP SetupPkt;
	CTRL_TRF_DATA CtrlTrfData;
} BDTDATA;                               //Buffer Descriptor data

extern __at(0x020) volatile BDTDATA bdt_data; //Buffer Descriptor data
extern byte usb_device_state;
extern USB_DEVICE_STATUS usb_stat;
extern byte usb_active_cfg;
extern byte usb_alt_intf[MAX_NUM_INT];

void USBCheckBusStatus(void);
void USBDriverService(void);
void USBPrepareForNextSetupTrf(void);

// temporary...
void ClearArray(byte* startAdr,byte count);

#endif //_USBDRV_H
