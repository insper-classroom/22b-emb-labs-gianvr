#include "conf_board.h"
#include <asf.h>

#include "gfx_mono_ug_2832hsweg04.h"

#include "gfx_mono_text.h"
#include "sysfont.h"

/************************************************************************/
/* BOARD CONFIG                                                         */
/************************************************************************/

#define TRIGGER_PIO PIOA
#define TRIGGER_PIO_ID ID_PIOA
#define TRIGGER_PIO_IDX 27
#define TRIGGER_PIO_IDX_MASK (1 << TRIGGER_PIO_IDX)

#define ECHO_PIO PIOD
#define ECHO_PIO_ID ID_PIOD
#define ECHO_PIO_PIN 31
#define ECHO_PIO_PIN_MASK (1 << ECHO_PIO_PIN)

#define BUT_PIO_OLED1 PIOD
#define BUT_PIO_OLED1_ID ID_PIOD
#define BUT_PIO_OLED1_IDX 28
#define BUT_PIO_OLED1_IDX_MASK (1 << BUT_PIO_OLED1_IDX)


#define USART_COM_ID ID_USART1
#define USART_COM USART1



/************************************************************************/
/* RTOS                                                                */
/************************************************************************/

#define TASK_TRIGGER_STACK_SIZE (4096 / sizeof(portSTACK_TYPE))
#define TASK_TRIGGER_STACK_PRIORITY (tskIDLE_PRIORITY)

#define TASK_ECHO_STACK_SIZE (4096 / sizeof(portSTACK_TYPE))
#define TASK_ECHO_STACK_PRIORITY (tskIDLE_PRIORITY)

extern void vApplicationStackOverflowHook(xTaskHandle *pxTask,
                                          signed char *pcTaskName);
extern void vApplicationIdleHook(void);
extern void vApplicationTickHook(void);
extern void vApplicationMallocFailedHook(void);
extern void xPortSysTickHandler(void);

/************************************************************************/
/* recursos RTOS                                                        */
/************************************************************************/

/** Semaforo a ser usado pela task echo */
SemaphoreHandle_t xSemaphoreEcho;

/** Queue for msg log send data */
QueueHandle_t xQueueLedFreq;

/************************************************************************/
/* prototypes local                                                     */
/************************************************************************/
void echo_callback();
static void USART1_init(void);
static void RTT_init(float freqPrescale, uint32_t IrqNPulses, uint32_t rttIRQSource);
void trigger_init();
static void ECHO_init(void);

/************************************************************************/
/* RTOS application funcs                                               */
/************************************************************************/

/**
 * \brief Called if stack overflow during execution
 */
extern void vApplicationStackOverflowHook(xTaskHandle *pxTask,
                                          signed char *pcTaskName) {
  printf("stack overflow %x %s\r\n", pxTask, (portCHAR *)pcTaskName);
  /* If the parameters have been corrupted then inspect pxCurrentTCB to
   * identify which task has overflowed its stack.
   */
  for (;;) {
  }
}

/**
 * \brief This function is called by FreeRTOS idle task
 */
extern void vApplicationIdleHook(void) { pmc_sleep(SAM_PM_SMODE_SLEEP_WFI); }

/**
 * \brief This function is called by FreeRTOS each tick
 */
extern void vApplicationTickHook(void) {}

extern void vApplicationMallocFailedHook(void) {
  /* Called if a call to pvPortMalloc() fails because there is insufficient
  free memory available in the FreeRTOS heap.  pvPortMalloc() is called
  internally by FreeRTOS API functions that create tasks, queues, software
  timers, and semaphores.  The size of the FreeRTOS heap is set by the
  configTOTAL_HEAP_SIZE configuration constant in FreeRTOSConfig.h. */

  /* Force an assert. */
  configASSERT((volatile void *)NULL);
}

/************************************************************************/
/* handlers / callbacks                                                 */
/************************************************************************/

void echo_callback() {
	  BaseType_t xHigherPriorityTaskWoken = pdFALSE;
  xSemaphoreGiveFromISR(xSemaphoreEcho, &xHigherPriorityTaskWoken);
}

/************************************************************************/
/* TASKS                                                                */
/************************************************************************/
static void task_echo(void *pvParameters) {

  /* inicializa botao */
  ECHO_init();
  
  for (;;) {
    /* aguarda por tempo inderteminado até a liberacao do semaforo */
    if (xSemaphoreTake(xSemaphoreEcho, 100)) {
    
	if(pio_get(ECHO_PIO, PIO_INPUT, ECHO_PIO_PIN_MASK)){
		RTT_init(10000, 0, 0);
	}else{
		 uint32_t time_read = rtt_read_timer_value(RTT);
		int dist= 1.0/10000*time_read*170*100;
		gfx_mono_draw_filled_rect(0, 0, 128, 32, GFX_PIXEL_CLR);
		if(dist>400){
			gfx_mono_draw_string("Espaco aberto", 35, 8, &sysfont);
		}else{
		char str[128];
		sprintf(str, "%d", dist);
		gfx_mono_draw_string("Distancia", 35, 0, &sysfont);
		gfx_mono_draw_string(str, 35, 16, &sysfont);
		gfx_mono_draw_string("cm", 60, 16, &sysfont);
		}
		printf("TASK_ECHO: %d \n", dist);
	}

    }else{
		gfx_mono_draw_filled_rect(0, 0, 128, 32, GFX_PIXEL_CLR);
		gfx_mono_draw_string("Mau contato", 35, 8, &sysfont);
	}
  }
}

