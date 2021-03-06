
//
//

#include  "msp430g2553.h"
#include "stdint.h"
#include <math.h>
#include <signal.h>
#define isdigit(n) (n >= '0' && n <= '9')
typedef unsigned char byte;
#define NOTE_B0  31
#define NOTE_C1  33
#define NOTE_CS1 35
#define NOTE_D1  37
#define NOTE_DS1 39
#define NOTE_E1  41
#define NOTE_F1  44
#define NOTE_FS1 46
#define NOTE_G1  49
#define NOTE_GS1 52
#define NOTE_A1  55
#define NOTE_AS1 58
#define NOTE_B1  62
#define NOTE_C2  65
#define NOTE_CS2 69
#define NOTE_D2  73
#define NOTE_DS2 78
#define NOTE_E2  82
#define NOTE_F2  87
#define NOTE_FS2 93
#define NOTE_G2  98
#define NOTE_GS2 104
#define NOTE_A2  110
#define NOTE_AS2 117
#define NOTE_B2  123
#define NOTE_C3  131
#define NOTE_CS3 139
#define NOTE_D3  147
#define NOTE_DS3 156
#define NOTE_E3  165
#define NOTE_F3  175
#define NOTE_FS3 185
#define NOTE_G3  196
#define NOTE_GS3 208
#define NOTE_A3  220
#define NOTE_AS3 233
#define NOTE_B3  247
#define NOTE_C4  262
#define NOTE_CS4 277
#define NOTE_D4  294
#define NOTE_DS4 311
#define NOTE_E4  330
#define NOTE_F4  349
#define NOTE_FS4 370
#define NOTE_G4  392
#define NOTE_GS4 415
#define NOTE_A4  440
#define NOTE_AS4 466
#define NOTE_B4  494
#define NOTE_C5  523
#define NOTE_CS5 554
#define NOTE_D5  587
#define NOTE_DS5 622
#define NOTE_E5  659
#define NOTE_F5  698
#define NOTE_FS5 740
#define NOTE_G5  784
#define NOTE_GS5 831
#define NOTE_A5  880
#define NOTE_AS5 932
#define NOTE_B5  988
#define NOTE_C6  1047
#define NOTE_CS6 1109
#define NOTE_D6  1175
#define NOTE_DS6 1245
#define NOTE_E6  1319
#define NOTE_F6  1397
#define NOTE_FS6 1480
#define NOTE_G6  1568
#define NOTE_GS6 1661
#define NOTE_A6  1760
#define NOTE_AS6 1865
#define NOTE_B6  1976
#define NOTE_C7  2093
#define NOTE_CS7 2217
#define NOTE_D7  2349
#define NOTE_DS7 2489
#define NOTE_E7  2637
#define NOTE_F7  2794
#define NOTE_FS7 2960
#define NOTE_G7  3136
#define NOTE_GS7 3322
#define NOTE_A7  3520
#define NOTE_AS7 3729
#define NOTE_B7  3951
#define NOTE_C8  4186
#define NOTE_CS8 4435
#define NOTE_D8  4699
#define NOTE_DS8 4978
#define OCTAVE_OFFSET 0
//I've changed here int to const int in order to use the flash
const int notes[] = { 0, NOTE_C4, NOTE_CS4, NOTE_D4, NOTE_DS4, NOTE_E4, NOTE_F4,
		NOTE_FS4, NOTE_G4, NOTE_GS4, NOTE_A4, NOTE_AS4, NOTE_B4, NOTE_C5,
		NOTE_CS5, NOTE_D5, NOTE_DS5, NOTE_E5, NOTE_F5, NOTE_FS5, NOTE_G5,
		NOTE_GS5, NOTE_A5, NOTE_AS5, NOTE_B5, NOTE_C6, NOTE_CS6, NOTE_D6,
		NOTE_DS6, NOTE_E6, NOTE_F6, NOTE_FS6, NOTE_G6, NOTE_GS6, NOTE_A6,
		NOTE_AS6, NOTE_B6, NOTE_C7, NOTE_CS7, NOTE_D7, NOTE_DS7, NOTE_E7,
		NOTE_F7, NOTE_FS7, NOTE_G7, NOTE_GS7, NOTE_A7, NOTE_AS7, NOTE_B7 };
