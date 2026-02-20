/************************************************************************************\

  usb9.c - PIC16F1489 USB driver

  (c) 2008-2016 Copyright Eckler Software

  Author: David Suffield, dsuffiel@ecklersoft.com

  Some of the following software was re-purposed from Microchip 
  Technology Incorporated. The following is their license agreement.
  
  The software supplied herewith by Microchip Technology Incorporated
  (the 'Company') for its PICmicro� Microcontroller is intended and
  supplied to you, the Company's customer, for use solely and
  exclusively on Microchip PICmicro Microcontroller products. The
  software is owned by the Company and/or its supplier, and is
  protected under applicable copyright laws. All rights are reserved.
  Any use in violation of the foregoing restrictions may subject the
  user to criminal sanctions under applicable laws, as well as to
  civil liability for the breach of the terms and conditions of this
  license.
 
  THIS SOFTWARE IS PROVIDED IN AN 'AS IS' CONDITION. NO WARRANTIES,
  WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING, BUT NOT LIMITED
  TO, IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
  PARTICULAR PURPOSE APPLY TO THIS SOFTWARE. THE COMPANY SHALL NOT,
  IN ANY CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL OR
  CONSEQUENTIAL DAMAGES, FOR ANY REASON WHATSOEVER.

\************************************************************************************/

#include "usb.h"

void USBStdGetDscHandler(void);
void USBStdSetCfgHandler(void);
void USBStdGetStatusHandler(void);
void USBStdFeatureReqHandler(void);
void USBVendorGetOSHandler(void);

/******************************************************************************
 * Function:        void USBCheckStdRequest(void)
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          None
 *
 * Side Effects:    None
 *
 * Overview:        This routine checks the setup data packet to see if it
 *                  knows how to handle it
 *
 * Note:            None
 *****************************************************************************/
void USBCheckStdRequest(void)
{   
    if(bdt_data.SetupPkt.RequestType != STANDARD) return;
    
    switch(bdt_data.SetupPkt.bRequest)
    {
        case SET_ADR:
            ctrl_trf_session_owner = MUID_USB9;
            usb_device_state = ADR_PENDING_STATE;       // Update state only
            /* See USBCtrlTrfInHandler() in usbctrltrf.c for the next step */
            break;
        case GET_DSC:
            USBStdGetDscHandler();
            break;
        case SET_CFG:
            USBStdSetCfgHandler();
            break;
        case GET_CFG:
            ctrl_trf_session_owner = MUID_USB9;
            pSrc = (__near unsigned char *)&usb_active_cfg;         // Set Source
            usb_stat.ctrl_trf_mem = _RAM;               // Set memory type
            wCount._word = 1;                            // Set data count
            break;
        case GET_STATUS:
            USBStdGetStatusHandler();
            break;
        case CLR_FEATURE:
        case SET_FEATURE:
            USBStdFeatureReqHandler();
            break;
        case GET_INTF:
            ctrl_trf_session_owner = MUID_USB9;
            pSrc = (__near unsigned char *)&usb_alt_intf+bdt_data.SetupPkt.bIntfID;  // Set source
            usb_stat.ctrl_trf_mem = _RAM;               // Set memory type
            wCount._word = 1;                            // Set data count
            break;
        case SET_INTF:
            ctrl_trf_session_owner = MUID_USB9;
            usb_alt_intf[bdt_data.SetupPkt.bIntfID] = bdt_data.SetupPkt.bAltID;
            break;
        case SET_DSC:
        case SYNCH_FRAME:
        default:
            break;
    }//end switch
    
}//end USBCheckStdRequest

/******************************************************************************
 * Function:        void USBStdGetDscHandler(void)
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          None
 *
 * Side Effects:    None
 *
 * Overview:        This routine handles the standard GET_DESCRIPTOR request.
 *                  It utilizes tables dynamically looks up descriptor size.
 *                  This routine should never have to be modified if the tables
 *                  in usbdsc.c are declared correctly.
 *
 * Note:            None
 *****************************************************************************/
