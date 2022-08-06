/*
 * =====================================================================================
 *
 *       Filename:  main.c
 *
 *    Description:  Projeto de um sistema de alerta para janelas, utilizando um buzzer
 * 					e um led para advertir caso a janela esteja aberta e alguem fique
 * 					em cima, correndo risco de queda.
 *
 *        Version:  1.0.0
 *        Created:  12/07/2022 12:02:02
 *       Revision:  none
 *       Compiler:  gcc
 * 
 * 	   Components:	Placa Beaglebone Black e os seguintes cabos para conexao da mesma: 
 *     				cabo de rede (RJ45), USB e cabo TTL. 
 * 					Componentes adicionais: 1 protoboard, 20 jumpers, 3 leds, 1 botao,
 * 					modulo flying-fish e 3 resistores.
 *
 *        Authors:  Assis Paiva Neto, Mickaelly Freitas Nobre, João Pedro Fernandes
 * 		  contact:  {assispaivaneto, mickaelly01nobre, joao.pedroeng}@gmail.com
 * 		  Teacher:	Francisco Helder
 *   Organization:  Federal University of Ceara (UFC) - Quixada, Ceara, Brazil
 *
 * =====================================================================================
 */

#include "bbb_regs.h"
#include "hw_types.h"
#include "timer.h"

bool flag_gpio = false; // sensor de inclinação
bool flag2_gpio = false; // botão
int frequencia = 500; // delay

typedef enum _pinNum{
	PIN1=1, // led vermelho
	PIN2, // led amarelo
	PIN3 // led azul
}pinNum;

typedef enum _buzzNum{
	BUZZ1=1
}buzzNum;


/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  disableWdt
 *  Description:  Desabilita o wdt
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
 *         Name:  putCh
 *  Description:  Imprime um caractere na tela
 * =====================================================================================
 */
void putCh(char c){
	while(!(HWREG(UART0_LSR) & (1<<5)));

	HWREG(UART0_THR) = c;
}

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  getCh
 *  Description:  Captura um caractere de uma entrada externa
 * =====================================================================================
 */
char getCh(){
	while(!(HWREG(UART0_LSR) & (1<<0)));

	return(HWREG(UART0_RHR));
}

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  putString
 *  Description:  Escreve uma 'cadeia' de caracteres
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
 *  Description:  Leio uma 'cadeia' de caracteres
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
 *         Name:  butConfig
 *  Description:  Configuracao dos modulos externos utilizados na BBB
 * =====================================================================================
 */
void butConfig ( ){
    /*  configure pin mux for input GPIO */
    HWREG(CM_PER_GPMCAD14_REGS) |= 0x2F; // sensor
    HWREG(CM_PER_GPMCAD12_REGS) |= 0x2F;  // botão
	
    /* clear pin 23 for input, led USR0, TRM 25.3.4.3 */
    HWREG(GPIO1_OE) |= 1<<14;	// Sensor de inclinacao
    HWREG(GPIO1_OE) |= 1<<12;	// Botao

	flag_gpio = false;
	flag2_gpio = false;

    /* Setting interrupt GPIO pin. */
    //habilitam um evento de interrupção específico para disparar um
	//pedido de interrupção. Escrever um 1 para um bit habilita o campo de interrupção. Escrever um 0 não tem efeito, ou seja, o
	//valor de registro não é modificado.
	HWREG(GPIO1_IRQSTATUS_SET_0) |= 1<<14; 	 // sensor
	HWREG(GPIO1_IRQSTATUS_SET_1) |= 1<<12;   // botao

  	/* Enable interrupt generation on detection of a rising edge.*/
	// vai de 0 para 1
	HWREG(GPIO1_RISINGDETECT) |= 1<<14;	
	HWREG(GPIO1_RISINGDETECT) |= 1<<12;	
}/* -----  end of function butConfig  ----- */


/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  gpioSetup
 *  Description:  Configuracao de GPIO
 * =====================================================================================
 */
void gpioSetup(){
	/* set clock for GPIO1, TRM 8.1.12.1.31 */
  	HWREG(CM_PER_GPIO1_CLKCTRL) = 0x40002;

 	/* Interrupt mask */
    //Este registrador é usado para limpar os bits da máscara de interrupção (mir clear)
    HWREG(INTC_MIR_CLEAR3) |= (1<<2);	// (98 --> Bit 2 do 4º registrador (MIR CLEAR3)) - A
    HWREG(INTC_MIR_CLEAR3) |= (1<<3);	// (99 --> Bit 3 do 4º registrador (MIR CLEAR3)) - B

  	/* clear pin 21 for output, led USR0, TRM 25.3.4.3 */
  	HWREG(GPIO1_OE) &= ~(1<<13);	// Pino 11 (led vermelho)
  	HWREG(GPIO1_OE) &= ~(1<<15);	// Pino 15 (led amarelo)
    HWREG(GPIO1_OE) &= ~(1<<29);	// Pino 26 (led azul)
	HWREG(GPIO1_OE) &= ~(1<<28);	// buzzer 
  
 	/* for leds */
  	HWREG(CM_PER_GPMCAD13_REGS) 	|= 0x7; 
  	HWREG(CM_PER_GPMCAD15_REGS) 	|= 0x7;
  	HWREG(CM_PER_GPMC_CSn0_REGS) 	|= 0x7;
	HWREG(CM_PER_GPMC_BEN1_REGS) 	|= 0x7;
}
/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  ledOff
 *  Description:  Desligo um led
 * =====================================================================================
 */
