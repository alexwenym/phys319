#include "msp430_io.inc"

#define _E \
  bis.b #2, &P4OUT \
  nop \
  nop \
  bic.b #2, &P4OUT

#define DISP_ON  0x0c
#define DISP_OFF 0x08
#define CLR_DISP 0x01
#define CUR_HOME 0x02
#define ENTRY_INC 0x06
#define DD_RAM_ADDR 0x80
#define DD_RAM_ADDR2 0xc0
#define DD_RAM_ADDR3 0x28
#define CG_RAM_ADDR 0x40

RS equ 8

org 0x1100
start:
  mov.w #0x5a80, &WDTCTL
  mov.w #0x09fe, SP
  mov.b #0xff, &P4DIR
  mov.b #(128|64), &P3DIR

  mov.b #64, &P3OUT
  mov.b #1, &P4OUT

  ;; SET CRYSTAL TO 8 MHz
  bic.w #(OSCOFF), SR
  bic.b #(XT2OFF), &BCSCTL1
test_osc:
  bic.b #(OFIFG), &IFG1
  mov.w #(0ffh), r15
dec_again:
  dec r15
  jnz dec_again
  bit.b #(OFIFG), &IFG1
  jnz test_osc
  mov.b #(SELM_2|SELS), &BCSCTL2
  ;; END SETTING CRYSTAL

  call #init_lcd
  mov.b #48, r13
  call #send_lcd_char

repeat:
  mov.b #64, &P3OUT
  call #delay
  mov.b #128, &P3OUT
  call #delay
  jmp repeat

delay400ms:
  mov.w #12, r14
delay400ms_again:
  mov.w #0xffff, r15
delay400ms_repeat:
  dec r15
  jnz delay400ms_repeat
  dec r14
  jnz delay400ms_again
  ret

delay100ms:
  mov.w #3, r14
delay100ms_again:
  mov.w #0xffff, r15
delay100ms_repeat:
  dec r15
  jnz delay100ms_repeat
  dec r14
  jnz delay100ms_again
  ret

delay:
  mov.w #0xffff, r8
delay_repeat:
  dec r8
  jnz delay_repeat
  ret

init_lcd:
  bic.b #RS, &P4OUT
  call delay400ms
  bis.b #0x30, &P4OUT
  bic.b #0xc0, &P4OUT
  _E
  call delay100ms
  _E
  call delay100ms
  _E
  call delay100ms
  bic.b #0x10, &P4OUT
  _E

  mov.b #DISP_ON, r13
  call send_lcd_cmd
  mov.b #CLR_DISP, r13
  call send_lcd_cmd
  mov.b #DD_RAM_ADDR, r13
  call send_lcd_cmd
  ret

send_lcd_cmd:
  call delay100ms
  mov.b r13, r12
  rlc.b r13
  rlc.b r13
  rlc.b r13
  rlc.b r13
  and.b #0xf0, r13
  and.b #0xf0, r12
  and.b #0x0f, &P4OUT
  bis.b r13, &P4OUT
  bis.b #RS, &P4OUT
  _E
  bis.b r12, &P4OUT
  bis.b #RS, &P4OUT
  _E
  ret

send_lcd_char:
  call delay100ms
  mov.b r13, r12
  rlc.b r13
  rlc.b r13
  rlc.b r13
  rlc.b r13
  and.b #0xf0, r13
  and.b #0xf0, r12
  and.b #0x0f, &P4OUT
  bis.b r13, &P4OUT
  bic.b #RS, &P4OUT
  _E
  bis.b r12, &P4OUT
  bic.b #RS, &P4OUT
  _E
  ret

org 0xfffe
  dw 0x1100

