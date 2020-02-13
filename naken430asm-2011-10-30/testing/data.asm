

start:
  mov #1, r6
  mov #1, r6

hello:
db "hello"
blah:
.db "hello"
hello_ascii:
.ascii "hello"
.asciiz "hello"
.db 50

dw hello*3, start, start+6, 0x50*5
dw -1, -5, -1 * 5, 65535
db -1, 0x69, 0x20, 0x55, 5*3, 3<<1

