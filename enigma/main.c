
#include "msp430.h"


void delay_ms(unsigned int ms){
	while(ms){
		__delay_cycles(1000);
		ms--;
	}
}


int reflector(int input){
	int ref_array[10] = {9,8,7,6,5,4,3,2,1,0}; 
	return ref_array[input];
}

void trig(){
	P1OUT-- ; //set strobe to L 
	P1OUT++ ; //set strobe back to high  
}

void main(void)
{
  WDTCTL = WDTPW + WDTHOLD;                 // Stop WDT
  P1DIR = 0xFF ;
  P2DIR = 0x00 ; 

  P1OUT = 0x11; trig();  
                             // Set P1.0 to output direction
  P1OUT = 0x05 ; trig();
  P1OUT = 0x07 ; trig();
  delay_ms(200);
  P1OUT = 0x15 ; trig();
  P1OUT = 0x17 ; trig(); 
  delay_ms(200) ;
  P1OUT = 0x2D;  trig();// << 
  P1OUT = 0x2F ; trig();
  delay_ms(200);
  P1OUT = 0x3D ; trig();// << 
  P1OUT = 0x3F ; trig();
  delay_ms(200) ;
  P1OUT = 0x45;  trig();
  P1OUT = 0x47;  trig(); 
  delay_ms(200); 
  P1OUT = 0x55 ; trig(); 
  P1OUT = 0x57;  trig(); 
  delay_ms(200) ;
  P1OUT = 0x6D;  trig();
  P1OUT = 0x6F;  trig(); 
  delay_ms(200);
  P1OUT = 0x7D ; trig();
  P1OUT = 0x7F;  trig(); 
  delay_ms(200) ;
  P1OUT = 0x85;  trig();
  P1OUT = 0x87 ; trig(); 
  delay_ms(200);
  P1OUT = 0x95 ; trig();
  P1OUT = 0x97 ; trig(); 
  delay_ms(200);
   
  int r1[10] = {1,5,8,6,4,0,3,2,9,7};
  int r2[10] = {2,0,9,3,6,5,1,7,4,8};
  int r3[10] = {4,8,5,6,1,7,0,3,2,9}; 
  int input; 
  int index; 

  int rotor_counter = 1; 
 
  for (;;)
  {      

     if(P2IN == 0x00){P1OUT = 0x07; trig(); input=0;}
     else if(P2IN == 0x01){P1OUT = 0x17; trig(); input=1;} 
     else if(P2IN == 0x02){P1OUT = 0x2F; trig(); input=2;}
     else if(P2IN == 0x03){P1OUT = 0x3F; trig(); input=3;} 
     else if(P2IN == 0x04){P1OUT = 0x47; trig(); input=4;} 
     else if(P2IN == 0x05){P1OUT = 0x57; trig(); input=5;} 
     else if(P2IN == 0x06){P1OUT = 0x6F; trig(); input=6;} 
     else if(P2IN == 0x07){P1OUT = 0x7F; trig(); input=7;} 
     else if(P2IN == 0x08){P1OUT = 0x87; trig(); input=8;} 
     else if(P2IN == 0x09){P1OUT = 0x97; trig(); input=9;}

 /*
     if(P2IN == 0x00){input=0;}
     else if(P2IN == 0x01){input=1;} 
     else if(P2IN == 0x02){input=2;}
     else if(P2IN == 0x03){input=3;} 
     else if(P2IN == 0x04){input=4;} 
     else if(P2IN == 0x05){input=5;} 
     else if(P2IN == 0x06){input=6;} 
     else if(P2IN == 0x07){input=7;} 
     else if(P2IN == 0x08){input=8;} 
     else if(P2IN == 0x09){input=9;}
  */   
     if(P2IN == 0x0C){
	for (int i=0; i<10; i++){
            r1[i]++; 
	    if(r1[i]==10) r1[i]=0;
	}
        rotor_counter++; 
	
	if (rotor_counter == 2){P1OUT = 0x29; trig();}
	else if (rotor_counter == 3){P1OUT = 0x39; trig();}
	else if (rotor_counter == 4){P1OUT = 0x41; trig();}
	else if (rotor_counter == 5){P1OUT = 0x51; trig();}
	else if (rotor_counter == 6){P1OUT = 0x69; trig();}
	else if (rotor_counter == 7){P1OUT = 0x79; trig();}
	else if (rotor_counter == 8){P1OUT = 0x81; trig();}
	else if (rotor_counter == 9){P1OUT = 0x91; trig();}


	delay_ms(2000);
     }


     //forward pass on r1
     input = r1[input]; 
     // forward pass on r2
     input = r2[input]; 
     //forward pass on r3
     input = r3[input]; 
     //reflector
     input = reflector(input);
     // reverse pass on r3
     for (int i = 0; i<10; i++){
     	if (input == r3[i]) index = i; 
     }
     input = index; 
     // reverse pass on r2
     for (int i = 0; i<10; i++){
     	if (input == r2[i]) index = i; 
     }
     input = index; 
     //reverse pass on r1 (need to find index)
     for (int i = 0; i<10; i++){
     	if (input == r1[i]) index = i; 
     }
     input = index; 
     
     if(input == 0){P1OUT = 0x05; trig();}
     else if(input == 1){P1OUT = 0x15; trig();} 
     else if(input == 2){P1OUT = 0x2D; trig();}
     else if(input == 3){P1OUT = 0x3D; trig();} 
     else if(input == 4){P1OUT = 0x45; trig();} 
     else if(input == 5){P1OUT = 0x55; trig();} 
     else if(input == 6){P1OUT = 0x6D; trig();} 
     else if(input == 7){P1OUT = 0x7D; trig();} 
     else if(input == 8){P1OUT = 0x85; trig();} 
     else if(input == 9){P1OUT = 0x95; trig();}
	
    
    
    /*if (ADC10MEM > 0x26C)
      P1OUT = 0x01;                       // Clear P1.0 LED off
    else if (ADC10MEM < 0xF8)		 // levels: 0.8 V to 2 V, or 248 - 620 bit	
      P1OUT = 0x40;				// aka F8 - 26C in hex
    else if (0xF8 < ADC10MEM < 0x26C)
      P1OUT = 0x20;                        // Set P1.0 LED on
    */
    //unsigned i;
    //for (i = 0x0001; i > 0; i--);           // Delay
    delay_ms(50);
  }
}
//******************************************************************************


