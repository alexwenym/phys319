#include "msp430g2553.inc"              ; include header statement
	org 0x0C000			; where to put the program 
RESET:					; begin the reset flag
	mov.w #0x400, sp		; initialize stack pointer
        mov.w #WDTPW|WDTHOLD,&WDTCTL	; disable the watchdog
	mov.b #11110111b, &P1DIR	; set all the pins except P1.3 to output	
	mov.b #01001001b, &P1OUT	; set pins 0 (led1), 3, and 6 (led2) to 1
	mov.b #00001000b, &P1REN	; set resistor on pin 3 (button)
	mov.b #00001000b, &P1IE		; interrupt enable on pin 3 (button) 
	mov.w #0000000001001001b, R7	; value of register R7 to 0x0049 (01001001)
	mov.w #0000000001001000b, R8
	mov.w #0000000000001001b, R9
	mov.w #0000000000001000b, R10
	;mov.w #0000000000001000b, R11
	mov.b R7, &P1OUT		; set value of P1OUT to value of register R7

	EINT
	bis.w #CPUOFF,SR		; cpu off 
PUSH:
	mov.b R7,&P1OUT			; set P1OUT to contents of R7 register

	mov.w R7, R11	
	mov.w R8, R7
	mov.w R9, R8
	mov.w R10, R9
	mov.w R11, R10
	bic.b #00001000b, &P1IFG	; interrupt flag, pin3 (button)
	reti				; return from interrupt

	org 0xffe4			
	dw PUSH
	org 0xfffe
	dw RESET

