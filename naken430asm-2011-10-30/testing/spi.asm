
.include "msp430g2x31.inc"

  org 0xf800
start:
  ;; Turn off watchdog
  mov.w #0x5a80, &WDTCTL

  ;; Please don't interrupt me
  dint

  ;; Set up stack pointer
  mov.w #0x0280, SP

  ;; Set up crystal
  mov.b #(64|32), &DCOCTL
  mov.b #0xf, &BCSCTL1
  mov.b #0, &BCSCTL2

  ;; Set up output pins
  mov.b #0x05, &P1DIR        ; P1.2 P1.0
  mov.b #0x01, &P1OUT        ; P1.0=/CS

  ;; Set up SPI (input pin is turned off)
  ;mov.b #(USIPE7|USIPE6|USIPE5|USIMST|USIOE|USISWRST), &USICTL0
  mov.b #(USIPE6|USIPE5|USIMST|USIOE|USISWRST), &USICTL0
  mov.b #USICKPH, &USICTL1
  mov.b #(USIDIV_0|USISSEL_2), &USICKCTL ; div 1, SMCLK
  bic.b #USISWRST, &USICTL0      ; clear reset

send_spi:
  ;; Just a test of SPI
  bic.b #0x01, &P1OUT            ; /CS = 0 (enable)

  mov.w #0x3fff, &USISR
  mov.b #(USI16B|16), &USICNT    ; Flush out 16 bits

  ;mov.b #0x30, &USISRL
  ;mov.b #8, &USICNT             ; Flush out 8 bits

wait_spi:
  bit.b #USIIFG, &USICTL1
  jz wait_spi

  ;mov.b #0xff, &USISRL
  ;mov.b #8, &USICNT             ; Flush out 8 bits

wait_spi_low:
  ;bit.b #USIIFG, &USICTL1
  ;jz wait_spi_low

  bis.b #0x01, &P1OUT            ; /CS = 1 (disable)

main:
  jmp send_spi

  org 0xffe8
vectors:
  dw 0
  dw 0
  dw 0
  dw 0
  dw 0
  dw 0 
  dw 0
  dw 0
  dw 0
  dw 0
  dw 0
  dw start                 ; Reset


