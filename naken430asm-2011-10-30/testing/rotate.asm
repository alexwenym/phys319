
.org 0xf000
start:
  nop
  rlc.b r7
  rlc.b r7
  rlc.b r7
  rlc.b r7
  rlc.b r7

  rrc.b r6
  rrc.b r6
  rrc.b r6
  rrc.b r6
  rrc.b r6

  rra.b r5
  rra.b r5
  rra.b r5
  rra.b r5
  rra.b r5
  ret


.org 0xfffe
  dw start

