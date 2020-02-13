

.macro blah(a)
  mov.w #a, r8
  mov.w #a, &0x1000    ; store this at address 0x1000
  mov.w #a, &a
  mov.w #a, r9
.endm

mov.w #1, r11
blah(6)
mov.w #2, r10

