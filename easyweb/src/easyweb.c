/******************************************************************
 *****                                                        *****
 *****  Name: easyweb.c                                       *****
 *****  Ver.: 1.0                                             *****
 *****  Date: 07/05/2001                                      *****
 *****  Auth: Andreas Dannenberg                              *****
 *****        HTWK Leipzig                                    *****
 *****        university of applied sciences                  *****
 *****        Germany                                         *****
 *****        adannenb@et.htwk-leipzig.de                     *****
 *****  Func: implements a dynamic HTTP-server by using       *****
 *****        the easyWEB-API                                 *****
 *****  Rem.: In IAR-C, use  linker option                    *****
 *****        "-e_medium_write=_formatted_write"              *****
 *****                                                        *****
 ******************************************************************/

// Modifications by Code Red Technologies for NXP LPC1768

// CodeRed - removed header for MSP430 microcontroller
//#include "msp430x14x.h"

#include "stdlib.h"
#include "stdio.h"
#include "stdint.h"

#include "string.h"

//Include the driver of temperature.
#include "light.h"

// CodeRed - added #define extern on next line (else variables
// not defined). This has been done due to include the .h files 
// rather than the .c files as in the original version of easyweb.
#define extern 

#include "easyweb.h"

// CodeRed - removed header for original ethernet controller
//#include "cs8900.c"                              // ethernet packet driver

//CodeRed - added for LPC ethernet controller
#include "ethmac.h"

// CodeRed - include .h rather than .c file
// #include "tcpip.c"                               // easyWEB TCP/IP stack
#include "tcpip.h"                               // easyWEB TCP/IP stack

// CodeRed - added NXP LPC register definitions header
#include "LPC17xx.h"


// CodeRed - include renamed .h rather than .c file
// #include "webside.c"                             // webside for our HTTP server (HTML)
#include "webside.h"                             // webside for our HTTP server (HTML)

#include "lpc17xx_pinsel.h"
#include "lpc17xx_gpio.h"
#include "lpc17xx_ssp.h"
#include "oled.h"

// import the top library....
#include "top.h"


// CodeRed - added for use in dynamic side of web page
unsigned int aaPagecounter=0;
unsigned int adcValue = 0;

// print text in console
void print(char* text) {
	fprintf(stderr, text);
}

static void init_ssp(void)
{
	SSP_CFG_Type SSP_ConfigStruct;
	PINSEL_CFG_Type PinCfg;

	/*
	 * Initialize SPI pin connect
	 * P0.7 - SCK;
	 * P0.8 - MISO
	 * P0.9 - MOSI
	 * P2.2 - SSEL - used as GPIO
	 */
	PinCfg.Funcnum = 2;
	PinCfg.OpenDrain = 0;
	PinCfg.Pinmode = 0;
	PinCfg.Portnum = 0;
	PinCfg.Pinnum = 7;
	PINSEL_ConfigPin(&PinCfg);
	PinCfg.Pinnum = 8;
	PINSEL_ConfigPin(&PinCfg);
	PinCfg.Pinnum = 9;
	PINSEL_ConfigPin(&PinCfg);
	PinCfg.Funcnum = 0;
	PinCfg.Portnum = 2;
	PinCfg.Pinnum = 2;
	PINSEL_ConfigPin(&PinCfg);

	SSP_ConfigStructInit(&SSP_ConfigStruct);

	// Initialize SSP peripheral with parameter given in structure above
	SSP_Init(LPC_SSP1, &SSP_ConfigStruct);

	// Enable SSP peripheral
	SSP_Cmd(LPC_SSP1, ENABLE);

}

static void init_i2c(void)
{
	PINSEL_CFG_Type PinCfg;

	/* Initialize I2C2 pin connect */
	PinCfg.Funcnum = 2;
	PinCfg.Pinnum = 10;
	PinCfg.Portnum = 0;
	PINSEL_ConfigPin(&PinCfg);
	PinCfg.Pinnum = 11;
	PINSEL_ConfigPin(&PinCfg);

	// Initialize I2C peripheral
	I2C_Init(LPC_I2C2, 100000);

	/* Enable I2C1 operation */
	I2C_Cmd(LPC_I2C2, ENABLE);
}

