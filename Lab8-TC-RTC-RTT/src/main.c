#include <asf.h>
#include "conf_board.h"

#include "gfx_mono_ug_2832hsweg04.h"
#include "gfx_mono_text.h"
#include "sysfont.h"

#include <string.h>

// Botão(OLED 1)
#define BUT_PIO_OLED1 PIOD
#define BUT_PIO_OLED1_ID ID_PIOD
#define BUT_PIO_OLED1_IDX 28
#define BUT_PIO_OLED1_IDX_MASK (1u << BUT_PIO_OLED1_IDX)

// LED placa
#define LED_PIO PIOC
#define LED_PIO_ID ID_PIOC
#define LED_IDX 8
#define LED_IDX_MASK (1 << LED_IDX)

// LED 1 (OLED)
#define LED1_PIO PIOA
#define LED1_PIO_ID ID_PIOA
#define LED1_PIO_IDX 0
#define LED1_IDX_MASK (1 << LED1_PIO_IDX)

// LED 2 (OLED)
#define LED2_PIO PIOC
#define LED2_PIO_ID ID_PIOC
#define LED2_PIO_IDX 30
#define LED2_IDX_MASK (1 << LED2_PIO_IDX)

// LED 3 (OLED)
#define LED3_PIO PIOB
#define LED3_PIO_ID ID_PIOB
#define LED3_PIO_IDX 2
#define LED3_IDX_MASK (1 << LED3_PIO_IDX)

/** RTOS  */
#define TASK_OLED_STACK_SIZE                (1024*6/sizeof(portSTACK_TYPE))
#define TASK_OLED_STACK_PRIORITY            (tskIDLE_PRIORITY)

extern void vApplicationStackOverflowHook(xTaskHandle *pxTask,  signed char *pcTaskName);
extern void vApplicationIdleHook(void);
extern void vApplicationTickHook(void);
extern void vApplicationMallocFailedHook(void);
extern void xPortSysTickHandler(void);

SemaphoreHandle_t xSemaphoreRelogio;
SemaphoreHandle_t xSemaphoreContador20;

/** prototypes */

typedef struct  {
	uint32_t year;
	uint32_t month;
	uint32_t day;
	uint32_t week;
	uint32_t hour;
	uint32_t minute;
	uint32_t second;
} calendar;

void but_callback(void);
static void io_init(void);
void TC_init(Tc * TC, int ID_TC, int TC_CHANNEL, int freq);
void pin_toggle(Pio *pio, uint32_t mask);
static void RTT_init(float freqPrescale, uint32_t IrqNPulses, uint32_t rttIRQSource);
void RTC_init(Rtc *rtc, uint32_t id_rtc, calendar t, uint32_t irq_type);
void apaga_tela();

/************************************************************************/
/* RTOS application funcs                                               */
/************************************************************************/

extern void vApplicationStackOverflowHook(xTaskHandle *pxTask, signed char *pcTaskName) {
	printf("stack overflow %x %s\r\n", pxTask, (portCHAR *)pcTaskName);
	for (;;) {	}
}

extern void vApplicationIdleHook(void) { }

extern void vApplicationTickHook(void) { }

extern void vApplicationMallocFailedHook(void) {
	configASSERT( ( volatile void * ) NULL );
}

/************************************************************************/
/* handlers / callbacks                                                 */
/************************************************************************/
void RTT_Handler(void) {
	uint32_t ul_status;
	ul_status = rtt_get_status(RTT);

	/* IRQ due to Alarm */
	if ((ul_status & RTT_SR_ALMS) == RTT_SR_ALMS) {
		pin_toggle(LED2_PIO, LED2_IDX_MASK);
		RTT_init(4, 16, RTT_MR_ALMIEN);
	}
}

void but_oled1_callback(void) {
	tc_start(TC1, 1);
}

void TC0_Handler(void) {
	/**
	* Devemos indicar ao TC que a interrupção foi satisfeita.
	* Isso é realizado pela leitura do status do periférico
	**/
	volatile uint32_t status = tc_get_status(TC0, 0);

	/** Muda o estado do LED (pisca) **/
	pin_toggle(LED_PIO, LED_IDX_MASK);  
}

void TC1_Handler(void) {
	/**
	* Devemos indicar ao TC que a interrupção foi satisfeita.
	* Isso é realizado pela leitura do status do periférico
	**/
	volatile uint32_t status = tc_get_status(TC0, 1);

	/** Muda o estado do LED (pisca) **/
	pin_toggle(LED1_PIO, LED1_IDX_MASK);  
}

