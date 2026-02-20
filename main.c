/************************************************************************************\

  main.c - PIC16F1489 usb to parallel port step/direction driver board

  (c) 2008-2016 Copyright Eckler Software

  Author: David Suffield, dsuffiel@ecklersoft.com

  The host will convert g-code to a step/direction byte array, calculate the acceleration/deceleration
  ramp, generate a step/direction byte array, then write the byte array to the driver board. The driver
  board will buffer the step/direction bytes from the host and clock the bytes out to PORTC.

  The step/direction bytes will be clocked out at a maximum rate of 46875hz.

  The host will handle ABORT command from the user. The ABORT will stop immediately with no valid
  stop position.

  History:

  See makefile.
  
\************************************************************************************/

#include <stdint.h>
#include "usb.h"

/* EP0 Vendor Setup commands (bRequest). */
enum STEP_CMD
{
   STEP_SET,     /* set step elements, clear state bits, clear running step count */
   STEP_QUERY,   /* query current step and state info */
   STEP_ABORT_SET,   /* set un-synchronized stop */
   STEP_ABORT_CLEAR,   /* clear un-synchronized stop */
   STEP_OUTPUT0_SET,
   STEP_OUTPUT0_CLEAR,
   STEP_OUTPUT1_SET,
   STEP_OUTPUT1_CLEAR,
   STEP_SYNC_START_SET,  /* set synchronized start */ 
};

struct step_state
{
   union 
   {
      uint16_t _word;      
      struct 
      {
         unsigned abort:1;   /* 1=yes, 0=no */
         unsigned empty:1;   /* 1=yes, 0=no */
//         unsigned stall:1;   /* stalled */
         unsigned sync_start:1;   /* 1=yes, 0=no */
         unsigned input0:1;
         unsigned input1:1;
         unsigned input2:1;
         unsigned output0:1;
         unsigned output1:1;
         unsigned :1;
         unsigned :1;
         unsigned :1;
         unsigned :1;
         unsigned :1;
         unsigned :1;
         unsigned :1;
         unsigned :1;
      };
   };
};

struct step_elements
{
   char reserved[8];
};

struct step_cal_elements
{
   uint16_t lfosc;   /* LFINTOSC calibration value */
   char reserved[6];
};

struct step_query
{
   union
   {
      struct
      {
         struct step_state state_bits;
         //   unsigned char reserved;
         uint16_t icount_period;     /* input0 period in counts */
         uint32_t step;              /* running step count */
      };
      struct step_cal_elements calibration;
   };
};

#define BANK_MAX 5

/* The 16f1459 has 5 usable 64byte banks. The 18f2455 had 8 usable 64byte banks. */
__at(0x0a0) volatile uint8_t rt_buf_rx0[RT_EP_SIZE]; /* Bulk Out EP buffer */
__at(0x120) volatile uint8_t rt_buf_rx1[RT_EP_SIZE];  /* Bulk Out EP buffer */
__at(0x1a0) volatile uint8_t rt_buf_rx2[RT_EP_SIZE];  /* Bulk Out EP buffer */
__at(0x220) volatile uint8_t rt_buf_rx3[RT_EP_SIZE];  /* Bulk Out EP buffer */
__at(0x2a0) volatile uint8_t rt_buf_rx4[RT_EP_SIZE];  /* Bulk Out EP buffer */

static unsigned char buf_cnt0;
static unsigned char buf_cnt1;
static unsigned char buf_cnt2;
static unsigned char buf_cnt3;
static unsigned char buf_cnt4;

static __near unsigned char *pbuf0;
static __near unsigned char *pbuf1;
static __near unsigned char *pbuf2;
static __near unsigned char *pbuf3;
static __near unsigned char *pbuf4;

//static unsigned char stall_cnt;
//static unsigned char stalled;
//static unsigned char trip_cnt;

#define DEBOUNCE_MAX 6