char *song = "LitterStar:d=16,o=5,b=180:16c,16e,16g";
char *warning = "d=8,o=5,b=95:32c,32e,32c,32e,32c,32e,32c,32e,32c,32e,32c,32e";

volatile unsigned int time = 0;

#define AUD BIT6
void delay(unsigned int ms);
void speaker_config(void);
void play(unsigned int hz);
void stop(void);

void timerSetup(void);
void CalibrateADC(void);
void CalibrateUART(void);

void sendDistance(char currentDist[32]);
void UARTSendArray(char *TxArray);
void setNeoPixel(int pixel[5], int G, int R, int B);
void write_ws2811(uint8_t *data, unsigned length, uint8_t pinmask);

static uint8_t z[5 * 3];

char command[64];
char *ip = &command[0];

int ThresDist1;							// set the lower threshold range; recommend range 155 - 165
int ThresDist2;							// set the upper threshold range; recommend range 174 - 179
char distChar[16];

int PIXEL[5];
int *ipPIXEL = &PIXEL[0];
int GRB[3];
int *ipGRB = &GRB[0];

void main(void)
{
	WDTCTL = WDTPW + WDTHOLD;	// disable watchdog timer

	timerSetup();

	CalibrateADC();				// setup ADC10
	CalibrateUART();

	__bis_SR_register(LPM1_bits); 		// Enter LPM3, interrupts enabled
}

void timerSetup(void)
{
	BCSCTL1 = CALBC1_16MHZ; 					// Set DCO to 16MHz
	DCOCTL = CALDCO_16MHZ; 						// Set DCO to 16MHz

	TA1CTL = TASSEL_2 + MC_1;	// TASSEL_2 is source from SM_CLK, with frequency 1MHz.
	TA1CCTL0 = CCIE;			// enable capture and compare control on TA1CCTL0
	TA1CCR0 = 1023;				// set TA1_A3 CCR0 as 1023, match with the ADC10MEM, which is from 0-1023

	__enable_interrupt();		// global interrupt enable
}

void speaker_config(void){

	TA1CCTL0 &= ~CCIE;			// disenable capture and compare control on TA1CCTL0
	IE2 &= ~UCA0RXIE;
	BCSCTL1 = CALBC1_1MHZ;						// Run at 16 MHz
	DCOCTL  = CALDCO_1MHZ;

	WDTCTL = WDTPW + WDTTMSEL + WDTCNTCL + WDTIS1; // Set interval mode, set to zero and interval to 0.5 ms
	IE1 |= WDTIE; // Enable WDT interrupt

	__enable_interrupt();

	P1DIR |= AUD; // P1.6 to output
	P1SEL |= AUD; // P1.6 to TA0.1

	CCTL1 = OUTMOD_7; // CCR1 reset/set

	byte default_dur = 4;
	byte default_oct = 6;
	int bpm = 63;
	int num;
	long wholenote;
	long duration;
	byte note;
	byte scale;

	char *p;

	p = warning;

	while (*p != ':')
		p++; // ignore name
		p++; // skip ':'
	// get default duration
	if (*p == 'd') {
		p++;
		p++; // skip "d="
		num = 0;
		while (isdigit(*p)) {
			num = (num * 10) + (*p++ - '0');
		}
		if (num > 0)
			default_dur = num;
		p++; // skip comma
	}
	// get default octave
	if (*p == 'o') {
		p++;
		p++; // skip "o="
		num = *p++ - '0';
		if (num >= 3 && num <= 7)
			default_oct = num;
		p++; // skip comma
	}
	// get BPM
	if (*p == 'b') {
		p++;
		p++; // skip "b="
		num = 0;
		while (isdigit(*p)) {
			num = (num * 10) + (*p++ - '0');
		}
		bpm = num;
		p++; // skip colon
	}
			// BPM usually expresses the number of quarter notes per minute
	wholenote = (60 * 1000L / bpm) * 4; // this is the time for whole note (in milliseconds)
			// now begin note loop
	while (*p) {
		// first, get note duration, if available
		num = 0;
		while (isdigit(*p)) {
			num = (num * 10) + (*p++ - '0');
		}

		if (num)
			duration = wholenote / num;
		else
			duration = wholenote / default_dur; // we will need to check if we are a dotted note after

		// now get the note
		note = 0;
		switch (*p) {
		case 'c':
			note = 1;
			break;
		case 'd':
			note = 3;
			break;
		case 'e':
			note = 5;
			break;
		case 'f':
			note = 6;
			break;
		case 'g':
			note = 8;
			break;
		case 'a':
			note = 10;
			break;
		case 'b':
			note = 12;
			break;
		case 'p':
			default:
			note = 0;
		}
		p++;
		// now, get optional '#' sharp
		if (*p == '#') {
			note++;
			p++;
		}
		// now, get optional '.' dotted note
		if (*p == '.') {
			duration += duration / 2;
			p++;
		}
		// now, get scale
		if (isdigit(*p)) {
			scale = *p - '0';
			p++;
		} else {
			scale = default_oct;
		}
		scale += OCTAVE_OFFSET;

		if (*p == ',')
			p++; // skip comma for next note (or we may be at the end)
		// now play the note

		if (note) {
			play(notes[(scale - 4) * 12 + note]);
			delay(duration);
			stop();
		} else {
			delay(duration);
		}
	}
	__disable_interrupt();

	TA1CCTL0 |= CCIE;			// enable capture and compare control on TA1CCTL0
	IE2 |= UCA0RXIE;
	BCSCTL1 = CALBC1_16MHZ;						// Run at 16 MHz
	DCOCTL  = CALDCO_16MHZ;
	P1OUT |= 0;
}

