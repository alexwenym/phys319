
.org 0xff00

start:
  mov.b &var1, r10
  mov.b &var2, r10
  mov.w &var2, r10

var1:
  db 50
var2:
  db 60
var3:
  db 60
var4:
  dw 1000


  mov.b var3, r11

ds 10
db "test"
db "zomg"