void USBStdGetDscHandler(void)
{
    if(bdt_data.SetupPkt.bmRequestType == 0x80)
    {
        switch(bdt_data.SetupPkt.bDscType)
        {
            case DSC_DEV:
                ctrl_trf_session_owner = MUID_USB9;
                pSrc = (__near unsigned char *)&device_dsc;
                wCount._word = sizeof(device_dsc);          // Set data count
                break;
            case DSC_CFG:
                ctrl_trf_session_owner = MUID_USB9;
                pSrc = (__near unsigned char *)*(USB_CD_Ptr+bdt_data.SetupPkt.bDscIndex);
                //wCount._word = *(((__code unsigned short *)pSrc) + 1);   // Why the +1? DES 9/11/2016
                wCount._word = *(const unsigned short *)pSrc;              // Set data count
                break;
            case DSC_STR:
                ctrl_trf_session_owner = MUID_USB9;
                if (bdt_data.SetupPkt.bDscIndex < MAX_SD_NUMBER)
                {
                   pSrc = (__near unsigned char *)*(USB_SD_Ptr+bdt_data.SetupPkt.bDscIndex);  /* descriptor is indexed by setup packet */
                   //     Bug fix. DES 8/4/2008
                   //     wCount._word = *pSrc;                  // Set data count
                   wCount._word = *(const unsigned char *)pSrc;
                }
                else if (bdt_data.SetupPkt.bDscIndex == 0xEE)
                {   //OS String Descriptor
                    pSrc = (__near unsigned char *)&usb_sd_os;
                    wCount._word = *(const unsigned char *)pSrc;
                }
#if 0
                 if (bdt_data.SetupPkt.bDscIndex == 0)
                {
                   pSrc = (__near unsigned char *)USB_SD_Ptr[0];
                   wCount._word = 4;
                }
                else if (bdt_data.SetupPkt.bDscIndex == 1)
                {
                   pSrc = (__near unsigned char *)USB_SD_Ptr[1];
                   wCount._word = 0x34;
                }
                else if (bdt_data.SetupPkt.bDscIndex == 2)
                {
                   pSrc = (__near unsigned char *)USB_SD_Ptr[2];
                   wCount._word = 0x3e;
                }
#endif
                break;
        }//end switch
        
        usb_stat.ctrl_trf_mem = _ROM;                       // Set memory type
    }//end if
}//end USBStdGetDscHandler

/******************************************************************************
 * Function:        void USBStdSetCfgHandler(void)
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          None
 *
 * Side Effects:    None
 *
 * Overview:        This routine first disables all endpoints by clearing
 *                  UEP registers. It then configures (initializes) endpoints
 *                  specified in the modifiable section.
 *
 * Note:            None
 *****************************************************************************/
void USBStdSetCfgHandler(void)
{
    ctrl_trf_session_owner = MUID_USB9;
    mDisableEP1to7();                          // See usbdrv.h
    ClearArray((byte*)&usb_alt_intf,MAX_NUM_INT);
    usb_active_cfg = bdt_data.SetupPkt.bCfgValue;
    if(bdt_data.SetupPkt.bCfgValue == 0)
        usb_device_state = ADDRESS_STATE;
    else
    {
        usb_device_state = CONFIGURED_STATE;

        /* Modifiable Section */
        rt_init_ep();
        /* End modifiable section */

    }//end if(bdt_data.SetupPkt.bcfgValue == 0)
}//end USBStdSetCfgHandler

/******************************************************************************
 * Function:        void USBStdGetStatusHandler(void)
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          None
 *
 * Side Effects:    None
 *
 * Overview:        This routine handles the standard GET_STATUS request
 *
 * Note:            None
 *****************************************************************************/
void USBStdGetStatusHandler(void)
{
    bdt_data.CtrlTrfData._byte0 = 0;                         // Initialize content
    bdt_data.CtrlTrfData._byte1 = 0;
        
    switch(bdt_data.SetupPkt.Recipient)
    {
        case RCPT_DEV:
            ctrl_trf_session_owner = MUID_USB9;
            /*
             * _byte0: bit0: Self-Powered Status [0] Bus-Powered [1] Self-Powered
             *         bit1: RemoteWakeup        [0] Disabled    [1] Enabled
             */
// No self_power. DES 2/22/09
//            if(self_power == 1)                     // self_power defined in io_cfg.h
//                bdt_data.CtrlTrfData._byte0|=0x1;    // Set bit0
            
            if(usb_stat.RemoteWakeup == 1)          // usb_stat defined in usbmmap.c
                bdt_data.CtrlTrfData._byte0|=0x2;     // Set bit1
            break;
        case RCPT_INTF:
            ctrl_trf_session_owner = MUID_USB9;     // No data to update
            break;
        case RCPT_EP:
            ctrl_trf_session_owner = MUID_USB9;
            /*
             * _byte0: bit0: Halt Status [0] Not Halted [1] Halted
             */
            pDst = (__near unsigned char *)&bdt_data.ep_bd_pairs[0].ep_bd_out+(bdt_data.SetupPkt.EPNum*8)+(bdt_data.SetupPkt.EPDir*4);
            if(*pDst & _BSTALL)    // Use _BSTALL as a bit mask
                bdt_data.CtrlTrfData._byte0=0x01;// Set bit0
            break;
    }//end switch
    
    if(ctrl_trf_session_owner == MUID_USB9)
    {
        pSrc = (__near unsigned char *)&bdt_data.CtrlTrfData;            // Set Source
        usb_stat.ctrl_trf_mem = _RAM;               // Set memory type
        wCount._word = 2;                            // Set data count
    }//end if(...)
}//end USBStdGetStatusHandler

