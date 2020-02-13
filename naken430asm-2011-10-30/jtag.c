/**
 *  Naken430asm MSP430 assembler.
 *  Author: Michael Kohn
 *   Email: mike@mikekohn.net
 *     Web: http://www.mikekohn.net/
 * License: GPL
 *
 * Copyright 2010-2011 by Michael Kohn
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>

#include "jtag.h"
#include "naken430util.h"

// UGH!
static struct termios oldtio;

int open_jtag(struct _util_context *util_context, char *device)
{
struct termios newtio;

  util_context->fd=open(device,O_RDWR|O_NOCTTY);
  if (util_context->fd==-1)
  {
    printf("Couldn't open serial device.\n");
    exit(1);
  }

  tcgetattr(util_context->fd,&oldtio);

  memset(&newtio,0,sizeof(struct termios));
  newtio.c_cflag=B9600|CS8|CLOCAL|CREAD;
  newtio.c_iflag=IGNPAR;
  newtio.c_oflag=0;
  newtio.c_lflag=0;
  newtio.c_cc[VTIME]=0;
  newtio.c_cc[VMIN]=1;

  tcflush(util_context->fd, TCIFLUSH);
  tcsetattr(util_context->fd,TCSANOW,&newtio);

  return 0;
}

void close_jtag(struct _util_context *util_context)
{
  tcsetattr(util_context->fd,TCSANOW,&oldtio);
  close(util_context->fd);
}

static int send_sync(struct _util_context *util_context)
{
unsigned char s=0x80;
int n;

  n=write(util_context->fd,&s,1);
  while(1)
  {
    n=read(util_context->fd,&s,1);
    if (n<0) return -1;
    if (n==1) break;
  }

  if (s==0x90) return 0;

  return -1;
}

static int checksum(unsigned char *buff, int len)
{
unsigned char ckl=0,ckh=0;
int n;

  for (n=0; n<len; n=n+2)
  {
    ckl^=buff[n];
    ckh^=buff[n+1];
  }

  ckl=~ckl;
  ckh=~ckh;

  return (ckh<<8)|ckl;
}

static int send_data(struct _util_context *util_context, unsigned char *buffer, int len)
{
int ck;
int n=0,r;

#if 0
  ck=checksum(buffer, len);
  buffer[len]=ck&255;
  buffer[len+1]=ck>>8;
#endif

  //len=len+2;

  while(n<len)
  {
printf("n=%d\n", n);
    r=write(util_context->fd,buffer+n,len-n);
    if (r<=0) return -1;
    n=n+r;
  }
printf("n=%d\n", n);

  return 0;
}

static int read_data(struct _util_context *util_context, unsigned char *buffer, int len)
{
int n=0,r;

printf("%d\n", util_context->fd);
fflush(stdout);

  while(n<len)
  {
    r=read(util_context->fd,buffer+n,len-n);
printf("r=%d n=%d\n", r, n);
fflush(stdout);
    if (r<0) return -1;
    n=n+r;
  }

  return 0;
}

int read_memory(struct _util_context *util_context, int address, int len)
{
//unsigned char buff[10+2+256] = { 0x80, 0x14, 0x04, 0x04, 0, 0, 0, 0x00 };
//unsigned char buff[10+2+256] = { 0x80, 0x14, 0x04, 0x04, 0xf0, 0x0f, 0x0e, 0x00 };
//unsigned char buff[10+2+256] = { 0x28 };
unsigned char buff[16] = { 0x0d, 0x02, 0x02, 0x00,
                           0x00, 0x00, 0x00, 0x00,
                           0x08, 0x00, 0x00, 0x00 };
int l;

#if 0
  buff[0]=0x02;
  buff[1]=0x00;
  buff[2]=0x00;
  if (send_data(util_context, buff, 3)!=0) return -2;
  if (read_data(util_context, buff, 2)!=0) return -3;
  printf("%02x %02x %02x %02x\n", buff[0], buff[1], buff[2], buff[3]);
#endif

/*
  buff[4]=address&255;
  buff[5]=address>>8;
  buff[6]=len&255;
*/

//printf("Send sync\n");
//fflush(stdout);
  //if (send_sync(util_context)!=0) return -1;
//printf("Send request\n");
//fflush(stdout);
  //if (send_data(util_context, buff, 1)!=0) return -2;
  if (send_data(util_context, buff, 12)!=0) return -2;
//printf("Read data\n");
//fflush(stdout);
  if (read_data(util_context, buff, 8)!=0) return -3;

  //printf("%02x %02x %02x %02x\n", buff[0], buff[1], buff[2], buff[3]);
  //l=buff[2];
  //if (read_data(util_context, buff, l+2)!=0) return -4;

int n;
for (n=0; n<16; n++) printf("%d\n", buff[n]);

  memcpy(util_context->bin, buff+8, 8);

  printf("Read %d bytes from device address 0x%04x.\n", l, address);

  return 0;
}


