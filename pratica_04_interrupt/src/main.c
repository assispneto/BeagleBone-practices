/*
 * =====================================================================================
 *
 *       Filename:  main.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  15/05/2018 14:32:47
 *       Revision:  none
 *       Compiler:  gcc
 *
 * =====================================================================================
 */

#include "bbb_regs.h"
#include "hw_types.h"

// Defini minhas macros
#define TIME   10000000
#define pino1   13
#define pino2   15

// Flag para ativar botao
bool flag_gpio1;
bool flag_gpio2;


typedef enum _pinNum{
	PIN1=1,
	PIN2
}pinNum;


/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  disableWdt
 *  Description:  BBB nao reinicia e com isso nao entra no modo kernel 
 * =====================================================================================
 */

void disableWdt(void){
	HWREG(WDT_WSPR) = 0xAAAA;
	while((HWREG(WDT_WWPS) & (1<<4)));
	
	HWREG(WDT_WSPR) = 0x5555;
	while((HWREG(WDT_WWPS) & (1<<4)));
}

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  delay
 *  Description:  
 * =====================================================================================
 */
void delay(unsigned int mSec){
    volatile unsigned int count;
    
	for(count=0; count<mSec; count++);

}

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  putCh
 *  Description:  Inicializa/chama UART
 * =====================================================================================
 */
void putCh(char c){
	while(!(HWREG(UART0_LSR) & (1<<5)));

	HWREG(UART0_THR) = c;
}

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  getCh
 *  Description:  
 * =====================================================================================
 */
char getCh(){
	while(!(HWREG(UART0_LSR) & (1<<0)));

	return(HWREG(UART0_RHR));
}

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  putString
 *  Description:  Pegar a string que a UART vai mostrar 
 * =====================================================================================
 */
int putString(char *str, unsigned int length){
	for(int i = 0; i < length; i++){
    	putCh(str[i]);
	}
	return(length);
}

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  getString
 *  Description:  Acessa o valor da string
 * =====================================================================================
 */
int getString(char *buf, unsigned int length){
	for(int i = 0; i < length; i ++){
    	buf[i] = getCh();
   	}
	return(length);
}

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  gpioSetup
 *  Description:  Configura o clock
 * =====================================================================================
 */
void gpioSetup(){
	/* set clock for GPIO1, TRM 8.1.12.1.31 */
	HWREG(CM_PER_GPIO1_CLKCTRL) = 0x40002;

    /* Interrupt mask */
    HWREG(INTC_MIR_CLEAR3) |= (1<<2);	//(98 --> Bit 2 do 4º registrador (MIR CLEAR3))
	HWREG(INTC_MIR_CLEAR3) |= (1<<3);	// 99
}

/*  * ===  FUNCTION  ======================================================================
 *         Name:  butConfig
 *  Description:  
 * =====================================================================================
 */
void butConfig ( ){

    /*  configure pin mux for input GPIO */
    HWREG(CM_PER_GPMCAD13_REGS) |= 0x2F;
	HWREG(CM_PER_GPMCAD15_REGS) |= 0x2F;	// 2F = 47 -> habilita 0,1,2 - multiplexação UP e DOWN
	
    /* clear pin 23 for input, led USR0, TRM 25.3.4.3 */
    HWREG(GPIO1_OE) |= 1<<pino1;
	HWREG(GPIO1_OE) |= 1<<pino2;
	
	flag_gpio1 = false;
	flag_gpio2 = false;

    /* Setting interrupt GPIO pin. */
	HWREG(GPIO1_IRQSTATUS_SET_0) |= 1<<pino1;
	HWREG(GPIO1_IRQSTATUS_SET_1) |= 1<<pino2; 	 	

  	/* Enable interrupt generation on detection of a rising edge.*/
	// RISING DETECT  -> detecta qnd a interrupcao vai de 0 pra 1
	// FALLING DETECT -> "..." vai de 1 pra 0 (so que n sera nosso caso) 
	HWREG(GPIO1_RISINGDETECT) |= 1<<pino1;
	HWREG(GPIO1_RISINGDETECT) |= 1<<pino2;	
}/* -----  end of function butConfig  ----- */

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  ledConfig
 *  Description:  
 * =====================================================================================
 */
