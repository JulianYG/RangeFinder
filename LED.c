#include <msp430.h>
#include <stdio.h>
#include <string.h>
#include "stdint.h"
#include "math.h"
#include <stdlib.h>
#define NEO BIT7
#define TXD BIT2
#define RXD BIT1
int atoi(const char * str);
void set(int pixel,int g,int r,int b);
void write_ws2811(uint8_t *data, unsigned length, uint8_t pinmask);
void uart_config(void);
void color_config(int dist);
static uint8_t z[5 * 3];
int dist;

void main(void)
{
 	WDTCTL = WDTPW + WDTHOLD; 					// No watchdog reset
	BCSCTL1 = CALBC1_16MHZ;						// Run at 16 MHz
	DCOCTL  = CALDCO_16MHZ;

	P1OUT = 0;
	P1DIR |= NEO;

	uart_config();

	int dist = 1999;


	color_config(dist);
}

void color_config(int d){

	if (d < 500){

		set(0,255,0,0);
	}

	if (500 <= d <= 3000){

		int first = d / 1000;

		int second = (d-1000 * first) / 100;

		int third = (d-1000 * first - 100 * second) / 10;

		int fourth = d - 1000*first - 100*second - 10*third;

		set(4,0,first * 85,0);

		set(3,0,0,second * 28);

		set(2,third * 20,0,0);

		set(1,fourth*20,fourth*20,fourth*20);

//		set(5,7,9,0);
	}

	if (d > 3000){

		set(0,0,128,0);

	}
}

void uart_config(void){

	P1SEL |= TXD + RXD;
    P1SEL2 |= TXD + RXD;	// select special function UART

    UCA0CTL1 |= UCSSEL_2;	// select SMCLK
    UCA0BR0 = 131;
    UCA0BR1 = 6;
    UCA0MCTL = UCBRS0;	// set baud rate 9600

    UCA0CTL1 &= ~UCSWRST;	// initialize

    IFG2 &= UCA0TXIFG;
    IE2 =  UCA0RXIE;	// enable interrupt

    __enable_interrupt();
}

#pragma vector = USCIAB0RX_VECTOR
__interrupt void RX_ISR(void){

	while (!(IFG2 & UCA0TXIFG));{

		dist = UCA0RXBUF;

		UCA0TXBUF = dist;

		__delay_cycles(30);
	}
}

void set(int pixel,int r,int g,int b){

	int p;

	if (pixel > 0 && pixel < 6){

		for (p = (pixel - 1) * 3; p < pixel * 3;){

			z[p++] = g;
			z[p++] = r;
			z[p++] = b;
		}
	}

	else if (pixel == 0){

		for (p = 0; p < 5 * 3;){

			z[p++] = g;
			z[p++] = r;
			z[p++] = b;
		}
	}

	write_ws2811(z, sizeof(z), NEO);
}