int getLuz(){
	return light_read();
}

char buff_luz[50];
void upadateSensorLuz(int x, int y) {
    sprintf((char*)buff_luz, "Sensor luz: %d ", getLuz());
	oled_putString(x,y, (uint8_t*)buff_luz, OLED_COLOR_BLACK, OLED_COLOR_WHITE);
}
char buff_luz_p[50];
int getLuzPercent() {
	return getLuz() / 9.72;
}
void showLuzPercente() {
	int luxp = getLuzPercent();
	sprintf((char*)buff_luz_p, "Luz per: %d ", luxp);
	oled_putString(0,35, (uint8_t*)buff_luz_p, OLED_COLOR_BLACK, OLED_COLOR_WHITE);
	oled_fillRect(0,50,luxp*0.8,55,OLED_COLOR_WHITE);
	oled_fillRect(0,50,(100 - luxp)*0.8 ,55,OLED_COLOR_BLACK);
}

int main (void){
	print("Starting...\n");
	uint8_t buf[50];
    init_ssp();
    init_i2c();

    oled_init();
    oled_clearScreen(OLED_COLOR_WHITE);
    		     //x,y
    oled_putString(1,3,  (uint8_t*)"________________", OLED_COLOR_BLACK, OLED_COLOR_WHITE);

    oled_putString(1,1,  (uint8_t*)"PROJETO TOP.", OLED_COLOR_BLACK, OLED_COLOR_WHITE);
    sprintf((char*)buf, "IP:%d.%d.%d.%d", MYIP_1, MYIP_2, MYIP_3, MYIP_4);
    oled_putString(1,14, (uint8_t*)buf, OLED_COLOR_BLACK, OLED_COLOR_WHITE);
    oled_line(0,10,95,10,OLED_COLOR_BLACK);

    light_init();
    light_enable();
    top_init();
	TCPLowLevelInit();

	HTTPStatus = 0;                                // clear HTTP-server's flag register

	TCPLocalPort = TCP_PORT_HTTP;                  // set port we want to listen to

//	char buf[12];
//	sprintf((char*)buf, "%d.%d.%d.%d", MYIP_1, MYIP_2, MYIP_3, MYIP_4);


	int x = 0;
	  while (1) {
		x++;
		if (x > 1000){
			upadateSensorLuz(1, 25);
			showLuzPercente();
			top_update_values(aaPagecounter, buf, getLuz(), getLuzPercent());
			x = 0;
		}

		if (!(SocketStatus & SOCK_ACTIVE)) TCPPassiveOpen();   // listen for incoming TCP-connection
		DoNetworkStuff();                                      // handle network and easyWEB-stack
															   // events
		HTTPServer();
	  }
}

// This function implements a very simple dynamic HTTP-server.
// It waits until connected, then sends a HTTP-header and the
// HTML-code stored in memory. Before sending, it replaces
// some special strings with dynamic values.
// NOTE: For strings crossing page boundaries, replacing will
// not work. In this case, simply add some extra lines
// (e.g. CR and LFs) to the HTML-code.