/** Delay function. **/
void delay(unsigned int ms) {
	unsigned int ik, ms2; //Modified here, eliminated volatile
	ik = time;
	ms2 = ms * 2;
	while ((time - ik) < ms2) {
		__no_operation();
	}
}

void play(unsigned int hz) {
	CCR0 = (1000000 / hz) - 1;
	CCR1 = (1000000 / hz) / 2;
	TACTL = TASSEL_2 + MC_1;
}

void stop(void) {
	TACTL = TASSEL_2 + MC_3; //stop
	CCR0 = 0;
}

#pragma vector=WDT_VECTOR
__interrupt void watchdog_timer(void) //__interrupt void watchdog_timer
{
	time++;
}

void CalibrateADC(void)
{
	P1DIR &= ~BIT0;				// set P1.0 to 0, as Ultrasonic sensor input
	P1SEL |= BIT0;				// set P1.0 SEL to 0
	ADC10AE0 |= BIT0;			// P1.0 ADC enable analog input
	ADC10CTL0 &= ~ENC;			// Disable ADC10 for initialization

	ADC10CTL0 = ADC10SHT_3 + ADC10ON + ADC10IE + REF2_5V;
	// Set Sample & Hold Time; external reference voltage V+ by P1.3, and V- by P1.4; ADC10 ON; Interrupt Enable

	__delay_cycles(3000);	// Wait for the reference voltage settle down

	ADC10CTL1 = ADC10SSEL_1 + INCH_1;// set ACLK as clock source for ADC10; selects channel 1
}

void CalibrateUART(void)
{
	P1DIR |= BIT7; 				// Set the LEDs on P1.0, P1.6 as outputs
	P1OUT = 0;
												/* Configure hardware UART */
	P1SEL |= BIT1 + BIT2; 						// P1.1 = RXD, P1.2=TXD
	P1SEL2 |= BIT1 + BIT2; 						// P1.1 = RXD, P1.2=TXD

	UCA0CTL1 |= UCSSEL_2; 						// Use SMCLK
	UCA0BR0 = 130; 								// Set baud rate to 9600 with 16MHz clock (According to User Guide 15.3.13)
	UCA0BR1 = 6; 								// Set baud rate to 9600 with 16MHz clock	130 + 6 * 256 = 1666
	UCA0MCTL = UCBRS0 + UCBRS1; 				// Modulation UCBRSx = 5.3333336; Sixth modulation stage select.
	UCA0CTL1 &= ~UCSWRST; 						// Initialize USCI state machine; software reset disabled, USCI released for operation

	int j;
	for (j = 0; j < 64; ++j) {					// initialize the array of character to all zero in entries
		command[j] = 0;
	}
	IE2 |= UCA0RXIE; 							// Enable USCI_A0 RX interrupt
	__bis_SR_register(GIE); 					// Enter LPM1, interrupts enabled
}

#pragma vector = TIMER1_A0_VECTOR	// says that the interrupt that follows will use the "TIMER0_A0_VECTOR" interrupt
__interrupt void Timer_A(void){		// can name the actual function anything
	ADC10CTL0 |= ENC + ADC10SC;		// Enable ADC Conversion and conversion start
}