static uint32_t step_cnt;
static uint32_t led_cnt;
static uint16_t icount;      /* current input0 count */
static uint16_t last_icount;  /* previous input0 count */
static uint16_t icount_period;    /* input0 frequency */
static unsigned char new_gate;
static unsigned char old_gate;
//static uint16_t lf_calibration; /* LFINTOSC (31khz) calibration value */
static uint16_t t0clk;         /* timer0 overflow */
static unsigned char sync_start;  /* synchronize start transfer (1=true, 0=false). */
static unsigned char debounce;    /* input0 debounce counter */

static unsigned char bank_write;   /* usb write pointer */
static unsigned char bank_read;   /* interrupt read pointer */ 
static unsigned char next_bank;   /* used to calculate next write pointer */

static struct step_elements elements;
static struct step_query query_response; 
static struct step_state state_bits;

/* See file:///opt/microchip/xc8/v1.38/docs/chips/16f1459.html for config definitions. */
#pragma config FOSC=INTOSC, WDTE=OFF, PWRTE=OFF, MCLRE=ON, CP=ON, BOREN=ON, CLKOUTEN=OFF, IESO=OFF
#pragma config FCMEN=OFF, WRT=ALL, CPUDIV=NOCLKDIV, USBLSCLK=48MHz, PLLMULT=3x, PLLEN=ENABLED
#pragma config STVREN=OFF, BORV=HI, LPBOR=ON, LVP=OFF

/*
 * Primary Oscilator = 20mhz
 * CPU instructon cycle = 48mhz/4 = 12mhz = 83ns (HSPLL)
 * Timer0 clock = 12mhz
 * Timer0 overflow = 12mhz/256 = 46875hz = 21.333us
 *
 */

void timer0_handler(void) __interrupt 0
{
   /* Timer0 overflow. */

   if (state_bits.sync_start == 0)
   {
   switch (bank_read)
   {
      case 0:
         if (buf_cnt0)
         {
            if (!state_bits.abort)
            {
               LATC = *pbuf0;      /* write step */ 
               step_cnt++;
            }
            pbuf0++;
            buf_cnt0--;
            if (buf_cnt0==0)
               bank_read=1;     /* bump to next bank */
         }
         break;
      case 1:
         if (buf_cnt1)
         {
            if (!state_bits.abort)
            {
               LATC = *pbuf1;      /* write step */ 
               step_cnt++;
            }
            pbuf1++;
            buf_cnt1--;
            if (buf_cnt1==0)
               bank_read=2;     /* bump to next bank */
         }
         break;
      case 2:
         if (buf_cnt2)
         {
            if (!state_bits.abort)
            {
               LATC = *pbuf2;      /* write step */ 
               step_cnt++;
            }
            pbuf2++;
            buf_cnt2--;
            if (buf_cnt2==0)
               bank_read=3;     /* bump to next bank */
         }
         break;
      case 3:
         if (buf_cnt3)
         {
            if (!state_bits.abort)
            {
               LATC = *pbuf3;      /* write step */ 
               step_cnt++;
            }
            pbuf3++;
            buf_cnt3--;
            if (buf_cnt3==0)
               bank_read=4;     /* bump to next bank */
         }
         break;
      case 4:
         if (buf_cnt4)
         {
            if (!state_bits.abort)
            {
               LATC = *pbuf4;      /* write step */ 
               step_cnt++;
            }
            pbuf4++;
            buf_cnt4--;
            if (buf_cnt4==0)
               bank_read=0;     /* bump to next bank */
         }
         break;
      default:
         break;
   }  /* end switch (bank_read) */
   }  /* end if (sync_start == 0) */

   /* Measure input0 frequency period in counts. */
   t0clk++;
   new_gate = PP_INPUT0_PIN;
   if (new_gate==0 && old_gate==1)
   {
      if (++debounce >= DEBOUNCE_MAX)
      {
         /* Found gate high to low transistion read 16-bit value. */
         icount = t0clk;
         icount_period = icount - last_icount;
         last_icount = icount;
         debounce = 0;

         /* If syncronized start is enabled, auto reset when step buffers are empty. */
         if (state_bits.sync_start)
            if (!(buf_cnt0==0 && buf_cnt1==0 && buf_cnt2==0 && buf_cnt3==0 && buf_cnt4==0))
               state_bits.sync_start = 0;
      }
      else
      {
         goto jmpout;  /* complete transistion debounce */
      }
   }
   else
   {
      debounce = 0;
   }
   old_gate = new_gate;
   
#if 0
   /* Using input0 as timer1 gate measure period. */ 
   new_gate = T1GCONbits.T1GVAL;
   if (new_gate==0 && old_gate==1)
   {
      /* Found gate high to low transtion read timer1 16-bit value. */
      icount = TMR1;
      icount_period = icount - last_icount;
      last_icount = icount; 
   }
   old_gate = new_gate;
#endif

jmpout:
   INTCONbits.TMR0IF = 0;   /* Reset the Timer0 interrupt pending flag */
}  /* timer0_handler() */