void ledConfig ( ){
    /*  configure pin mux for output GPIO */
    HWREG(CM_PER_GPMCA7_REGS) |= 0x7;
    HWREG(CM_PER_GPMCA8_REGS) |= 0x7;

    /* clear pin 23 and 24 for output, leds USR3 and USR4, TRM 25.3.4.3 */
    HWREG(GPIO1_OE) &= ~(1<<23);
    HWREG(GPIO1_OE) &= ~(1<<24);

}/* -----  end of function ledConfig  ----- */

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  readBut
 *  Description:  Identificar se meu botao esta on (n vai ser necessario pq vou usar inter)
 * =====================================================================================
 */
unsigned int readBut1 ( ){
	unsigned int temp;
	temp = HWREG(GPIO1_DATAIN)&(1<<pino1);
	
	return(temp);
}/* -----  end of function readBut  ----- */
unsigned int readBut2 ( ){
	unsigned int temp;
	temp = HWREG(GPIO1_DATAIN)&(1<<pino2);
	
	return(temp);
}/* -----  end of function readBut  ----- */

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  ledOff
 *  Description:  
 * =====================================================================================
 */
void ledOff(pinNum pin){
	switch (pin) {
		case PIN1:
			HWREG(GPIO1_CLEARDATAOUT) |= (1<<23);
		break;
		case PIN2:	
			HWREG(GPIO1_CLEARDATAOUT) |= (1<<24);
		break;
		default:
		break;
	}/* -----  end switch  ----- */
}

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  ledOn
 *  Description:  
 * =====================================================================================
 */
void ledOn(pinNum pin){
	
	switch (pin) {
		case PIN1:
			HWREG(GPIO1_SETDATAOUT) |= (1<<23);
		break;
		case PIN2:
			HWREG(GPIO1_SETDATAOUT) |= (1<<24);
		break;
		default:
		break;
	}/* -----  end switch  ----- */
}

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  gpioIsrHandler
 *  Description:  
 * =====================================================================================
 */
void gpioIsrHandler1(void){

    /* Clear the status of the interrupt flags */
	HWREG(GPIO1_IRQSTATUS_0) = (1<<pino1); 

	flag_gpio1 = true;
}
void gpioIsrHandler2(void){

    /* Clear the status of the interrupt flags */
	HWREG(GPIO1_IRQSTATUS_1) = (1<<pino2);
 //   HWREG(GPIO1_IRQSTATUS_1) = (1<<pino2);  

	flag_gpio2 = true;
}

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  ISR_Handler
 *  Description:  
 * =====================================================================================
 */
void ISR_Handler(void){
	/* Verify active IRQ number */
	unsigned int irq_number = HWREG(INTC_SIR_IRQ) & 0x7f;

	
	
	if(irq_number == 98){
		gpioIsrHandler1();
	}
	if(irq_number == 99){
		gpioIsrHandler2();
	}
	/* acknowledge IRQ */
	HWREG(INTC_CONTROL) = 0x1;
}

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  main
 *  Description:  
 * =====================================================================================
 */
int main(void){
	
	/* Hardware setup */
	gpioSetup();
	ledConfig();
	butConfig();
	disableWdt();

	putString("gpio Interrupt...\n\r",19);
	ledOn(PIN1);
	ledOn(PIN2);
	//delay(TIME);
	//ledOff(PIN1);
	//ledOff(PIN2);

	while(true){
		if(flag_gpio1){
			putString("Botao 1!\n\r",10);
			ledOn(PIN1);
				delay(TIME);
				delay(TIME);
			ledOn(PIN2);
				delay(TIME);
				delay(TIME);
			ledOff(PIN1);
				delay(TIME);
				delay(TIME);
			ledOff(PIN2);
				delay(TIME);
				delay(TIME);
			flag_gpio1 = false;

		}if(flag_gpio2){
			putString("Botao 2!\n\r",10);
			ledOn(PIN1);
				delay(TIME);
				delay(TIME);
			ledOn(PIN2);
				delay(TIME);
				delay(TIME);
			ledOff(PIN1);
				delay(TIME);
				delay(TIME);
			ledOff(PIN2);
				delay(TIME);
				delay(TIME);
			flag_gpio2 = false;

		} else{
	//		ledOn(PIN1);
	//		    delay(TIME);
			ledOff(PIN1);
	//		    delay(TIME);
	//		ledOn(PIN2);
	//		    delay(TIME);
			ledOff(PIN2);
	//  			delay(TIME);
				
		}
	}

	return(0);
}




