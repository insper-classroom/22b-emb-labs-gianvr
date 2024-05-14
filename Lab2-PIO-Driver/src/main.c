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

/*  Default pin configuration (no attribute). */
#define _PIO_DEFAULT             (0u << 0)
/*  The internal pin pull-up is active. */
#define _PIO_PULLUP              (1u << 0)
/*  The internal glitch filter is active. */
#define _PIO_DEGLITCH            (1u << 1)
/*  The internal debouncing filter is active. */
#define _PIO_DEBOUNCE            (1u << 3)


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

/**
 * \brief Set a high output level on all the PIOs defined in ul_mask.
 * This has no immediate effects on PIOs that are not output, but the PIO
 * controller will save the value if they are changed to outputs.
 *
 * \param p_pio Pointer to a PIO instance.
 * \param ul_mask Bitmask of one or more pin(s) to configure.
 */
void _pio_set(Pio *p_pio, const uint32_t ul_mask)
{
	 p_pio->PIO_SODR = ul_mask;
}

/**
 * \brief Set a low output level on all the PIOs defined in ul_mask.
 * This has no immediate effects on PIOs that are not output, but the PIO
 * controller will save the value if they are changed to outputs.
 *
 * \param p_pio Pointer to a PIO instance.
 * \param ul_mask Bitmask of one or more pin(s) to configure.
 */
void _pio_clear(Pio *p_pio, const uint32_t ul_mask)
{
	 p_pio->PIO_CODR = ul_mask;
}

/**
 * \brief Configure PIO internal pull-up.
 *
 * \param p_pio Pointer to a PIO instance.
 * \param ul_mask Bitmask of one or more pin(s) to configure.
 * \param ul_pull_up_enable Indicates if the pin(s) internal pull-up shall be
 * configured.
 */
void _pio_pull_up(Pio *p_pio, const uint32_t ul_mask,
        const uint32_t ul_pull_up_enable){
	if(ul_pull_up_enable){
	p_pio->PIO_PUER = ul_mask ;
	}else{
		p_pio->PIO_PUDR = ul_mask;
	}
 }
 
 /**
 * \brief Configure one or more pin(s) or a PIO controller as inputs.
 * Optionally, the corresponding internal pull-up(s) and glitch filter(s) can
 * be enabled.
 *
 * \param p_pio Pointer to a PIO instance.
 * \param ul_mask Bitmask indicating which pin(s) to configure as input(s).
 * \param ul_attribute PIO attribute(s).
 */
void _pio_set_input(Pio *p_pio, const uint32_t ul_mask,
        const uint32_t ul_attribute)
{
	if(_PIO_DEBOUNCE){
		p_pio->PIO_IFSCDR = ul_mask;
	}
	if(_PIO_DEGLITCH){
		p_pio->PIO_IFSCER = ul_mask;
	}
	if(_PIO_PULLUP){
		_pio_pull_up(p_pio, ul_mask, 1);
	}
}

/**
 * \brief Configure one or more pin(s) of a PIO controller as outputs, with
 * the given default value. Optionally, the multi-drive feature can be enabled
 * on the pin(s).
 *
 * \param p_pio Pointer to a PIO instance.
 * \param ul_mask Bitmask indicating which pin(s) to configure.
 * \param ul_default_level Default level on the pin(s).
 * \param ul_multidrive_enable Indicates if the pin(s) shall be configured as
 * open-drain.
 * \param ul_pull_up_enable Indicates if the pin shall have its pull-up
 * activated.
 */
void _pio_set_output(Pio *p_pio, const uint32_t ul_mask,
        const uint32_t ul_default_level,
        const uint32_t ul_multidrive_enable,
        const uint32_t ul_pull_up_enable)
{
	p_pio->PIO_PER = ul_mask;
	p_pio->PIO_OER = ul_mask;
	
	if(ul_default_level){
		_pio_set(p_pio, ul_mask);
	}else{
		_pio_clear(p_pio, ul_mask);
	}
	
	if(ul_multidrive_enable){
		p_pio->PIO_MDER = ul_mask;
	}else{
		p_pio->PIO_MDDR = ul_mask;
	}
	
	if(ul_pull_up_enable){
		_pio_pull_up(p_pio,ul_mask, 1);
	}else{
		_pio_pull_up(p_pio, ul_mask, 0);
	}
	
}

