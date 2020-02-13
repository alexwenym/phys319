
;.org 0xf0000
.org 0x00f0

start:
  mov #010, r15
  mov #10q, r15
  mov #10, r15
  mov #11, r15

  clr.w r7
  clr.w r8
  cmp r7, r8

  mov.w @r7, @r8

  mov #0x1234, r7
  mov #0x1234, r8
  cmp r7, r8

  mov.w #200, sp
  mov.w #0x1069, &300
  mov.w 300, &200
  jmp loop

  mov.w #200, r10
  mov.w #010, r11
  nop
  mov.w #300, r14
loop:
  mov.b #0, 0x0(r15)
  jmp loop

.org 0xfff4
  db 10
.org 0xfffe
  dw start