void blink()
{
   unsigned char i=16;  /* blink i/2 times */
   while (i)
   {
      /* Blink LED momentary. */
      led_cnt++;
      if (led_cnt > 40160)
      {
         if (LED_PIN)
            LED_PIN = 0;
         else
            LED_PIN = 1;
         led_cnt=0;
         i--;
      }
   }
}

/* Arm EP OUT for next bulk transaction. */
static void rt_prepare_for_next_setup_trf(void)
{
    switch (bank_write)
    {
       case 0:
          bdt_data.ep_bd_pairs[RT_EP].ep_bd_out.Cnt = RT_EP_SIZE;   // Defined in usbcfg.h    
          bdt_data.ep_bd_pairs[RT_EP].ep_bd_out.ADR = (__near unsigned char *)rt_buf_rx0;
          bdt_data.ep_bd_pairs[RT_EP].ep_bd_out.Stat._byte = _USIE; 
          break;
       case 1:
          bdt_data.ep_bd_pairs[RT_EP].ep_bd_out.Cnt = RT_EP_SIZE;    
          bdt_data.ep_bd_pairs[RT_EP].ep_bd_out.ADR = (__near unsigned char *)rt_buf_rx1;
          bdt_data.ep_bd_pairs[RT_EP].ep_bd_out.Stat._byte = _USIE;
          break;
       case 2:
          bdt_data.ep_bd_pairs[RT_EP].ep_bd_out.Cnt = RT_EP_SIZE;    
          bdt_data.ep_bd_pairs[RT_EP].ep_bd_out.ADR = (__near unsigned char *)rt_buf_rx2;
          bdt_data.ep_bd_pairs[RT_EP].ep_bd_out.Stat._byte = _USIE;
          break;
       case 3:
          bdt_data.ep_bd_pairs[RT_EP].ep_bd_out.Cnt = RT_EP_SIZE;    
          bdt_data.ep_bd_pairs[RT_EP].ep_bd_out.ADR = (__near unsigned char *)rt_buf_rx3;
          bdt_data.ep_bd_pairs[RT_EP].ep_bd_out.Stat._byte = _USIE;
          break;
       case 4:
          bdt_data.ep_bd_pairs[RT_EP].ep_bd_out.Cnt = RT_EP_SIZE;    
          bdt_data.ep_bd_pairs[RT_EP].ep_bd_out.ADR = (__near unsigned char *)rt_buf_rx4;
          bdt_data.ep_bd_pairs[RT_EP].ep_bd_out.Stat._byte = _USIE;
          break;
       default:
          break;
    }
}  /* rt_prepare_for_next_setup_trf() */