void ledOff(pinNum pin){
	switch (pin) {
		case PIN1:
  			HWREG(GPIO1_CLEARDATAOUT) = (1<<13);
  		break;
  		case PIN2:
  			HWREG(GPIO1_CLEARDATAOUT) = (1<<15);
  		break;
  		case PIN3:
  			HWREG(GPIO1_CLEARDATAOUT) = (1<<29);
  		break;
		default:
		break;
	}
}

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  ledOn
 *  Description:  Aciono um led
 * =====================================================================================
 */
void ledOn(pinNum pin){
   	switch (pin) {
		case PIN1:
  			HWREG(GPIO1_SETDATAOUT) = (1<<13); //LED VERMELHO
  		break;
  		case PIN2:
  			HWREG(GPIO1_SETDATAOUT) = (1<<15); //LED AMARELO
  		break;
  		case PIN3:
  			HWREG(GPIO1_SETDATAOUT) = (1<<29); //LED AZUL
  		break;
		default:
		break;
	}
}

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  buzzerON
 *  Description:  Aciono o buzzer
 * =====================================================================================
 */
void buzzerON(buzzNum buzz){
	switch (buzz) {
	case BUZZ1:
        HWREG(GPIO1_SETDATAOUT) = (1<<28);
		break;
	default:
		break;
	}
}

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  buzzerOff
 *  Description:  Desligo o buzzer
 * =====================================================================================
 */
void buzzerOFF(buzzNum buzz){
	switch (buzz)
	{
	case BUZZ1:
        HWREG(GPIO1_CLEARDATAOUT) = (1<<28);
		break;
	
	default:
		break;
	}
}

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  gpioIsrHandler
 *  Description:  Limpa a interrupcao
 * =====================================================================================
 */
 void gpioIsrHandler(void){

    /* Clear the status of the interrupt flags */
	HWREG(GPIO1_IRQSTATUS_0) = 1<<14;
	//0x1000; 

	flag_gpio = true;
}
void gpioIsrHandler2(void){

	HWREG(GPIO1_IRQSTATUS_1) = 1<<12; 
	
	flag2_gpio = true;
}

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  ISR_Handler
 *  Description:  Tratador de interrupacao
 * =====================================================================================
 */
void ISR_Handler(void){
	/* Verifica se é interrupção do DMTIMER7 */
	// INTC_SIR_IRQ -> Armazena o numero da interrupção a ser chamada
	unsigned int irq_number = HWREG(INTC_SIR_IRQ) & 0x7f;
	
	if(irq_number == 95) { // interrupcao de timer
		timerIrqHandler(); 
	}

	if(irq_number == 98) { // interrupcao de gpio1 - 98
		gpioIsrHandler();
	}
    
	if(irq_number == 99) { //interrupcao de gpio - 99
		gpioIsrHandler2();
	}
	/* Reconhece a IRQ */
	//Esse registrador ele reserta a saida interrupção para
	// que uma nova interrupção possa ser tratada
	HWREG(INTC_CONTROL) = 0x1;
}

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  main
 *  Description:  Se o botao for pressionado e o sensor de inclinacao acionado, 
 * 				  o buzzer eh ligado e o led vermelho acesso.
 * =====================================================================================
 */
int main(void) {
	
	char count=2;
	
	/* Hardware setup */
	gpioSetup();
	timerSetup();
	disableWdt();
	butConfig();

	#ifdef DELAY_USE_INTERRUPT 
		putString("Timer Interrupt",17);
	#else
		putString("Timer",7);
	#endif

	while(count){
		putCh(0x30+count);
		putCh(' ');
		delay(1000);
		count--;
	}

	while(true){
	    putString("\033[H\033[J\r", 7);
		// Quando os dois tiverem ativados
		if(flag_gpio == true && flag2_gpio == true){
			putString("\rPERIGO\n",8);
			putString("\rRisco de queda!!!\n",19);
			putString("\r\n",2);
				ledOff(PIN3);	// Led azul
				ledOn(PIN1);	// Led vermelho
			delay(frequencia);
			buzzerON(BUZZ1);
            delay(frequencia);
				ledOff(PIN1);	// Led vermelho
			buzzerOFF(BUZZ1);
			flag2_gpio = false;
			flag_gpio = false;
           // sensor ativado
		} else if(flag_gpio==true && flag2_gpio==false) {
			putString("\rCUIDADO\n",9);
			putString("\rAlguem esta em perigo\n",23);
			putString("\rRisco de queda\n",16);
			putString("\r\n",2);
				ledOff(PIN3);	// Led azul
				ledOn(PIN2);	// Led amarelo
			delay(frequencia);
				ledOff(PIN2);	// Led amarelo
			flag_gpio = false;
           // botao 
		} else if(flag2_gpio==true && flag_gpio ==false){
			putString("\rCuidado\n",9);
            putString("\rAlguem esta em cima da janela\n",31);
			putString("\r\n",2);
				ledOff(PIN3);	// Led azul
				ledOn(PIN2);	// Led amarelo
			delay(frequencia);
				ledOff(PIN2);	// Led amarelo
			flag2_gpio = false;	

		} else if(flag_gpio==false && flag_gpio==false){
			putString("\rAmbiente seguro\n",17);
			putString("\r\n",2);
            	ledOn(PIN3);	// Led azul
			delay(frequencia);
		}

		delay(frequencia);
		delay(frequencia);
	}	

	return(0);
}