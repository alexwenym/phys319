.include "msp430g2553.inc"
org 0xc000
START: 
	mov #0x400, SP
	mov.w #WDTPW|WDTHOLD, &WDTCTL
	mov.b #11111111b, &P1DIR ; set all port 1 pins to output
	;mov.b #00001000b, &P1REN
	; Pin0 not working well, strobe set to pin3 instead 
	
	mov.b #10010110b, &P1OUT ;set first to 9
	mov.b #10011111b, &P1OUT ; turn strobe on

	mov.b #01001101b, &P1OUT ; set second to 4
	mov.b #01000100b, &P1OUT ; turn strobe off
	mov.b #01001101b, &P1OUT ; turn strobe on

	mov.b #01110011b, &P1OUT ; set third to 7
	mov.b #01110010b, &P1OUT ; turn strobe off
	mov.b #01111011b, &P1OUT ; turn strobe on

	mov.b #01111001b, &P1OUT ; set fourth to 7
	mov.b #01110000b, &P1OUT ; turn strobe off
	mov.b #01111001b, &P1OUT ; turn strobe on
	
	bis.w #CPUOFF, SR

org 0xfffe
	dw START