static void rt_init(void)
{
   /* Note, init global memory manually (not guaranteed to be zero). */

   state_bits._word = 0;
   buf_cnt0 = 0;
   buf_cnt1 = 0;
   buf_cnt2 = 0;
   buf_cnt3 = 0;
   buf_cnt4 = 0;
   bank_read = 0;
   bank_write = 0;
   step_cnt = 0;
//   stalled = 0;
//   stall_cnt = 0;
//   trip_cnt = 0;
   next_bank = 0;
   last_icount = 0;
   icount_period = 0;

#if 0
   /* Reset timer1 counter. */
   T1CONbits.TMR1ON = 0;  // disable counter
   TMR1L = 0;
   TMR1H = 0;
   T1CONbits.TMR1ON = 1;  // enable counter
#endif
}

/* Class specific callback for EP0 Setup requests. Called when no other module in the firmware framework owns the request. */
void rt_check_request(void)
{
   if(bdt_data.SetupPkt.RequestType != VENDOR)
      return;
    
   switch(bdt_data.SetupPkt.bRequest)
   {
       case STEP_ABORT_SET:
          state_bits.abort = 1;
          ctrl_trf_session_owner = MUID_RT;
          break;
       case STEP_ABORT_CLEAR:
          state_bits.abort = 0;
          ctrl_trf_session_owner = MUID_RT;
          break;
       case STEP_QUERY:
          query_response.state_bits._word = state_bits._word;
          if (buf_cnt0==0 && buf_cnt1==0 && buf_cnt2==0 && buf_cnt3==0 && buf_cnt4==0)
             query_response.state_bits.empty = 1;
//          query_response.state_bits.stall = stalled;
          query_response.state_bits.input0 = PP_INPUT0_PIN;
          query_response.state_bits.input1 = PP_INPUT1_PIN;
          query_response.state_bits.input2 = PP_INPUT2_PIN;
          query_response.state_bits.output0 = PP_OUTPUT0_PIN;
          query_response.state_bits.output1 = PP_OUTPUT1_PIN;
//          query_response.trip_cnt = trip_cnt;
          query_response.icount_period = icount_period;
          query_response.step = step_cnt;
          ctrl_trf_session_owner = MUID_RT;
          pSrc = (__near unsigned char *)&query_response;  /* set source for next IN session */
          usb_stat.ctrl_trf_mem = _RAM;               // Set memory type
          wCount._word = sizeof(query_response);      // Set data count
          break;
       case STEP_OUTPUT0_SET:
          PP_OUTPUT0_PIN = 1;
          ctrl_trf_session_owner = MUID_RT;
          break;
       case STEP_OUTPUT0_CLEAR:
          PP_OUTPUT0_PIN = 0;
          ctrl_trf_session_owner = MUID_RT;
          break;
       case STEP_OUTPUT1_SET:
          PP_OUTPUT1_PIN = 1;
          ctrl_trf_session_owner = MUID_RT;
          break;
       case STEP_OUTPUT1_CLEAR:
          PP_OUTPUT1_PIN = 0;
          ctrl_trf_session_owner = MUID_RT;
          break;
       case STEP_SYNC_START_SET:
          state_bits.sync_start = 1;          /* enable synchronized transfer using input0 index pulse */
          ctrl_trf_session_owner = MUID_RT;
          break;
       case STEP_SET:
          rt_init();
          pDst = (__near unsigned char *)&elements;  /* set destination for next OUT session */
          usb_stat.ctrl_trf_mem = _RAM;               // Set memory type
          wCount._word = sizeof(elements);            // Set data count
          ctrl_trf_session_owner = MUID_RT;
          break;
#if 0
       case STEP_CAL_QUERY:
          query_response.calibration.lfosc = lf_calibration;
          ctrl_trf_session_owner = MUID_RT;
          pSrc = (__near unsigned char *)&query_response;  /* set source for next IN session */
          usb_stat.ctrl_trf_mem = _RAM;               // Set memory type
          wCount._word = sizeof(query_response);      // Set data count          
          break;
#endif
       default:
          break;
    }
}  /* rt_check_request() */

/* 
 * Class specific callback for initializing endpoints, buffer descriptors, internal state-machine, and variables.
 * Called after the USB host has sent a SET_CONFIGURATION request. 
 */
