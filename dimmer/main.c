//******************************************************************************
//  MSP430G2x31 Demo - ADC10, Sample A1, AVcc Ref, Set P1.0 if > 0.75*AVcc
//
//  Description: A single sample is made on A1 with reference to AVcc.
//  Software sets ADC10SC to start sample and conversion - ADC10SC
//  automatically cleared at EOC. ADC10 internal oscillator times sample (16x)
//  and conversion. 
//
//                MSP430G2x31
//             -----------------
//         /|\|              XIN|-
//          | |                 |
//          --|RST          XOUT|
//            |                 |           
//            |                 |
//            |                 |
//            |                 |
//            |                 |
// input  >---|P1.1/A1      P1.0|--> 
//            |                 |
//            |             P1.2|--> output 
//            |             P1.6|-->
//
//
//  D. Dang
//  Texas Instruments Inc.
//******************************************************************************
#include "msp430.h"

void main(void)
{
  WDTCTL = WDTPW + WDTHOLD;                 // Stop WDT
  ADC10CTL0 = ADC10SHT_2 + ADC10ON; 	    // ADC10ON
  ADC10CTL1 = INCH_1;                       // input A1
  ADC10AE0 |= 0x02;                         // PA.1 ADC option select
  P1DIR = 0x04 ;                            // Set P1.0 to output direction
  P1SEL = 0x04;
  CCR0 = 1000-1;
  CCTL1 = OUTMOD_7; 
  float r; 
  int ccr1; 

  for (;;)
  {
    ADC10CTL0 |= ENC + ADC10SC;             // Sampling and conversion start
    while (ADC10CTL1 &ADC10BUSY);          // ADC10BUSY?
    
    r = ADC10MEM/1000.; 
    ccr1 = (int) 999*r; 
    CCR1 = ccr1 ;
    TACTL = TASSEL_2 + MC_1;
    
    /*if (ADC10MEM > 0x26C)
      P1OUT = 0x01;                       // Clear P1.0 LED off
    else if (ADC10MEM < 0xF8)		 // levels: 0.8 V to 2 V, or 248 - 620 bit	
      P1OUT = 0x40;				// aka F8 - 26C in hex
    else if (0xF8 < ADC10MEM < 0x26C)
      P1OUT = 0x20;                        // Set P1.0 LED on
    */
    unsigned i;
    for (i = 0xFFFF; i > 0; i--);           // Delay
  }
}
//******************************************************************************


