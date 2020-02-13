

.define CPUOFF (0x0010)
.define CRAP (0x0040)
.define SCG1 (0x0080)
.define LPM3 (SCG1+CRAP+CPUOFF)

.org 0xf000

start:

  bis.w #LPM3, SR