void rt_init_ep(void)
{
   rt_init();
   LED_PIN = 1;                /* Enumerated, set LED on. */
   RT_UEP = EP_OUT | HSHK_EN;         // Init EP, see usbdrv.h
   rt_prepare_for_next_setup_trf();
   bdt_data.ep_bd_pairs[RT_EP].ep_bd_in.Stat._byte = _UCPU;          
}

static void rt_class_service(void)
{
   /* Check Buffer Descriptor Table ownership. */
   if (bdt_data.ep_bd_pairs[RT_EP].ep_bd_out.Stat.UOWN == 0)
   {
      /* Save completed bank write. */
      switch (bank_write)
      {
         case 0:
            pbuf0 = (__near unsigned char *)rt_buf_rx0;
            buf_cnt0 = bdt_data.ep_bd_pairs[RT_EP].ep_bd_out.Cnt;
            next_bank = 1;  /* bump to next bank write */
            break;
         case 1:
            pbuf1 = (__near unsigned char *)rt_buf_rx1;
            buf_cnt1 = bdt_data.ep_bd_pairs[RT_EP].ep_bd_out.Cnt;
            next_bank = 2;
            break;
         case 2:
            pbuf2 = (__near unsigned char *)rt_buf_rx2;
            buf_cnt2 = bdt_data.ep_bd_pairs[RT_EP].ep_bd_out.Cnt;
            next_bank = 3;
            break;
         case 3:
            pbuf3 = (__near unsigned char *)rt_buf_rx3;
            buf_cnt3 = bdt_data.ep_bd_pairs[RT_EP].ep_bd_out.Cnt; // ??
            next_bank = 4;
            break;
         case 4:
            pbuf4 = (__near unsigned char *)rt_buf_rx4;
            buf_cnt4 = bdt_data.ep_bd_pairs[RT_EP].ep_bd_out.Cnt;
            next_bank = 0;
            break;
         default:
            break;
      }

      /* If next bank is empty start a new transfer. */
      if (!(next_bank == bank_read))
      {
         bank_write = next_bank;
         rt_prepare_for_next_setup_trf();   /* initiate new transfer */
      }
   }
   else
   {
      /* Nothing to do, see if device has enumerated. */
      if (usb_device_state != CONFIGURED_STATE)
      {
         /* No enumeration, blink LED. */
         led_cnt++;
         if (led_cnt > 40160)
         {
            if (LED_PIN)
               LED_PIN = 0;
            else
               LED_PIN = 1;
            led_cnt=0;
         }
      }
   }
}  /* rt_class_service() */

/* Save following calibration code, it works great! DES 8/2/2017 */
#if 0
/* Calibrate LFINTOSC using the HFINTOSC. */
static void rt_cal()
{
   /* Configure timer0 (12mhz/65536 = 183.1054hz = 5.461333ms = gate) */
   OPTION_REGbits.TMR0CS = 0;   // FOSC/4 (12mhz)
   OPTION_REGbits.TMR0SE = 0;   // increment on low to high edge
   OPTION_REGbits.PS = 7;       // prescaler 1:256
   OPTION_REGbits.PSA = 0;      // enable prescaler

   /* Configure timer1 (31khz = clock) */
   T1CONbits.TMR1CS = 3;  // select internal clocking, LFINTOSC (31khz)
   T1CONbits.T1OSCEN = 0;  // no dedicated timer1 oscilator
   T1GCONbits.TMR1GE = 1;  // gate function on 
   T1GCONbits.T1GPOL = 1;  // timer1 count when gate is high 
   T1GCONbits.T1GTM = 1;  // gate toggle mode on 
   T1GCONbits.T1GSPM = 0;  // gate sisgle-pulse mode off 
   T1GCONbits.T1GSS = 1;  // use timer0 as the gate 
   T1CONbits.T1CKPS = 0;   // prescaler = divide by 1
   T1CONbits.nT1SYNC = 0;   // synchronize asynchronous clock input

   /* Zero timer1 and start counting. */
   T1CONbits.TMR1ON = 0;  // disable counter
   TMR1L = 0;
   TMR1H = 0;
   T1CONbits.TMR1ON = 1;  // enable counter

   /* Wait for gate toggle transition on timer1. */
   old_gate = 0;
   while (1)
   {
      new_gate = T1GCONbits.T1GVAL;
      if (new_gate==0 && old_gate==1)
      {
            /* Found gate high to low transition read timer1 16-bit value. */
            lf_calibration = TMR1;
            break;
      }
      old_gate = new_gate;
   }
}
#endif

