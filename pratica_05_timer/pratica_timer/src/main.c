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
 *         Author:  Francisco Helder (FHC), helderhdw@gmail.com
 *   Organization:  UFC-Quixadá
 *
 * =====================================================================================
 */

#include "bbb_regs.h"
#include "hw_types.h"

#define DELAY_USE_INTERRUPT			1
int time = 10000;

/**
 * \brief   This macro will check for write POSTED status
 *
 * \param   reg          Register whose status has to be checked
 *
 *    'reg' can take the following values \n
 *    DMTIMER_WRITE_POST_TCLR - Timer Control register \n
 *    DMTIMER_WRITE_POST_TCRR - Timer Counter register \n
 *    DMTIMER_WRITE_POST_TLDR - Timer Load register \n
 *    DMTIMER_WRITE_POST_TTGR - Timer Trigger register \n
 *    DMTIMER_WRITE_POST_TMAR - Timer Match register \n
 *
 **/

// Espera e observa se o registrador ta sendo usado ou nao
#define DMTimerWaitForWrite(reg)   \
            if(HWREG(DMTIMER_TSICR) & 0x4)\
            while((reg & HWREG(DMTIMER_TWPS)));


int flag_timer = false;

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  disableWdt
 *  Description:  
 * =====================================================================================
 */

#define pino28   	28
#define pino16   	16
#define pino1   	1
#define pino3   	3
#define pino4   	4

bool flag_gpio1 = false;
bool flag_gpio2 = false;


// CLEARDATAOUT -> limpa a saida (no caso, como vamos
//					colocar no ledOff vai servir para
//					parar de mandar)

