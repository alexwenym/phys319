
#define P1OUT 0x0021
#define P1DIR 0x0022
#define WDTPW (0x5A<<8)
#define WDTHOLD (1<<7)
;#define WDTCTL __MSP430_WDT_A_BASE__ + 0x0C
#define WDTCTL 0x0120 

  .org 0xf800
start:
  mov.w #0x5a80, &WDTCTL
  mov.w #0x41, &P1DIR
  mov.w #0x01, r8
repeat:
  mov.w r8, &P1OUT
  xor.w #0x41, r8
  mov.w #60000, r9
waiter:
  dec r9
  jnz waiter
  jmp repeat

  .org 0xfffe
  .dc start             ; set reset vector to 'init' label


