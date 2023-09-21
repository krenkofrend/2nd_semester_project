/*
 
 * Author : Max Gallardo
  
 */ 

#define F_CPU 16000000UL //needs to be defined for the delay functions to work.
#define BAUD 9600
#include <avr/io.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>
#include "usart.h"

void change_page(void);
void delay_0_28ms(unsigned int);
void punch(void);
void retrive_punch(void);
void run_motor_forwards(int);
void run_motor_backwards(int);
void stop_motor(void);
void stop_punch(void);
void run_motor_to_A4(void);

char readBuffer[100];
char buffer[100];

int paper_size = 4; // 4 = A4, 3 = A3, 2 = A2

//small motor 2048 steps/rev
	/* Clockwise order */
	
	#define S_BLUE		0b10000000 //pin blue cable is connected 
	#define S_GREEN		0b01000000 // green cable
	#define S_YELLOW	0b00100000 // yellow
	#define S_ORANGE	0b00010000 // orange cable
	#define S_DELAY 8  //S_DELAY * 0.28ms = delay between each step
	
	#define DISTANCE 1613 //2048*7.425/(2*pi*1.5) number of steps the motor has to do to change from one paper size to the next/previous
	#define DOUBLE_DISTANCE 3227 //double the distance. To move from A4 to A2
	
	#define ZERO   0b00001001 //pins energized at step 0
	#define ONE    0b00000101 //step 1
	#define TWO    0b00000110 //step 2
	#define THREE  0b00001010 //step 3
	
	#define B_DELAY 5 //cycles the timer will run
	
	#define PP 50 //number to accelerate for punch
	#define RR 50 //number to accelerate for retrieve punch
	int p = PP; //variable to accelerate the motor
	int r = RR; //variable to accelerate for retrieve punch

	
int main(void)
{
	
	DDRB = 0xFF;    /* Enable output on all of the B pins */
	PORTB = 0x00;            /* Set them all to 0v */
	DDRD = 0xFF;
	PORTD = 0x00;
	
	uart_init();//initialize communication with PC - debugging
	io_redirect();//redirect printf function to uart, so text will be shown on PC
	
	printf("page 0%c%c%c",255,255,255);//use number of page not name
	delay_0_28ms(1500);
	run_motor_to_A4();
	
    while (1) 
    {
		scanf("%c", &buffer[0]);
		if(buffer[0] == 0x65){
			for(int i = 1; i<7; i++)
			{
				scanf("%c", &buffer[i]);
			}
			if(buffer[1] == 0x00){
				if (buffer[2] == 0x01){ //if A4 button pressed on nextion
					run_motor_to_A4(); //call run_motor_to_A4 function so motor stops when reaching end-stop
					change_page();
					paper_size = 4; //save A4 as the new paper size
				}
				
				if (buffer[2] == 0x02){ //if A3 button pressed on nextion
					if (paper_size == 4){ //A4 previous paper size 
						run_motor_backwards(DISTANCE); //run motor backwards to reach A3 
					}
					if (paper_size == 2){ //A2 previous paper size
						run_motor_forwards(DISTANCE); //run motor forwards to reach A3
					}
					change_page();
					paper_size = 3; //save A3 as the new paper size
				}
				
				if (buffer[2] == 0x03){ //if A2  button pressed on nextion
					if (paper_size == 4){ //A4 previous paper size
						run_motor_backwards(DOUBLE_DISTANCE); //run motor backwards twice the distance to reach A2
					}
					if (paper_size == 3){ //A3 previous paper size
						run_motor_backwards(DISTANCE); //run motor backwards to reach A3 
					}
					change_page();
					paper_size = 2; //save A2 as the new paper size
				}
				
			}
			if(buffer[1] == 0x02){
				printf("page 1%c%c%c",255,255,255);
				punch(); //represents big motor
				printf("page 3%c%c%c",255,255,255);
			}
			if(buffer[1] == 0x03){
				printf("page 1%c%c%c",255,255,255);
				punch(); //represents big motor
				printf("page 3%c%c%c",255,255,255);
			}
		}
	}
}