// ADC10 interrupt service routine
#pragma vector = ADC10_VECTOR
__interrupt void ADC10_ISR(void)
{

	int pixelIntensity;
	float distance;
	int digit;

	int k;
	for (k = 0; k < 16; ++k) {					// initialize the array of character to all zero in entries
		distChar[k] = 0;
	}

	int t;
	for (t = 0; t < 15; ++t) {		// initialize the array of character to all zero in entries
		distChar[t] = 0;
	}

	char *ipDisChar = &distChar[0];
	//distance = ADC10MEM;
	if (ADC10MEM < 14){											// if distance is near, set neopixel to full intensity
		//ADC10MEM = 13;
		pixelIntensity = 180;
		speaker_config();
	}
	else if (ADC10MEM > 60){									// if distance is far away, set neopixel to off
		//ADC10MEM = 60;
		pixelIntensity = 0;
	}
	else{
		pixelIntensity = ADC10MEM * (-3.829787) + 229.787234; 	// ADC10MEM is a number range from 13 - 60, and we map this range to 180 - 0,
																//  which is the the range for neopixel intensity (the actual range is 0 - 255, which is too bright)
//		speaker_config();
	}

	distance = (pixelIntensity * (-0.174978) + 31.496);		// convert pixel intensity to corresponding distance in inches
	int dist = (int)(distance);								// type cast floating point distance to integer dist

	int n;
	if (dist > 9){
		n = 2;											// distance is in range 0 - 9, so 1 digits
		ipDisChar++;									// if two digits, increase the pointer to second entry of distChar
	}
	else{
		n = 1;											// distance is in range 10 - 31, so 2 digits
	}

	int i;
	for (i = 0; i < n; ++i, dist /= 10 ){				// convert integer to array of character
		digit = dist % 10;
		*ipDisChar-- = digit + '0';
	}

	int displayArry[5] = {0};
	if (pixelIntensity < ThresDist1){
		setNeoPixel(displayArry, pixelIntensity, pixelIntensity, pixelIntensity);
		//__delay_cycles(1000000);
	}
	else if((pixelIntensity >= ThresDist1) && (pixelIntensity < ThresDist2)){
		setNeoPixel(displayArry, 0, 179, 0);					// display neopixel in all red to warm distance is too near
		//__delay_cycles(4000000);
	}
	else if(pixelIntensity >= ThresDist2){
		int j;
		for (j = 3; j >= 0; j --){
			setNeoPixel(displayArry, 0, 180, 0);					// display neopixel in all red and blink to warm distance is too near
			//__delay_cycles(4000000);
			setNeoPixel(displayArry, 0, 0, 0);
			//__delay_cycles(4000000);
		}
	}
}

void sendDistance(char currentDist[16]){		// send current distance value to terminal
	UARTSendArray("\r");						// erase the user typed array, start from the beginning of the line
	UARTSendArray("Current distance: ");
	UARTSendArray(currentDist);
	UARTSendArray(" inches.");
	UARTSendArray("\n\r");
}
												// Echo back RXed character, confirm TX buffer is ready first