/**
 * \brief Return 1 if one or more PIOs of the given Pin instance currently have
 * a high level; otherwise returns 0. This method returns the actual value that
 * is being read on the pin. To return the supposed output value of a pin, use
 * pio_get_output_data_status() instead.
 *
 * \param p_pio Pointer to a PIO instance.
 * \param ul_type PIO type.
 * \param ul_mask Bitmask of one or more pin(s) to configure.
 *
 * \retval 1 at least one PIO currently has a high level.
 * \retval 0 all PIOs have a low level.
 */
uint32_t _pio_get(Pio *p_pio, const pio_type_t ul_type,
const uint32_t ul_mask){
	if(ul_type == PIO_INPUT){
		if(p_pio->PIO_PDSR & ul_mask){
			return 1;
		}
		return 0;
	}
	if(ul_type == PIO_OUTPUT_0){
		if(p_pio->PIO_OSR & ul_mask){
			return 1;
		}
		return 0;
	}
}

void _delay_ms(int tempo){
	for(unsigned long int i; i<(tempo/0.0019); i++){}
}



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
  _pio_set_output(LED_PIO_OLED1, LED_PIO_OLED1_IDX_MASK, 0, 0, 0);
  _pio_set_output(LED_PIO_OLED2, LED_PIO_OLED2_IDX_MASK, 0, 0, 0);
  _pio_set_output(LED_PIO_OLED3, LED_PIO_OLED3_IDX_MASK, 0, 0, 0);
  
  

  // Inicializa PIO do botao
  pmc_enable_periph_clk(BUT_PIO_OLED1_ID);
  _pio_set_input(BUT_PIO_OLED1, BUT_PIO_OLED1_IDX_MASK, _PIO_PULLUP | _PIO_DEBOUNCE );
  pmc_enable_periph_clk(BUT_PIO_OLED2_ID);
  _pio_set_input(BUT_PIO_OLED2, BUT_PIO_OLED2_IDX_MASK,  _PIO_PULLUP | _PIO_DEBOUNCE);
  pmc_enable_periph_clk(BUT_PIO_OLED3_ID);
  _pio_set_input(BUT_PIO_OLED3, BUT_PIO_OLED3_IDX_MASK,  _PIO_PULLUP | _PIO_DEBOUNCE);
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
	   if(!_pio_get(BUT_PIO_OLED1, PIO_INPUT, BUT_PIO_OLED1_IDX_MASK)){
		   for(int i = 0; i<5; i++){
			   _pio_set(LED_PIO_OLED1, LED_PIO_OLED1_IDX_MASK);
			   _delay_ms(500);
			   _pio_clear(LED_PIO_OLED1, LED_PIO_OLED1_IDX_MASK);
			   _delay_ms(500);
		   }
		   }else{
		   _pio_clear(LED_PIO_OLED1, LED_PIO_OLED1_IDX_MASK);
	   }
	  	   if(!_pio_get(BUT_PIO_OLED2, PIO_INPUT, BUT_PIO_OLED2_IDX_MASK)){
		  	   for(int i = 0; i<5; i++){
			  	   _pio_set(LED_PIO_OLED2, LED_PIO_OLED2_IDX_MASK);
			  	   _delay_ms(500);
			  	   _pio_clear(LED_PIO_OLED2, LED_PIO_OLED2_IDX_MASK);
			  	   _delay_ms(500);
		  	   }
		  	   }else{
		  	   _pio_clear(LED_PIO_OLED2, LED_PIO_OLED2_IDX_MASK);
	  	   }

	   if(!_pio_get(BUT_PIO_OLED3, PIO_INPUT, BUT_PIO_OLED3_IDX_MASK)){
		   for(int i = 0; i<5; i++){
			   _pio_set(LED_PIO_OLED3, LED_PIO_OLED3_IDX_MASK);     
			   _delay_ms(500);                        
			   _pio_clear(LED_PIO_OLED3, LED_PIO_OLED3_IDX_MASK);    
			   delay_ms(500);                     
		   }
		   }else{
		   _pio_clear(LED_PIO_OLED3, LED_PIO_OLED3_IDX_MASK);
	   }


  }
  return 0;
}