void TC3_Handler(void) {
	/**
	* Devemos indicar ao TC que a interrupção foi satisfeita.
	* Isso é realizado pela leitura do status do periférico
	**/
	volatile uint32_t status = tc_get_status(TC1, 0);
	xSemaphoreGiveFromISR(xSemaphoreRelogio, 0);
}

void TC4_Handler(void) {
	/**
	* Devemos indicar ao TC que a interrupção foi satisfeita.
	* Isso é realizado pela leitura do status do periférico
	**/
	volatile uint32_t status = tc_get_status(TC1, 1);
	xSemaphoreGiveFromISR(xSemaphoreContador20, 0);
}
void RTC_Handler(void) {
	uint32_t ul_status = rtc_get_status(RTC);
	
	/* seccond tick */
	if ((ul_status & RTC_SR_SEC) == RTC_SR_SEC) {
		// o código para irq de segundo vem aqui
	}
	
	/* Time or date alarm */
	if ((ul_status & RTC_SR_ALARM) == RTC_SR_ALARM) {
		// o código para irq de alame vem aqui
	}

	rtc_clear_status(RTC, RTC_SCCR_SECCLR);
	rtc_clear_status(RTC, RTC_SCCR_ALRCLR);
	rtc_clear_status(RTC, RTC_SCCR_ACKCLR);
	rtc_clear_status(RTC, RTC_SCCR_TIMCLR);
	rtc_clear_status(RTC, RTC_SCCR_CALCLR);
	rtc_clear_status(RTC, RTC_SCCR_TDERRCLR);
}

/************************************************************************/
/* TASKS                                                                */
/************************************************************************/

static void task_oled(void *pvParameters) {
	io_init();
	gfx_mono_ssd1306_init();
	
	RTT_init(4, 16, RTT_MR_ALMIEN);
	
	TC_init(TC0, ID_TC0, 0, 5);
	tc_start(TC0, 0);
	
	TC_init(TC0, ID_TC1, 1, 4);
	tc_start(TC0, 1);
	
	TC_init(TC1, ID_TC3, 0, 1);
	tc_start(TC1, 0);
	
	TC_init(TC1, ID_TC4, 1, 1);

	calendar rtc_initial = {2018, 3, 19, 12, 15, 45 ,1};
	RTC_init(RTC, ID_RTC, rtc_initial, RTC_IER_ALREN);
	uint32_t current_hour, current_min, current_sec;
	
	char time_pass = 0;
	char str_hour[10];
	char str_min[10];
	char str_sec[10];
	char time_now[30];
	for (;;)  {
		 if( xSemaphoreTake(xSemaphoreRelogio, 0) == pdTRUE ){
			 apaga_tela();
		     rtc_get_time(RTC, &current_hour, &current_min, &current_sec);
			 sprintf(str_hour, "%d", current_hour);
			 sprintf(str_min, "%d", current_min);
			 sprintf(str_sec, "%d", current_sec);
			   
			 strcat(strcpy(time_now, str_hour), ":");
			 if(current_min<10){
				strcat(time_now, "0");
			 }
			 strcat(time_now, str_min);
			 strcat(time_now, ":");
			 if(current_sec<10){
				strcat(time_now, "0");
			  }
			 strcat(time_now, str_sec);
			 gfx_mono_draw_string(time_now, 40, 15, &sysfont);
		 }
		 if(xSemaphoreTake(xSemaphoreContador20, 0) == pdTRUE){
			  time_pass++;
			  if(time_pass==20){
				  tc_stop(TC1, 1);
				  time_pass = 0;
				  pin_toggle(LED3_PIO,LED3_IDX_MASK);
			  }
		  }
		pmc_sleep(SAM_PM_SMODE_SLEEP_WFI);
	}
}

/************************************************************************/
/* funcoes                                                              */
/************************************************************************/
void apaga_tela() {
	gfx_mono_draw_filled_rect(0, 0, 128, 32, GFX_PIXEL_CLR);
}

void pin_toggle(Pio *pio, uint32_t mask) {
	if(pio_get_output_data_status(pio, mask))
	pio_clear(pio, mask);
	else
	pio_set(pio,mask);
}

