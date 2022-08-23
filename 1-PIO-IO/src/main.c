/**
 * 5 semestre - Eng. da Computação - Insper
 * Rafael Corsi - rafael.corsi@insper.edu.br
 *
 * Projeto 0 para a placa SAME70-XPLD
 *
 * Objetivo :
 *  - Introduzir ASF e HAL
 *  - Configuracao de clock
 *  - Configuracao pino In/Out
 *
 * Material :
 *  - Kit: ATMEL SAME70-XPLD - ARM CORTEX M7
 */

/************************************************************************/
/* includes                                                             */
/************************************************************************/

#include "asf.h"

/************************************************************************/
/* defines                                                              */
/************************************************************************/

#define LED_PIO_OLED1 PIOA
#define LED_PIO_OLED1_ID ID_PIOA
#define LED_PIO_OLED1_IDX 0
#define LED_PIO_OLED1_IDX_MASK (1<<LED_PIO_OLED1_IDX)

#define LED_PIO_OLED2 PIOC
#define LED_PIO_OLED2_ID ID_PIOA
#define LED_PIO_OLED2_IDX 30
#define LED_PIO_OLED2_IDX_MASK (1<<LED_PIO_OLED2_IDX)

#define LED_PIO_OLED3 PIOB
#define LED_PIO_OLED3_ID ID_PIOA
#define LED_PIO_OLED3_IDX 2
#define LED_PIO_OLED3_IDX_MASK (1<<LED_PIO_OLED3_IDX)

#define BUT_PIO_OLED1 PIOD
#define BUT_PIO_OLED1_ID ID_PIOD
#define BUT_PIO_OLED1_IDX 28
#define BUT_PIO_OLED1_IDX_MASK (1<<BUT_PIO_OLED1_IDX)

#define BUT_PIO_OLED2 PIOC
#define BUT_PIO_OLED2_ID ID_PIOC
#define BUT_PIO_OLED2_IDX 31
#define BUT_PIO_OLED2_IDX_MASK (1<<BUT_PIO_OLED2_IDX)

#define BUT_PIO_OLED3 PIOA
#define BUT_PIO_OLED3_ID ID_PIOA
#define BUT_PIO_OLED3_IDX 19
#define BUT_PIO_OLED3_IDX_MASK (1<<BUT_PIO_OLED3_IDX)

/************************************************************************/
/* constants                                                            */
/************************************************************************/

/************************************************************************/
/* variaveis globais                                                    */
/************************************************************************/

/************************************************************************/
/* prototypes                                                           */
/************************************************************************/

void init(void);

/************************************************************************/
/* interrupcoes                                                         */
/************************************************************************/

/************************************************************************/
/* funcoes                                                              */
/************************************************************************/

// Função de inicialização do uC
void init(void){
  // Initialize the board clock
  sysclk_init();

  // Desativa WatchDog Timer
  WDT->WDT_MR = WDT_MR_WDDIS;
  
  // Ativa o PIO na qual o LED foi conectado
  // para que possamos controlar o LED.
  pmc_enable_periph_clk(LED_PIO_OLED1_ID);
  pmc_enable_periph_clk(LED_PIO_OLED2_ID);
  pmc_enable_periph_clk(LED_PIO_OLED3_ID);
  
  //Inicializa PC8 como saída
  pio_set_output(LED_PIO_OLED1, LED_PIO_OLED1_IDX_MASK, 0, 0, 0);
  pio_set_output(LED_PIO_OLED2, LED_PIO_OLED2_IDX_MASK, 0, 0, 0);
  pio_set_output(LED_PIO_OLED3, LED_PIO_OLED3_IDX_MASK, 0, 0, 0);
  
  

  // Inicializa PIO do botao
  pmc_enable_periph_clk(BUT_PIO_OLED1_ID);
  pio_set_input(BUT_PIO_OLED1, BUT_PIO_OLED1_IDX_MASK, PIO_PULLUP);
  pmc_enable_periph_clk(BUT_PIO_OLED2_ID);
  pio_set_input(BUT_PIO_OLED2, BUT_PIO_OLED2_IDX_MASK, PIO_PULLUP);
  pmc_enable_periph_clk(BUT_PIO_OLED3_ID);
  pio_set_input(BUT_PIO_OLED3, BUT_PIO_OLED3_IDX_MASK, PIO_PULLUP);
  

  
}


/************************************************************************/
/* Main                                                                 */
/************************************************************************/

// Funcao principal chamada na inicalizacao do uC.
int main(void)
{
  init();

  // super loop
  // aplicacoes embarcadas não devem sair do while(1).
  while (1)
  {
	   if(!pio_get(BUT_PIO_OLED1, PIO_INPUT, BUT_PIO_OLED1_IDX_MASK)){
		   for(int i = 0; i<5; i++){
			   pio_set(LED_PIO_OLED1, LED_PIO_OLED1_IDX_MASK);
			   delay_ms(500);
			   pio_clear(LED_PIO_OLED1, LED_PIO_OLED1_IDX_MASK);
			   delay_ms(500);
		   }
		   }else{
		   pio_clear(LED_PIO_OLED1, LED_PIO_OLED1_IDX_MASK);
	   }
	  	   if(!pio_get(BUT_PIO_OLED2, PIO_INPUT, BUT_PIO_OLED2_IDX_MASK)){
		  	   for(int i = 0; i<5; i++){
			  	   pio_set(LED_PIO_OLED2, LED_PIO_OLED2_IDX_MASK);
			  	   delay_ms(500);
			  	   pio_clear(LED_PIO_OLED2, LED_PIO_OLED2_IDX_MASK);
			  	   delay_ms(500);
		  	   }
		  	   }else{
		  	   pio_clear(LED_PIO_OLED2, LED_PIO_OLED2_IDX_MASK);
	  	   }

	   if(!pio_get(BUT_PIO_OLED3, PIO_INPUT, BUT_PIO_OLED3_IDX_MASK)){
		   for(int i = 0; i<5; i++){
			   pio_set(LED_PIO_OLED3, LED_PIO_OLED3_IDX_MASK);     
			   delay_ms(500);                        
			   pio_clear(LED_PIO_OLED3, LED_PIO_OLED3_IDX_MASK);    
			   delay_ms(500);                     
		   }
		   }else{
		   pio_clear(LED_PIO_OLED3, LED_PIO_OLED3_IDX_MASK);
	   }


  }
  return 0;
}