void HTTPServer(void)
{
  if (SocketStatus & SOCK_CONNECTED)             // check if somebody has connected to our TCP
  {
    if (SocketStatus & SOCK_DATA_AVAILABLE)      // check if remote TCP sent data
      TCPReleaseRxBuffer();                      // and throw it away

    if (SocketStatus & SOCK_TX_BUF_RELEASED)     // check if buffer is free for TX
    {
      if (!(HTTPStatus & HTTP_SEND_PAGE))        // init byte-counter and pointer to webside
      {                                          // if called the 1st time
        HTTPBytesToSend = sizeof(WebSide) - 1;   // get HTML length, ignore trailing zero
        PWebSide = (unsigned char *)WebSide;     // pointer to HTML-code
      }

      if (HTTPBytesToSend > MAX_TCP_TX_DATA_SIZE)     // transmit a segment of MAX_SIZE
      {
        if (!(HTTPStatus & HTTP_SEND_PAGE))           // 1st time, include HTTP-header
        {
          print("Make response.\n");
          ++aaPagecounter;

          memcpy(TCP_TX_BUF, GetResponse, sizeof(GetResponse) - 1);
          memcpy(TCP_TX_BUF + sizeof(GetResponse) - 1, PWebSide, MAX_TCP_TX_DATA_SIZE - sizeof(GetResponse) + 1);
          HTTPBytesToSend -= MAX_TCP_TX_DATA_SIZE - sizeof(GetResponse) + 1;
          PWebSide += MAX_TCP_TX_DATA_SIZE - sizeof(GetResponse) + 1;
        }
        else
        {
          memcpy(TCP_TX_BUF, PWebSide, MAX_TCP_TX_DATA_SIZE);
          HTTPBytesToSend -= MAX_TCP_TX_DATA_SIZE;
          PWebSide += MAX_TCP_TX_DATA_SIZE;
        }


        TCPTxDataCount = MAX_TCP_TX_DATA_SIZE;   // bytes to xfer
        InsertDynamicValues();                   // exchange some strings...
        TCPTransmitTxBuffer();                   // xfer buffer
      }
      else if (HTTPBytesToSend)                  // transmit leftover bytes
      {
        memcpy(TCP_TX_BUF, PWebSide, HTTPBytesToSend);
        TCPTxDataCount = HTTPBytesToSend;        // bytes to xfer
        InsertDynamicValues();                   // exchange some strings...
        TCPTransmitTxBuffer();                   // send last segment
        TCPClose();                              // and close connection
        HTTPBytesToSend = 0;                     // all data sent
      }

      HTTPStatus |= HTTP_SEND_PAGE;              // ok, 1st loop executed
    }
  }
  else
    HTTPStatus &= ~HTTP_SEND_PAGE;               // reset help-flag if not connected
}




// Code Red - GetAD7Val function replaced
// Rather than using the AD convertor, in this version we simply increment
// a counter the function is called, wrapping at 1024. 
volatile unsigned int aaScrollbar = 400;

unsigned int GetAD7Val(void)
{
  aaScrollbar = (aaScrollbar +16) % 1024;
  adcValue = (aaScrollbar / 10) * 1000/1024;
  return aaScrollbar;
}