void RTC_init(Rtc *rtc, uint32_t id_rtc, calendar t, uint32_t irq_type) {
	/* Configura o PMC */
	pmc_enable_periph_clk(ID_RTC);

	/* Default RTC configuration, 24-hour mode */
	rtc_set_hour_mode(rtc, 0);

	/* Configura data e hora manualmente */
	rtc_set_date(rtc, t.year, t.month, t.day, t.week);
	rtc_set_time(rtc, t.hour, t.minute, t.second);

	/* Configure RTC interrupts */
	NVIC_DisableIRQ(id_rtc);
	NVIC_ClearPendingIRQ(id_rtc);
	NVIC_SetPriority(id_rtc, 4);
	NVIC_EnableIRQ(id_rtc);

	/* Ativa interrupcao via alarme */
	rtc_enable_interrupt(rtc,  irq_type);
}

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

void TC_init(Tc * TC, int ID_TC, int TC_CHANNEL, int freq){
	uint32_t ul_div;
	uint32_t ul_tcclks;
	uint32_t ul_sysclk = sysclk_get_cpu_hz();

	/* Configura o PMC */
	pmc_enable_periph_clk(ID_TC);

	/** Configura o TC para operar em  freq hz e interrupçcão no RC compare */
	tc_find_mck_divisor(freq, ul_sysclk, &ul_div, &ul_tcclks, ul_sysclk);
	tc_init(TC, TC_CHANNEL, ul_tcclks | TC_CMR_CPCTRG);
	tc_write_rc(TC, TC_CHANNEL, (ul_sysclk / ul_div) / freq);

	/* Configura NVIC*/
	NVIC_SetPriority(ID_TC, 4);
	NVIC_EnableIRQ((IRQn_Type) ID_TC);
	tc_enable_interrupt(TC, TC_CHANNEL, TC_IER_CPCS);
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

static void io_init(void) {
    // Leds
    pmc_enable_periph_clk(LED_PIO_ID);
    pio_configure(LED_PIO, PIO_OUTPUT_1, LED_IDX_MASK, PIO_DEFAULT);
    pmc_enable_periph_clk(LED1_PIO_ID);
    pio_configure(LED1_PIO, PIO_OUTPUT_1, LED1_IDX_MASK, PIO_DEFAULT);
    pmc_enable_periph_clk(LED2_PIO_ID);
    pio_configure(LED2_PIO, PIO_OUTPUT_1, LED2_IDX_MASK, PIO_DEFAULT);
    pmc_enable_periph_clk(LED3_PIO_ID);
    pio_configure(LED3_PIO, PIO_OUTPUT_1, LED3_IDX_MASK, PIO_DEFAULT);
	
	/* configura prioridade */
	NVIC_EnableIRQ(BUT_PIO_OLED1_ID);
	NVIC_SetPriority(BUT_PIO_OLED1_ID, 4);

	/* conf botão como entrada */
	pio_configure(BUT_PIO_OLED1, PIO_INPUT, BUT_PIO_OLED1_IDX_MASK, PIO_PULLUP | PIO_DEBOUNCE);
	pio_set_debounce_filter(BUT_PIO_OLED1, BUT_PIO_OLED1_IDX_MASK, 60);
	pio_enable_interrupt(BUT_PIO_OLED1, BUT_PIO_OLED1_IDX_MASK);
	pio_handler_set(BUT_PIO_OLED1, BUT_PIO_OLED1_ID, BUT_PIO_OLED1_IDX_MASK, PIO_IT_FALL_EDGE , but_oled1_callback);
}

/************************************************************************/
/* main                                                                 */
/************************************************************************/


int main(void) {
	/* Initialize the SAM system */
	sysclk_init();
	board_init();

	/* Initialize the console uart */
	configure_console();

	/* Create task to control oled */
	if (xTaskCreate(task_oled, "oled", TASK_OLED_STACK_SIZE, NULL, TASK_OLED_STACK_PRIORITY, NULL) != pdPASS) {
	  printf("Failed to create oled task\r\n");
	}
    // cria semáforo binário
    xSemaphoreRelogio = xSemaphoreCreateBinary();
	xSemaphoreContador20 = xSemaphoreCreateBinary();

    // verifica se semáforo foi criado corretamente
    if (xSemaphoreRelogio == NULL)
    printf("falha em criar o semaforo \n");
	
	if (xSemaphoreContador20 == NULL)
	printf("falha em criar o semaforo \n");
	/* Start the scheduler. */
	vTaskStartScheduler();

  /* RTOS não deve chegar aqui !! */
	while(1){}

	/* Will only get here if there was insufficient memory to create the idle task. */
	return 0;
}
