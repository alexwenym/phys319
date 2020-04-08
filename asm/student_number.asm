.include "msp430g2553.inc"
org 0xc000
START: 
	mov #0x400, SP
	mov.w #WDTPW|WDTHOLD, &WDTCTL
	mov.b #11111111b, &P1DIR ; set all port 1 pins to output
	;mov.b #00001000b, &P1REN
	; Pin5 broken; pin 5 output sent to pin 3 instead. Consequently pin3 -> D1 
	
	mov.b #10010110b, &P1OUT ;set first to 9
	;mov.b #10010111b, &P1OUT ; turn strobe on

	mov.b #01000100b, &P1OUT ; set second to 4
	;mov.b #01000100b, &P1OUT ; turn strobe off
	;mov.b #01000101b, &P1OUT ; turn strobe on

	mov.b #10000010b, &P1OUT ; set third to 8
	;mov.b #01111010b, &P1OUT ; turn strobe off
	;mov.b #01111011b, &P1OUT ; turn strobe on

	mov.b #01111000b, &P1OUT ; set fourth to 7
	;mov.b #01111000b, &P1OUT ; turn strobe off
	;mov.b #01111001b, &P1OUT ; turn strobe on
	
	bis.w #CPUOFF, SR

org 0xfffe
	dw START