static void task_trigger(void *pvParameters) {
	trigger_init();
	for (;;) {
		if(!pio_get(ECHO_PIO, PIO_INPUT, ECHO_PIO_PIN_MASK)){
		vTaskDelay(60);
		pio_set(TRIGGER_PIO, TRIGGER_PIO_IDX_MASK);
		delay_us(10);
		pio_clear(TRIGGER_PIO, TRIGGER_PIO_IDX_MASK);
		}
	}
}

/************************************************************************/
/* funcoes                                                              */
/************************************************************************/

/**
 * \brief Configure the console UART.
 */

static void RTT_init(float freqPrescale, uint32_t IrqNPulses, uint32_t rttIRQSource) {

	uint16_t pllPreScale = (int) (((float) 32768) / freqPrescale);
	
	rtt_sel_source(RTT, false);
	rtt_init(RTT, pllPreScale);
	
	if (rttIRQSource & RTT_MR_ALMIEN) {
		uint32_t ul_previous_time;
		ul_previous_time = rtt_read_timer_value(RTT);
		while (ul_previous_time == rtt_read_timer_value(RTT));
		rtt_write_alarm_time(RTT, IrqNPulses+ul_previous_time);
	}

	/* config NVIC */
	NVIC_DisableIRQ(RTT_IRQn);
	NVIC_ClearPendingIRQ(RTT_IRQn);
	NVIC_SetPriority(RTT_IRQn, 4);
	NVIC_EnableIRQ(RTT_IRQn);

	/* Enable RTT interrupt */
	if (rttIRQSource & (RTT_MR_RTTINCIEN | RTT_MR_ALMIEN))
	rtt_enable_interrupt(RTT, rttIRQSource);
	else
	rtt_disable_interrupt(RTT, RTT_MR_RTTINCIEN | RTT_MR_ALMIEN);
	
}

static void configure_console(void) {
  const usart_serial_options_t uart_serial_options = {
      .baudrate = CONF_UART_BAUDRATE,
      .charlength = CONF_UART_CHAR_LENGTH,
      .paritytype = CONF_UART_PARITY,
      .stopbits = CONF_UART_STOP_BITS,
  };

  /* Configure console UART. */
  stdio_serial_init(CONF_UART, &uart_serial_options);

  /* Specify that stdout should not be buffered. */
  setbuf(stdout, NULL);
}

void trigger_init(){
	pmc_enable_periph_clk(TRIGGER_PIO_ID);
	pio_set_output(TRIGGER_PIO, TRIGGER_PIO_IDX_MASK, 1, 0, 0);
};


static void ECHO_init(void) {
	
  // Configura PIO para lidar com o pino do botão como entrada
  // com pull-up
  pio_configure(ECHO_PIO, PIO_INPUT, ECHO_PIO_PIN_MASK, 0);

  // Configura interrupção no pino referente ao botao e associa
  // função de callback caso uma interrupção for gerada
  // a função de callback é a: but_callback()
  pio_handler_set(ECHO_PIO,
                  ECHO_PIO_ID,
                  ECHO_PIO_PIN_MASK,
                  PIO_IT_EDGE,
                  echo_callback);

  // Ativa interrupção e limpa primeira IRQ gerada na ativacao
  pio_enable_interrupt(ECHO_PIO, ECHO_PIO_PIN_MASK);
  pio_get_interrupt_status(ECHO_PIO);
  
  // Configura NVIC para receber interrupcoes do PIO do botao
  // com prioridade 4 (quanto mais próximo de 0 maior)
  NVIC_EnableIRQ(ECHO_PIO_ID);
  NVIC_SetPriority(ECHO_PIO_ID, 4); // Prioridade 4
}

/************************************************************************/
/* main                                                                 */
/************************************************************************/

/**
 *  \brief FreeRTOS Real Time Kernel example entry point.
 *
 *  \return Unused (ANSI-C compatibility).
 */
int main(void) {
  /* Initialize the SAM system */
  sysclk_init();
  gfx_mono_ssd1306_init();
  board_init();
  configure_console();

  printf("Sys init ok \n");
  /* Attempt to create a semaphore. */
  xSemaphoreEcho = xSemaphoreCreateBinary();
  if (xSemaphoreEcho == NULL)
    printf("falha em criar o semaforo \n");

  /* cria queue com 32 "espacos" */
  /* cada espaço possui o tamanho de um inteiro*/
  xQueueLedFreq = xQueueCreate(32, sizeof(uint32_t));
  if (xQueueLedFreq == NULL)
    printf("falha em criar a queue \n");

  /* Create task to monitor processor activity */
  if (xTaskCreate(task_echo, "ECHO", TASK_ECHO_STACK_SIZE, NULL,
                  TASK_ECHO_STACK_PRIORITY, NULL) != pdPASS) {
    printf("Failed to create UartTx task\r\n");
  } else {
     printf("task led but \r\n");  
  }
  
  if (xTaskCreate(task_trigger, "TRIGGER", TASK_TRIGGER_STACK_SIZE, NULL,
    TASK_TRIGGER_STACK_PRIORITY, NULL) != pdPASS) {
	    printf("Failed to create UartTx task\r\n");
    }

  /* Start the scheduler. */
  vTaskStartScheduler();

  /* RTOS não deve chegar aqui !! */
  while (1) {
  }

  /* Will only get here if there was insufficient memory to create the idle
   * task. */
  return 0;
}