void InsertDynamicValues(void)
{
  unsigned char *key;
  char NewKey[9000];
  unsigned int i;

  if (TCPTxDataCount < 4) return;                     // there can't be any special string

  key = TCP_TX_BUF;
  struct TOP_INFO* info = getTopInfo();


  for (i = 0; i < (TCPTxDataCount - 3); i++)
  {

    if (*key == 'A')
     if (*(key + 1) == 'D')
       if (*(key + 3) == '%')
         switch (*(key + 2))
         {
           case '1' ://CONTADOR                                 // "AD1%"?
			 {

				 sprintf(NewKey, "%4u",info->counter);    // increment and insert page counter
				 memcpy(key, NewKey, 4);

				 break;
			 }
           case '2' : { // ip

					char buf[12];
					sprintf((char*)buf, "%d.%d.%d.%d", MYIP_1, MYIP_2, MYIP_3, MYIP_4);

					sprintf(NewKey, "%s",buf);
					memcpy(key, NewKey,sizeof(buf));

				   break;
				 }
           case '3' : //luz
				 {
				   sprintf(NewKey, "%4u",  info->luz);
				   memcpy(key, NewKey, 4);
				   break;
				 }
           case '4' : //luz percente
				 {
				   sprintf(NewKey, "%3d",  info->luzPercent);
				   memcpy(key, NewKey, 3);
				   break;
				 }
         }
    key++;
  }



// Code Red - Original MSP430 version of GetAD7Val() removed
/*
// samples and returns the AD-converter value of channel 7
// (associated with Port P6.7)

unsigned int GetAD7Val(void)
{
  ADC12CTL0 = ADC12ON | SHT0_15 | REF2_5V | REFON;   // ADC on, int. ref. on (2,5 V),
                                                     // single channel single conversion
  ADC12CTL1 = ADC12SSEL_2 | ADC12DIV_7 | CSTARTADD_0 | SHP;// MCLK / 8 = 1 MHz

  ADC12MCTL0 = SREF_1 | INCH_7;                  // int. ref., channel 7
  
  ADC12CTL0 |= ENC;                              // enable conversion
  ADC12CTL0 |= ADC12SC;                          // sample & convert
  
  while (ADC12CTL0 & ADC12SC);                   // wait until conversion is complete
  
  ADC12CTL0 &= ~ENC;                             // disable conversion

  return ADC12MEM0 / 41;                         // scale 12 bit value to 0..100%
}

// End of Original MSP430 version of GetAD7Val()
*/


// Code Red - Original GetTempVal() removed
// Function no longer used
/*
// samples and returns AD-converter value of channel 10
// (MSP430's internal temperature reference diode)
// NOTE: to get a more exact value, 8-times oversampling is used

unsigned int GetTempVal(void)
{
  unsigned long ReturnValue;

  ADC12CTL0 = ADC12ON | SHT0_15 | MSH | REFON;   // ADC on, int. ref. on (1,5 V),
                                                 // multiple sample & conversion
  ADC12CTL1 = ADC12SSEL_2 | ADC12DIV_7 | CSTARTADD_0 | CONSEQ_1 | SHP;   // MCLK / 8 = 1 MHz

  ADC12MCTL0 = SREF_1 | INCH_10;                 // int. ref., channel 10
  ADC12MCTL1 = SREF_1 | INCH_10;                 // int. ref., channel 10
  ADC12MCTL2 = SREF_1 | INCH_10;                 // int. ref., channel 10
  ADC12MCTL3 = SREF_1 | INCH_10;                 // int. ref., channel 10
  ADC12MCTL4 = SREF_1 | INCH_10;                 // int. ref., channel 10
  ADC12MCTL5 = SREF_1 | INCH_10;                 // int. ref., channel 10
  ADC12MCTL6 = SREF_1 | INCH_10;                 // int. ref., channel 10
  ADC12MCTL7 = EOS | SREF_1 | INCH_10;           // int. ref., channel 10, last seg.
  
  ADC12CTL0 |= ENC;                              // enable conversion
  ADC12CTL0 |= ADC12SC;                          // sample & convert
  
  while (ADC12CTL0 & ADC12SC);                   // wait until conversion is complete
  
  ADC12CTL0 &= ~ENC;                             // disable conversion

  ReturnValue = ADC12MEM0;                       // sum up values...
  ReturnValue += ADC12MEM1;
  ReturnValue += ADC12MEM2;
  ReturnValue += ADC12MEM3;
  ReturnValue += ADC12MEM4;
  ReturnValue += ADC12MEM5;
  ReturnValue += ADC12MEM6;
  ReturnValue += ADC12MEM7;

  ReturnValue >>= 3;                             // ... and divide by 8

  if (ReturnValue < 2886) ReturnValue = 2886;    // lower bound (0% = 20�C)
  ReturnValue = (ReturnValue - 2886) / 2.43;     // convert AD-value to a temperature from
                                                 // 20�C...45�C represented by a value
                                                 // of 0...100%
  if (ReturnValue > 100) ReturnValue = 100;      // upper bound (100% = 45�C)

  return ReturnValue;
}
// End of Original MSP430 version of GetTempVal()
*/


// searches the TX-buffer for special strings and replaces them
// with dynamic values (AD-converter results)

// Code Red - new version of InsertDynamicValues()

}

