
#define BLAH \
  mov #0, r12 \
  mov #1, r12 \
  mov #2, r12

start:
  mov #3, r13
  BLAH
  mov #3, r13
  roar

