#include "timer.h"

#define DELAY_USE_INTERRUPT			1
/**
 * \brief   This macro will check for write POSTED status
 *
 * \param   reg          Register whose status has to be checked
 *
 *    'reg' can take the following values \n
 *    DMTIMER_WRITE_POST_TCLR - Timer Control register \n
	  -> Através do TCLR (timer control register) podemos iniciar e parar o time a qualquer momento.
 *    DMTIMER_WRITE_POST_TCRR - Timer Counter register \n
 	  -> Registro do Contador de temporizador (TCRR) pode ser carregado quando parado ou em tempo real. ELE também pode ser carregado
com o valor mantido no timer load register (tldr) por um tigger register(ttgr)
 *    DMTIMER_WRITE_POST_TLDR - Timer Load register \n
 *    DMTIMER_WRITE_POST_TTGR - Timer Trigger register \n
		-> Registro de disparo do temporizador
 *    DMTIMER_WRITE_POST_TMAR - Timer Match register \n
 *
 **/
#define DMTimerWaitForWrite(reg)   \
            if(HWREG(DMTIMER_TSICR) & 0x4)\
            while((reg & HWREG(DMTIMER_TWPS)));
//o software deve ler os bits de status de gravação pendentes para garantir que o acesso de gravação seguinte não seja descartado devido à sincronização de gravação em andamento
//Esses bits são automaticamente apagados pela lógica interna quando a escrita no registrador correspondente é reconhecido.

int flag_timer = false;

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  timerIrqHandler
 *  Description:  Trata a interrupcao gerada pelo timer
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
 *         Name:  timerSetup
 *  Description:  Inicializa o modulo do timer, e caso necessario a interrupcao
 * =====================================================================================
 */
void timerSetup(void){
     /*  Clock enable for DMTIMER7 TRM 8.1.12.1.25 */
    HWREG(CM_PER_TIMER7_CLKCTRL) |= 0x2;

	/*  Check clock enable for DMTIMER7 TRM 8.1.12.1.25 */  
	//Este registro gerencia os relógios do TIMER7/  
	//Enquanto o módulo não estiver habilitado, tentar habilitar./
    while((HWREG(CM_PER_TIMER7_CLKCTRL) & 0x3) != 0x2);

#ifdef DELAY_USE_INTERRUPT
    /* Interrupt mask */
    /* Modulo de interrupção */
    HWREG(INTC_MIR_CLEAR2) |= (1<<31);//(95 --> Bit 31 do 3º registrador (MIR CLEAR2))
#endif
}

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  timerEnable
 *  Description:  Inicia o contador do timer
 * =====================================================================================
 */
void timerEnable(){
    /* Espera a escrita anterior terminar no TCLR */
    /* Quando igual a 1, uma escrita está pendente no registrador TCLR */
	DMTimerWaitForWrite(0x1);

    /* Start the timer */
    HWREG(DMTIMER_TCLR) |= 0x1;
}/* -----  end of function timerEnable  ----- */

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  timerDisable
 *  Description:  Desabilita o contador do timer
 * =====================================================================================
 */
void timerDisable(){
    /* Wait for previous write to complete in TCLR */
	DMTimerWaitForWrite(0x1);

    /* Stop the timer */
    HWREG(DMTIMER_TCLR) &= ~(0x1);
}/* -----  end of function timerEnable  ----- */

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  delay
 *  Description:  Tempo decorrido
 * =====================================================================================
 */
void delay(unsigned int mSec){
	#ifdef DELAY_USE_INTERRUPT
		unsigned int countVal = TIMER_OVERFLOW - (mSec * TIMER_1MS_COUNT);
		///TIMER_OVERFLOW = 0xFFFFFFFFu

		/* Wait for previous write to complete */
		//Quando igual a 1, uma escrita está pendente no registrador TCRR/
		DMTimerWaitForWrite(0x2);

		/*Carregar o registro com o valor de recarregamento */
		HWREG(DMTIMER_TCRR) = countVal;
		
		flag_timer = false;

		/* Habilita as interrupções do DMTimer */
		//Read 1 = Interrupt enabled./
		
		HWREG(DMTIMER_IRQENABLE_SET) = 0x2; 

		/* Start the DMTimer */
		timerEnable();

		while(flag_timer == false);

		/* Disable the DMTimer interrupts */
		HWREG(DMTIMER_IRQENABLE_CLR) = 0x2; 
	#else
		while(mSec != 0){
			/* Wait for previous write to complete */
			/Quando igual a 1, uma escrita está pendente no registrador TCRR/
			DMTimerWaitForWrite(0x2);

		/* Configura o valor do contador. */
		/Valor do contador TIMER/
			HWREG(DMTIMER_TCRR) = 0x0;

			timerEnable();

			while(HWREG(DMTIMER_TCRR) < TIMER_1MS_COUNT);

			/* Stop the timer */
			HWREG(DMTIMER_TCLR) &= ~(0x00000001u);

			mSec--;
		}
	#endif
}