void change_page(){
	printf("page 2%c%c%c",255,255,255);
}
void run_motor_forwards(int steps){
	for (int i = 0; i < steps ; i = i + 4){
	   PORTD = S_BLUE;
	   delay_0_28ms(S_DELAY);
	   PORTD = S_GREEN;
	   delay_0_28ms(S_DELAY);
	   PORTD = S_YELLOW;
	   delay_0_28ms(S_DELAY);
	   PORTD = S_ORANGE;
	   delay_0_28ms(S_DELAY);
	}
	stop_motor();
}

void run_motor_backwards(int steps){
	for (int i = 0; i < steps; i = i + 4){
		PORTD = S_ORANGE;
		delay_0_28ms(S_DELAY);
		PORTD = S_YELLOW;
		delay_0_28ms(S_DELAY);
		PORTD = S_GREEN;
		delay_0_28ms(S_DELAY);
		PORTD = S_BLUE;
		delay_0_28ms(S_DELAY);
	}
	stop_motor();
}

void run_motor_to_A4 (){
	PINB = 0b00010000; // enable pull up
	while (!(PINB == 0b00010000)){ //pin D12
	   PORTD = S_BLUE; //pin blue cable from motor is connected 
	   delay_0_28ms(S_DELAY);//delay between each step
	   PORTD = S_GREEN; //green cable 
	   delay_0_28ms(S_DELAY);
	   PORTD = S_YELLOW; //yellow cable  
	   delay_0_28ms(S_DELAY);
	   PORTD = S_ORANGE; //pin orange 
	   delay_0_28ms(S_DELAY);
	}
	PORTB = 0x00; // DISABLE pull up
	stop_motor(); //call the loop that stops the motor
}

void stop_motor(){
		PORTD = 0x00; 
}

void punch(){
	PIND = 0b00000100; // enable pull up
	while (!(PIND&4)){ // pin D2
		PORTB = (ZERO); //3 0
		delay_0_28ms(B_DELAY + p);
		PORTB = (ONE); //2 0
		delay_0_28ms(B_DELAY + p);
		PORTB = (TWO);//2 1
		delay_0_28ms(B_DELAY + p);
		PORTB = (THREE);//3 1
		delay_0_28ms(B_DELAY + p);
		if (p>0) p--; /*decrease p value until 0 so the 
		motor accelerates until the needed value to rotate 
		at the desired RPM*/
	}
	stop_punch();//stop motor before reversing it
	p = PP;// reset p value
	PORTD = 0x00; //disable pullups
	delay_0_28ms(1000); //delay before reversing the motor
	retrive_punch();
}

void retrive_punch(){
	PIND = 0b00001000; // enable pull up
	while (!(PIND&8)){ // pin D3
		PORTB = (THREE);// 3 1
		delay_0_28ms(B_DELAY + r);
		PORTB = (TWO);// 3 2
		delay_0_28ms(B_DELAY + r);
		PORTB = (ONE);// 2 0
		delay_0_28ms(B_DELAY + r);
		PORTB = (ZERO);// 3 0
		delay_0_28ms(B_DELAY + r);
		if (r>0) r--; /*decrease r value 
		until 0 so the motor accelerates 
		until the needed value to rotate 
		at the desired RPM*/
	}
	r  = RR; //reset r value
	PORTD = 0x00; //disable pull ups
	stop_punch();
}

void stop_punch(){
	PORTB = 0x00;
}

void delay_0_28ms(unsigned int loops){
	int counter = 0;//set counter to 0
	do {
		// Set the Timer Mode to CTC
		TCCR0A |= (1 << WGM01);
		// Set the value that you want to count to
		OCR0A = 0x45; 
		// start the timer
		TCCR0B |= (1 << CS01) | (1 << CS00); // set prescaler to 64 and start the timer
		while ( (TIFR0 & (1 << OCF0A) ) == 0)  // wait for the overflow event
		{
			
		}
		// reset the overflow flag
		TIFR0 = (1 << OCF0A);
		counter ++;
	} while (counter < loops); //do while counter hasn't reached the loops
}

