void ledOff(int pin){
	switch (pin) {
		case 1:
			HWREG(GPIO2_CLEARDATAOUT) |= (1<<1);
		break;
		case 3:	
			HWREG(GPIO2_CLEARDATAOUT) |= (1<<3);
		break;
		case 4:	
			HWREG(GPIO2_CLEARDATAOUT) |= (1<<4);
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
void ledOn(int pin){
	
// SetDataout ->   liga/seta a saida

	switch (pin) {
		case 1:
			HWREG(GPIO2_SETDATAOUT) |= (1<<1);
		break;
		case 3:
			HWREG(GPIO2_SETDATAOUT) |= (1<<3);
		break;
		case 4:
			HWREG(GPIO2_SETDATAOUT) |= (1<<4);
		break;
		default:
		break;
	}/* -----  end switch  ----- */
}

void butConfig ( ){

    /*  configure pin mux for input GPIO */
    HWREG(CM_PER_GMPCBEN1_REGS) |= 0x2F;
	HWREG(CM_PER_GMPCA0_REGS) 	|= 0x2F;	// 2F = 47 -> habilita 0,1,2 - multiplexação UP e DOWN
	
    /* clear pin 23 for input, led USR0, TRM 25.3.4.3 */
    HWREG(GPIO1_OE) |= 1<<pino28;
	HWREG(GPIO1_OE) |= 1<<pino16;
	
	flag_gpio1 = false;
	flag_gpio2 = false;

    /* Setting interrupt GPIO pin. */
	HWREG(GPIO1_IRQSTATUS_SET_0) |= 1<<pino28;	//	
	HWREG(GPIO1_IRQSTATUS_SET_1) |= 1<<pino16;	//	 	

  	/* Enable interrupt generation on detection of a rising edge.*/
	// RISING DETECT  -> detecta qnd a interrupcao vai de 0 pra 1
	// FALLING DETECT -> "..." vai de 1 pra 0 (so que n sera nosso caso) 
	HWREG(GPIO1_RISINGDETECT) |= 1<<pino28;
	HWREG(GPIO1_RISINGDETECT) |= 1<<pino16;	
}/* -----  end of function butConfig  ----- */

void ledConfig ( ){
    /*  configure pin mux for output GPIO */
    HWREG(CM_PER_GPMC_CLK_REGS) 	|= 0x7;
    HWREG(CM_PER_GPMCOENREN_REGS) 	|= 0x7;
	HWREG(CM_PER_GPMCWEN_REGS) 		|= 0x7;

    /* clear pin 23 and 24 for output, leds USR3 and USR4, TRM 25.3.4.3 */
    HWREG(GPIO2_OE) &= ~(1<<1);
    HWREG(GPIO2_OE) &= ~(1<<3);
	HWREG(GPIO2_OE) &= ~(1<<4);

}

void disableWdt(void){
	HWREG(WDT_WSPR) = 0xAAAA;
	while((HWREG(WDT_WWPS) & (1<<4)));
	
	HWREG(WDT_WSPR) = 0x5555;
	while((HWREG(WDT_WWPS) & (1<<4)));
}


/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  putCh
 *  Description:  
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
 *  Description:  
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
 *  Description:  
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
 *         Name:  timerEnable
 *  Description:  
 * =====================================================================================
 */
void timerEnable(){
    /* Wait for previous write to complete in TCLR */
	DMTimerWaitForWrite(0x1);

    /* Start the timer */
    HWREG(DMTIMER_TCLR) |= 0x1;
}/* -----  end of function timerEnable  ----- */

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  timerDisable
 *  Description:  
 * =====================================================================================
 */
void timerDisable(){
    /* Wait for previous write to complete in TCLR */
	DMTimerWaitForWrite(0x1);

    /* Start the timer */
    HWREG(DMTIMER_TCLR) &= ~(0x1);
}/* -----  end of function timerEnable  ----- */

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  delay
 *  Description:  
 * =====================================================================================
 */
void delay(unsigned int mSec){
#ifdef DELAY_USE_INTERRUPT
    unsigned int countVal = TIMER_OVERFLOW - (mSec * TIMER_1MS_COUNT);

   	/* Wait for previous write to complete */
	DMTimerWaitForWrite(0x2);

    /* Load the register with the re-load value */
	HWREG(DMTIMER_TCRR) = countVal;
	
	flag_timer = false;

    /* Enable the DMTimer interrupts */
	HWREG(DMTIMER_IRQENABLE_SET) = 0x2; 

    /* Start the DMTimer */
	timerEnable();

    while(flag_timer == false);

    /* Disable the DMTimer interrupts */
	HWREG(DMTIMER_IRQENABLE_CLR) = 0x2; 
#else
    while(mSec != 0){
        /* Wait for previous write to complete */
        DMTimerWaitForWrite(0x2);

        /* Set the counter value. */
        HWREG(DMTIMER_TCRR) = 0x0;

        timerEnable();

        while(HWREG(DMTIMER_TCRR) < TIMER_1MS_COUNT);

        /* Stop the timer */
        HWREG(DMTIMER_TCLR) &= ~(0x00000001u);

        mSec--;
    }
#endif
}

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  timerSetup
 *  Description:  
 * =====================================================================================
 */
void timerSetup(void){
    /*  Clock enable for DMTIMER7 TRM 8.1.12.1.25 */
    HWREG(CM_PER_TIMER7_CLKCTRL) |= 0x2;

	/*  Check clock enable for DMTIMER7 TRM 8.1.12.1.25 */    
    while((HWREG(CM_PER_TIMER7_CLKCTRL) & 0x3) != 0x2);

#ifdef DELAY_USE_INTERRUPT
    /* Interrupt mask */
    HWREG(INTC_MIR_CLEAR2) |= (1<<31); // 95 --> Bit 31 do 3º registrador (MIR CLEAR2)
#endif
}

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  gpioSetup
 *  Description:  
 * =====================================================================================
 */
void gpioSetup(){
  	/* set clock for GPIO1, TRM 8.1.12.1.31 */
  	HWREG(CM_PER_GPIO1_CLKCTRL) = 0x40002;
  	HWREG(CM_PER_GPIO2_CLKCTRL) = 0x40002;

  	HWREG(INTC_MIR_CLEAR3) |= (1<<2);	//(98 --> Bit 2 do 4º registrador (MIR CLEAR3))
	HWREG(INTC_MIR_CLEAR3) |= (1<<3);	// 99
/* clear pin 21 for output, led USR0, TRM 25.3.4.3 */
}

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  ledOff
 *  Description:  
 * =====================================================================================
 */

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  ledOn
 *  Description:  
 * =====================================================================================
 */

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  timerIrqHandler
 *  Description:  
 * =====================================================================================
 */
void timerIrqHandler(void){

    /* Clear the status of the interrupt flags */
	HWREG(DMTIMER_IRQSTATUS) = 0x2; 

	flag_timer = true;

    /* Stop the DMTimer */
	timerDisable();
}

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  ISR_Handler
 *  Description:  
 * =====================================================================================
 */

void gpioIsrHandler1(void){

    /* Clear the status of the interrupt flags */
	HWREG(GPIO1_IRQSTATUS_0) = (1<<pino28); 

	flag_gpio1 = true;
}

void gpioIsrHandler2(void){

    /* Clear the status of the interrupt flags */
	HWREG(GPIO1_IRQSTATUS_1) = (1<<pino16);
 //   HWREG(GPIO1_IRQSTATUS_1) = (1<<pino2);  

	flag_gpio2 = true;
}

void ISR_Handler(void){
	/* Verifica se é interrupção do DMTIMER7 */
	unsigned int irq_number = HWREG(INTC_SIR_IRQ) & 0x7f;
	
	if(irq_number == 95)
		timerIrqHandler();
    else if(irq_number == 98)
		gpioIsrHandler1();
	else if(irq_number == 99)

	/* Reconhece a IRQ */
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
	timerSetup();
	disableWdt();

#ifdef DELAY_USE_INTERRUPT 
	putString("Timer Interrupt: ",17);
#else
	putString("Timer: ",7);
#endif
	ledOn(pino1);
	delay(300);
	ledOff(pino1);

	ledOn(pino1);
	delay(300);
	ledOff(pino1);

	ledOn(pino3);
	delay(300);
	ledOff(pino3);

	ledOn(pino3);
	delay(300);
	ledOff(pino3);

	ledOn(pino4);
	delay(300);
	ledOff(pino4);

	ledOn(pino4);
	delay(300);
	ledOff(pino4);

	
	while(true){
		if(flag_gpio1){
			ledOn(pino1);
				delay(time);
			ledOn(pino3);
				delay(time);
			ledOn(pino4);
				delay(time);
			ledOff(pino4);
				delay(time);
			ledOff(pino3);
				delay(time);
			ledOff(pino1);
				delay(time);
			flag_gpio1 = false;
		} else if(flag_gpio2){
			while(true){
			putString("Menu de Opções de Frequência:\n",17);
			putString("(a)    2000 ms",14);
			putString("(b)    3000 ms",14);
			putString("(c)    4000 ms",14);
			char sono = getCh();
			if(sono == 'a'){
				time = 2000;
				break;
			}else if(sono == 'b'){
				time = 3000;
				break;
			}else if(sono == 'c'){
				time = 4000;
				break;
			}else{
			putString("digite uma opcao valida\n",24);
			
			}
			}
			flag_gpio2 = false;

		}else{
			ledOn(pino1);
			ledOn(pino3);
			ledOn(pino4);
				delay(time);
			ledOff(pino4);
			ledOff(pino3);
			ledOff(pino1);
				delay(time);
		}

	}
	putString("...OK",5);

	return(0);
}