void main()
{
   /* Make sure all interrupts are disabled. */
   INTCONbits.GIE = 0;
   INTCONbits.PEIE = 0;
   INTCONbits.INTE = 0;
   INTCONbits.IOCIE = 0;

   /* Set PORTA to digital IO. */
   PORTA = 0;
   LATA = 0;
   ANSELA = 0;

   /* Set PORTB to digital IO. */
   PORTB = 0;
   LATB = 0;
   ANSELB = 0;

   /* Set pins to output. */
   LED_TRIS = 0;
   TRISC = 0;
   PP_OUTPUT0_TRIS = 0;
   PP_OUTPUT1_TRIS = 0;

   /* Clear output pins. */
   PP_OUTPUT0_PIN = 0;
   PP_OUTPUT1_PIN = 0;

   /* Set pins to input. */
   PP_INPUT0_TRIS = 1;
   PP_INPUT1_TRIS = 1;
   PP_INPUT2_TRIS = 1;

   /* Enable weak pull-up on inputs. */
   OPTION_REGbits.nWPUEN = 0;
   PP_INPUT0_WPUB = 1;
   PP_INPUT1_WPUB = 1;
   PP_INPUT2_WPUB = 1;

   /* Select 48 mhz internal clock. */
   OSCCONbits.IRCF0 = 1;
   OSCCONbits.IRCF1 = 1;
   OSCCONbits.IRCF2 = 1;
   OSCCONbits.IRCF3 = 1;
    
   /* Enable clock tuning from USB. */
   ACTCONbits.ACTSRC = 1;
   ACTCONbits.ACTEN = 1;

   //rt_cal();

   /* Configure timer0 */
   OPTION_REGbits.TMR0CS = 0;   // FOSC/4
   OPTION_REGbits.TMR0SE = 0;   // increment on low to high edge
   OPTION_REGbits.PSA = 1;      // disable prescaler

   /* Clear and enable timer0 interrupt */ 
   INTCONbits.TMR0IF = 0;
   INTCONbits.TMR0IE = 1;

#if 0
   /* Configure timer1 */
   T1CONbits.TMR1CS = 3;  // select internal clocking, LFINTOSC (31khz)
   T1CONbits.T1OSCEN = 0;  // no dedicated timer1 oscilator
   T1GCONbits.TMR1GE = 1;  // gate function on 
   T1GCONbits.T1GPOL = 1;  // timer1 count when gate is high 
   T1GCONbits.T1GTM = 1;  // gate toggle mode on 
   T1GCONbits.T1GSPM = 0;  // gate sisgle-pulse mode off 
   T1GCONbits.T1GSS = 0;  // use timer1 gate pin input0 (RA4) 
   T1CONbits.T1CKPS = 0;   // prescaler = divide by 1
   T1CONbits.nT1SYNC = 0;   // synchronize asynchronous clock input
   T1CONbits.TMR1ON = 1;  // enable counter
#endif

   mInitializeUSBDriver();         // See usbdrv.h

   /* Enable interrupts */
   INTCONbits.GIE = 1;

   while (1)
   {
      /* Process standard EP0 transactions (enumeration, descriptors, EP0 Setup, EP0 In, EP0 Out). */
      USBCheckBusStatus();    // See usbdrv.c
      USBDriverService();
      rt_class_service();
   }
}