#pragma vector = USCIAB0RX_VECTOR
__interrupt void USCI0RX_ISR(void) {
	if (UCA0RXBUF != 0x0D){						// 0x0D is enter in ascii
		*ip = UCA0RXBUF;
		UARTSendArray(ip);
		++ip;
	}
	else {
		// data = UCA0RXBUF;
		// UARTSendArray("Received command: ", 18);
		UARTSendArray("\r");						// erase the user typed array, start from the beginning of the line
		UARTSendArray("Received Command: ");
		UARTSendArray(command);
		UARTSendArray("\n\r");

		switch(command[0]){																		// 0x7B is ascii for '{'
			case 't': {								// s stands for set, eg: set({1,2,3},100,200,150)    0x28 is ascii for '('    0x29 is ascii for )
				if (command[1] != 'h' || command[2] != 'r'|| command[3] != 'e' || command[4] != 's' || command[5] != 'h' || command[6] != 'o' || command[7] != 'l' || command[8] != 'd' || command[9] != 0x20 || command[10] != 0x28 || command[15] != 0x20 ||command[19] != 0x29 ||command[20] != 0){
					UARTSendArray("Unknown Command: ");
					UARTSendArray(command);
					UARTSendArray("\n\r");
				}
				else {		// 0x20 is ascii for space
					int q;
					for (q = 0; q < 2; ++q) {		// initialize the array of character to all zero in entries
						GRB[q] = 0;
					}
					ipGRB = &GRB[0];

					char *ipCommand = &command[0];					// e.g. set({1,2,3}, 100, 210, 250)
					while (*ipCommand){								// this part is extracting {1,2,3} into another global array of integer
						if (*ipCommand == 0x28) {
							ipCommand ++;						// e.g. set({1,2,3}, 100, 210, 250)
													// this part is extracting 100, 210, 250, convert to integer, and store into global int variable
							while (*ipCommand != 0x29) {		// 0x29 is ascii for )
								char arry[3];
								int p;
								for (p = 0; p < 3; ++p) {		// initialize the array of character to all zero in entries
									arry[p] = 0;
								}
								char *ipArry = &arry[0];			// 0x2C is ascii for ,
								while ((*ipCommand != 0x2C) && (*ipCommand != 0x29)){
									if (*ipCommand != 0x20){
										*ipArry++ = *ipCommand;
										//ipArry ++;
									}
									ipCommand ++;
								}
								*ipGRB++ = atoi(arry);			// convert an array of character to an integer
								if (*ipCommand != 0x29) {		// only increment pointer if pointer is not pointing to a left parenthesis
									ipCommand ++;
								}
							}
						}
						if (*ipCommand != 0x29) {				// if haven't reach the end of command, which is ')'
							ipCommand ++;
						}
						else {
							ipCommand = &command[63];			// if reached end of command, let pointer point to end of array of character command
						}
					}
					if ((GRB[0] >= 155) && (GRB[0] <= 165)){
						ThresDist1 = GRB[0];
					}
					else {
						UARTSendArray("Low threshold out of range 155 - 165");
						UARTSendArray("\n\r");
					}
					if ((GRB[1] >= 174) && (GRB[1] <= 179)){
						ThresDist2 = GRB[1];
					}
					else {
						UARTSendArray("Up threshold out of range 174 - 179");
						UARTSendArray("\n\r");
					}
				}
			}
			break;

			case 'd': {
				//P1OUT &= ~BIT0;							0x20 is ascii for space
				if (command[1] != 'i' || command[2] != 's' || command[3] != 't'|| command[4] != 'a' || command[5] != 'n' || command[6] != 'c' || command[7] != 'e' ||command[8] != 0){
					UARTSendArray("Unknown Command: ");
					UARTSendArray(command);
					UARTSendArray("\n\r");
				}
				else{
					sendDistance(distChar);								// send distChar to display on Tera Term
				}
			}
			break;

			default: {
				UARTSendArray("Unknown Command: ");
				UARTSendArray(command);
				UARTSendArray("\n\r");
			}
			break;
		}

		int n;
		for (n = 0; n < 64; ++n) {		// initialize the array of character to all zero in entries
			command[n] = 0;
		}
		ip = &command[0];				// initialize the pointer to point to first element of commend
	}
}

void setNeoPixel(int pixel[5], int G, int R, int B){
	unsigned x;
	if (pixel[0] == 0){					// the case which PIXEL is an array with only 0, then set all Neopixels will specified color
		for(x = 0; x < 5 * 3;) {
			z[x++] = G;
			z[x++] = R;
			z[x++] = B;
		}
		write_ws2811(z, sizeof(z), BIT7);
	}
	else{
		for(x = 0; x < 5; ++x) {		// the case which PIXEL is an array with certain number from 1 - 5, then set specified Neopixels to certain color
			z[pixel[x] * 3 - 3] = G;
			z[pixel[x] * 3 - 2] = R;
			z[pixel[x] * 3 - 1] = B;
			write_ws2811(z, sizeof(z), BIT7);
		}
	}
}
										// Send number of bytes Specified in ArrayLength in the array at using the hardware UART 0
void UARTSendArray(char *TxArray){
 	while(*TxArray){ 					// Loop through the TxArray
		while(!(IFG2 & UCA0TXIFG)); 	// Wait for TX buffer to be ready for new data
		UCA0TXBUF = *TxArray++;			// Write the character at the location specified by the pointer
	}
}
