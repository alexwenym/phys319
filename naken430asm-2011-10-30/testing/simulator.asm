

test_carry:
  mov.w #-1, r8
  mov.w #-1, r7
  add.w r7, r8
  ret

test_no_overflow:
  mov.w #-1, r8
  mov.w #1, r7
  add.w r7, r8
  ret

test_overflow:
  mov.b #127, r8
  mov.b #127, r7
  add.b r7, r8
  ret

message:
  db "Just a marker for end of the file.\n"