/******************************************************************************
 * Function:        void USBStdFeatureReqHandler(void)
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          None
 *
 * Side Effects:    None
 *
 * Overview:        This routine handles the standard SET & CLEAR FEATURES
 *                  requests
 *
 * Note:            None
 *****************************************************************************/
void USBStdFeatureReqHandler(void)
{
    if((bdt_data.SetupPkt.bFeature == DEVICE_REMOTE_WAKEUP)&&
       (bdt_data.SetupPkt.Recipient == RCPT_DEV))
    {
        ctrl_trf_session_owner = MUID_USB9;
        if(bdt_data.SetupPkt.bRequest == SET_FEATURE)
            usb_stat.RemoteWakeup = 1;
        else
            usb_stat.RemoteWakeup = 0;
    }//end if
    
    if((bdt_data.SetupPkt.bFeature == ENDPOINT_HALT)&&
       (bdt_data.SetupPkt.Recipient == RCPT_EP)&&
       (bdt_data.SetupPkt.EPNum != 0))
    {
        ctrl_trf_session_owner = MUID_USB9;
        /* Must do address calculation here */
        pDst = (__near unsigned char *)&bdt_data.ep_bd_pairs[0].ep_bd_out+(bdt_data.SetupPkt.EPNum*8)+(bdt_data.SetupPkt.EPDir*4);
        
        if(bdt_data.SetupPkt.bRequest == SET_FEATURE)
            *pDst = _USIE|_BSTALL;
        else
        {
            if(bdt_data.SetupPkt.EPDir == 1) // IN
                *pDst = _UCPU;
            else
                *pDst = _USIE|_DAT0|_DTSEN;
        }//end if
    }//end if
}//end USBStdFeatureReqHandler

/********************************************************************
 * Function:        void USBCheckVendorRequest(void)
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          None
 *
 * Side Effects:    None
 *
 * Overview:        This routine checks the setup data packet to see
 *                  if it knows how to handle it
 *
 * Note:            None
 *******************************************************************/
void USBCheckVendorRequest(void)
{
    if(bdt_data.SetupPkt.RequestType != VENDOR) return;

    switch(bdt_data.SetupPkt.bRequest)
    {
        case OS_VENDOR_CODE:
            USBVendorGetOSHandler();
            break;
        case SYNCH_FRAME:
        default:
            break;
    }//end switch
}//end USBCheckVendorRequest

/********************************************************************
 * Function:        void USBVendorGetOSHandler(void)
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          None
 *
 * Side Effects:    None
 *
 * Overview:        This routine handles OS Vendor Request for MS/WinUSB
 *
 * Note:            None
 *******************************************************************/
void USBVendorGetOSHandler(void)
{
    if(bdt_data.SetupPkt.bmRequestType | 0x80)
    {
        switch(bdt_data.SetupPkt.wIndex)
        {
            case 0x04:
                ctrl_trf_session_owner = MUID_USB9;
                pSrc = (__near unsigned char *)&ext_cid_os_fd;
                wCount._word = *(const unsigned char *)pSrc;     // truncate Little Endian DWORD to a WORD
                break;                   
            case 0x05:
                ctrl_trf_session_owner = MUID_USB9;
                pSrc = (__near unsigned char *)&ext_p_os_fd;
                wCount._word = *(const unsigned char *)pSrc;
                break;
        }//end switch
    }//end if
}//end USBVendorGetOSHandler

