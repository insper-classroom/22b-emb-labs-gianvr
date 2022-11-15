/************************************************************************/
/* includes                                                             */
/************************************************************************/

#include <asf.h>
#include <string.h>
#include "ili9341.h"
#include "lvgl.h"
#include "touch/touch.h"

#define AFEC AFEC0
#define AFEC_ID ID_AFEC0
#define AFEC_CHANNEL 5 // Canal do pino PB2

/************************************************************************/
/* LCD / LVGL                                                           */
/************************************************************************/

LV_FONT_DECLARE(dseg70);
LV_FONT_DECLARE(dseg48);
LV_FONT_DECLARE(dseg24);
LV_FONT_DECLARE(dseg30);
LV_FONT_DECLARE(dseg36);
LV_FONT_DECLARE(dseg12);
LV_FONT_DECLARE(clock);

#define LV_HOR_RES_MAX          (320)
#define LV_VER_RES_MAX          (240)
#define CLOCK_SYMBOL "\xEF\x80\x97"

static  lv_obj_t * labelBtn1;
lv_obj_t * labelFloor;
lv_obj_t * labelDec;
lv_obj_t * labelClock;
lv_obj_t * labelSetValue;
lv_obj_t * btnpower;
lv_obj_t * labelBtnPower;
lv_obj_t * btnmenu;
lv_obj_t * labelMenu;
lv_obj_t * btnsettings;
lv_obj_t * labelSettings;
lv_obj_t * btnup;
lv_obj_t * labelUp;
lv_obj_t * btndown;
lv_obj_t * labelDown;

volatile char modifica = 0;

SemaphoreHandle_t xSemaphoreClock;
SemaphoreHandle_t xSemaphoreDesliga;

static void configure_console(void) ;
void lv_termostato(void);

typedef struct  {
	uint32_t year;
	uint32_t month;
	uint32_t day;
	uint32_t week;
	uint32_t hour;
	uint32_t minute;
	uint32_t second;
} calendar;

/*A static or global variable to store the buffers*/
static lv_disp_draw_buf_t disp_buf;

/*Static or global buffer(s). The second buffer is optional*/
static lv_color_t buf_1[LV_HOR_RES_MAX * LV_VER_RES_MAX];
static lv_disp_drv_t disp_drv;          /*A variable to hold the drivers. Must be static or global.*/
static lv_indev_drv_t indev_drv;

/************************************************************************/
/* RTOS                                                                 */
/************************************************************************/
TimerHandle_t xTimer;

#define TASK_LCD_STACK_SIZE                (1024*6/sizeof(portSTACK_TYPE))
#define TASK_LCD_STACK_PRIORITY            (tskIDLE_PRIORITY)

#define TASK_RTC_STACK_SIZE                (1024*6/sizeof(portSTACK_TYPE))
#define TASK_RTC_STACK_PRIORITY            (tskIDLE_PRIORITY)

void RTC_init(Rtc *rtc, uint32_t id_rtc, calendar t, uint32_t irq_type);
static void config_AFEC(Afec *afec, uint32_t afec_id, uint32_t afec_channel,
afec_callback_t callback);

extern void vApplicationStackOverflowHook(xTaskHandle *pxTask,  signed char *pcTaskName);
extern void vApplicationIdleHook(void);
extern void vApplicationTickHook(void);
extern void vApplicationMallocFailedHook(void);
extern void xPortSysTickHandler(void);

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
/* Handlers                                                             */
/************************************************************************/
void RTC_Handler(void) {
	uint32_t ul_status = rtc_get_status(RTC);
	
	/* seccond tick */
	if ((ul_status & RTC_SR_SEC) == RTC_SR_SEC) {
		  xSemaphoreGiveFromISR(xSemaphoreClock, 0);
	}
	
	rtc_clear_status(RTC, RTC_SCCR_SECCLR);
	rtc_clear_status(RTC, RTC_SCCR_ALRCLR);
	rtc_clear_status(RTC, RTC_SCCR_ACKCLR);
	rtc_clear_status(RTC, RTC_SCCR_TIMCLR);
	rtc_clear_status(RTC, RTC_SCCR_CALCLR);
	rtc_clear_status(RTC, RTC_SCCR_TDERRCLR);
}

/************************************************************************/
/* lvgl                                                                 */
/************************************************************************/
static void AFEC_callback(void) {
	uint32_t value_read = afec_channel_get_value(AFEC, AFEC_CHANNEL);
	float temp = (50.0/4096)*value_read;
	uint32_t parte_inteira = temp;
	uint32_t parte_decimal = (temp-parte_inteira)*10;
	lv_label_set_text_fmt(labelFloor, "%02d", parte_inteira);
	lv_label_set_text_fmt(labelDec, ". %01d", parte_decimal);
	
}

static void event_handler(lv_event_t * e) {
	lv_event_code_t code = lv_event_get_code(e);

	if(code == LV_EVENT_CLICKED) {
		LV_LOG_USER("Clicked");
	}
	else if(code == LV_EVENT_VALUE_CHANGED) {
		LV_LOG_USER("Toggled");
	}
}

static void handler_power(lv_event_t * e) {
	lv_event_code_t code = lv_event_get_code(e);

	if(code == LV_EVENT_CLICKED) {
		  xSemaphoreGiveFromISR(xSemaphoreDesliga, 0);
	}

}

static void handler_menu(lv_event_t * e) {
	lv_event_code_t code = lv_event_get_code(e);

	if(code == LV_EVENT_CLICKED) {
		LV_LOG_USER("Clicked");
	}
	else if(code == LV_EVENT_VALUE_CHANGED) {
		LV_LOG_USER("Toggled");
	}
}

static void handler_config(lv_event_t * e) {
	lv_event_code_t code = lv_event_get_code(e);

	if(code == LV_EVENT_CLICKED) {
		if(modifica == 2){
			modifica = 0;
		}else{
			modifica++;
		}
	}
}

static void handler_up(lv_event_t * e) {
    lv_event_code_t code = lv_event_get_code(e);
    char *c;
    int temp;
	uint32_t current_hour, current_min, current_sec;
    if(code == LV_EVENT_CLICKED) {
		if(modifica == 0){
			c = lv_label_get_text(labelSetValue);
			temp = atoi(c);
			lv_label_set_text_fmt(labelSetValue, "%02d", temp + 1);			
		}else if(modifica == 1){
			rtc_get_time(RTC, &current_hour, &current_min, &current_sec);
			if(current_min == 59){
				if(current_hour == 23){
					rtc_set_time(RTC, 0, 0, current_sec);
				}else{
					rtc_set_time(RTC, current_hour+1, 0, current_sec);
				}
			}else{
				rtc_set_time(RTC, current_hour, current_min+1, current_sec);
			}
		}else if(modifica == 2){
			rtc_get_time(RTC, &current_hour, &current_min, &current_sec);
			if(current_hour == 23){
				rtc_set_time(RTC, 0, current_min, current_sec);
			}else{
				rtc_set_time(RTC, current_hour+1, current_min, current_sec);
			}
		}
    }
}

static void handler_down(lv_event_t * e) {
	lv_event_code_t code = lv_event_get_code(e);
	char *c;
	int temp;
	uint32_t current_hour, current_min, current_sec;
	if(code == LV_EVENT_CLICKED) {
		if(modifica == 0){
			c = lv_label_get_text(labelSetValue);
			temp = atoi(c);
			lv_label_set_text_fmt(labelSetValue, "%02d", temp - 1);
			}else if(modifica == 1){
				rtc_get_time(RTC, &current_hour, &current_min, &current_sec);
				if(current_min == 0){
					if(current_hour == 0){
						rtc_set_time(RTC, 23, 59, current_sec);
						}else{
						rtc_set_time(RTC, current_hour-1, 59, current_sec);
					}
					}else{
					rtc_set_time(RTC, current_hour, current_min-1, current_sec);
				}
			}else if(modifica == 2){
				rtc_get_time(RTC, &current_hour, &current_min, &current_sec);
				if(current_hour == 0){
					rtc_set_time(RTC, 23, current_min, current_sec);
					}else{
					rtc_set_time(RTC, current_hour-1, current_min, current_sec);
				}
		}
	}
}

void lv_termostato(void) {
    static lv_style_t style;
    lv_style_init(&style);
    lv_style_set_bg_color(&style, lv_color_black());
	lv_style_set_text_color(&style, lv_color_white());
    lv_style_set_border_width(&style, 0);
	lv_style_set_text_font(&style, &lv_font_montserrat_24);
	
	btnpower = lv_btn_create(lv_scr_act());
	lv_obj_add_event_cb(btnpower, handler_power, LV_EVENT_ALL, NULL);
	lv_obj_align(btnpower, LV_ALIGN_BOTTOM_LEFT, 10, -10);
	lv_obj_add_style(btnpower, &style, 0);
	
	labelBtnPower = lv_label_create(btnpower);
	lv_label_set_text(labelBtnPower, "[  " LV_SYMBOL_POWER);
	lv_obj_center(labelBtnPower);
	
	btnmenu = lv_btn_create(lv_scr_act());
	lv_obj_add_event_cb(btnmenu, handler_menu, LV_EVENT_ALL, NULL);
	lv_obj_align_to(btnmenu, btnpower ,LV_ALIGN_OUT_RIGHT_TOP, 0, 0);
	lv_obj_add_style(btnmenu, &style, 0);
	
    labelMenu = lv_label_create(btnmenu);
	lv_label_set_text(labelMenu, "|  M  |");
	lv_obj_center(labelMenu);	
	
	btnsettings = lv_btn_create(lv_scr_act());
	lv_obj_add_event_cb(btnsettings, handler_config, LV_EVENT_ALL, NULL);
	lv_obj_align_to(btnsettings, btnmenu ,LV_ALIGN_OUT_RIGHT_MID, 0, -3);
	lv_obj_add_style(btnsettings, &style, 0);
	
	labelSettings = lv_label_create(btnsettings);
	lv_obj_set_style_text_font(labelSettings, &clock, LV_STATE_DEFAULT);
	lv_label_set_text(labelSettings, CLOCK_SYMBOL);
	lv_obj_center(labelSettings);
	
    lv_obj_t * labelBarra = lv_label_create(lv_scr_act());
    lv_obj_align_to(labelBarra, btnsettings ,LV_ALIGN_OUT_RIGHT_TOP, 0, 0);
	lv_obj_add_style(labelBarra, &style, 0);
    lv_label_set_text(labelBarra, " ]");
	
	btnup = lv_btn_create(lv_scr_act());
	lv_obj_add_event_cb(btnup, handler_up, LV_EVENT_ALL, NULL);
	lv_obj_align_to(btnup, labelBarra ,LV_ALIGN_OUT_RIGHT_TOP, 5, -3);
	lv_obj_add_style(btnup, &style, 0);
	
	labelUp = lv_label_create(btnup);
	lv_label_set_text(labelUp, "[  " LV_SYMBOL_UP);
	lv_obj_center(labelUp);
	
	btndown = lv_btn_create(lv_scr_act());
	lv_obj_add_event_cb(btndown, handler_down, LV_EVENT_ALL, NULL);
	lv_obj_align_to(btndown, btnup ,LV_ALIGN_OUT_RIGHT_TOP, 5, 0);
	lv_obj_add_style(btndown, &style, 0);
	
	labelDown = lv_label_create(btndown);
	lv_label_set_text(labelDown,  LV_SYMBOL_DOWN "  ]");
	lv_obj_center(labelDown);
	
	lv_obj_t * dia = lv_label_create(lv_scr_act());
	lv_obj_align(dia, LV_ALIGN_TOP_LEFT, 40 , 20);
	lv_obj_set_style_text_color(dia, lv_color_white(), LV_STATE_DEFAULT);
	lv_label_set_text(dia, "Mon");

	lv_obj_t * floorText = lv_label_create(lv_scr_act());
	lv_obj_align(floorText, LV_ALIGN_LEFT_MID, 10 , 0);
	lv_obj_set_style_text_color(floorText, lv_color_white(), LV_STATE_DEFAULT);
	lv_label_set_text(floorText, "Floor\nTemp");

    labelFloor = lv_label_create(lv_scr_act());
    lv_obj_align(labelFloor, LV_ALIGN_LEFT_MID, 55 , -20);
    lv_obj_set_style_text_font(labelFloor, &dseg70, LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(labelFloor, lv_color_white(), LV_STATE_DEFAULT);
    lv_label_set_text_fmt(labelFloor, "%02d", 23);
	
	labelDec = lv_label_create(lv_scr_act());
	lv_obj_align_to(labelDec,labelFloor ,LV_ALIGN_OUT_RIGHT_BOTTOM, 0 , -15);
	lv_obj_set_style_text_font(labelDec, &dseg30, LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(labelDec, lv_color_white(), LV_STATE_DEFAULT);
	lv_label_set_text_fmt(labelDec, ". %01d", 4);
	
	lv_obj_t * bolota1 = lv_label_create(lv_scr_act());
	lv_obj_align_to(bolota1, labelDec,LV_ALIGN_OUT_TOP_LEFT, 10 , -20);
	lv_obj_set_style_text_color(bolota1, lv_color_white(), LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(bolota1, &lv_font_montserrat_8, 0);
	lv_label_set_text(bolota1, "o");
	
	lv_obj_t * celsius1 = lv_label_create(lv_scr_act());
	lv_obj_align_to(celsius1, bolota1,LV_ALIGN_OUT_BOTTOM_LEFT, 6 , -6);
	lv_obj_set_style_text_color(celsius1, lv_color_white(), LV_STATE_DEFAULT);
	lv_label_set_text(celsius1, "C");
	
    labelClock = lv_label_create(lv_scr_act());
    lv_obj_align(labelClock, LV_ALIGN_TOP_RIGHT, -15 , 15);
    lv_obj_set_style_text_font(labelClock, &dseg30, LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(labelClock, lv_color_white(), LV_STATE_DEFAULT);
    lv_label_set_text_fmt(labelClock, "%s", "23:00");
	
    labelSetValue = lv_label_create(lv_scr_act());
    lv_obj_align(labelSetValue, LV_ALIGN_RIGHT_MID, -25 , -15);
    lv_obj_set_style_text_font(labelSetValue, &dseg48, LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(labelSetValue, lv_color_white(), LV_STATE_DEFAULT);
    lv_label_set_text_fmt(labelSetValue, "%02d", 23);
	
	lv_obj_t * bolota2 = lv_label_create(lv_scr_act());
	lv_obj_align_to(bolota2, labelSetValue,LV_ALIGN_OUT_RIGHT_TOP, 5 , 0);
	lv_obj_set_style_text_color(bolota2, lv_color_white(), LV_STATE_DEFAULT);
	lv_obj_set_style_text_font(bolota2, &lv_font_montserrat_8, 0);
	lv_label_set_text(bolota2, "o");
	
	lv_obj_t * celsius2 = lv_label_create(lv_scr_act());
	lv_obj_align_to(celsius2, bolota2,LV_ALIGN_OUT_BOTTOM_LEFT, 6 , -6);
	lv_obj_set_style_text_color(celsius2, lv_color_white(), LV_STATE_DEFAULT);
	lv_label_set_text(celsius2, "C");
}

/************************************************************************/
/* TASKS                                                                */
/************************************************************************/
void vTimerCallback(TimerHandle_t xTimer) {
	/* Selecina canal e inicializa convers?o */
	afec_channel_enable(AFEC, AFEC_CHANNEL);
	afec_start_software_conversion(AFEC);
}

static void task_lcd(void *pvParameters) {
	configure_console();
	 
	lv_termostato();
	char atualizar = 1;
	for (;;)  {
		if( xSemaphoreTake(xSemaphoreDesliga, 0) == pdTRUE ){
			atualizar = 0;
		}
		if(atualizar){
			lv_tick_inc(50);
			lv_task_handler();
		}else{
			ili9341_backlight_off();
		}
		vTaskDelay(50);
	}
}

static void task_rtc(void *pvParameters) {
	calendar rtc_initial = {2018, 3, 19, 12, 20, 55 ,1};
	RTC_init(RTC, ID_RTC, rtc_initial, RTC_IER_SECEN);
	config_AFEC(AFEC, AFEC_ID, AFEC_CHANNEL, AFEC_callback);
	xTimer = xTimerCreate(/* Just a text name, not used by the RTOS
                          kernel. */
                          "Timer",
                          /* The timer period in ticks, must be
                          greater than 0. */
                          100,
                          /* The timers will auto-reload themselves
                          when they expire. */
                          pdTRUE,
                          /* The ID is used to store a count of the
                          number of times the timer has expired, which
                          is initialised to 0. */
                          (void *)0,
                          /* Timer callback */
                          vTimerCallback);
    xTimerStart(xTimer, 0);
	for (;;)  {
      if( xSemaphoreTake(xSemaphoreClock, 0) == pdTRUE ){
		   uint32_t current_hour, current_min, current_sec;
		   rtc_get_time(RTC, &current_hour, &current_min, &current_sec);
		   char *c = lv_label_get_text(labelClock);
		   if(c[2]== ':'){
			   if(current_min<10){
				   lv_label_set_text_fmt(labelClock, "%02d 0%2d", current_hour, current_min);
				   }else{
				   lv_label_set_text_fmt(labelClock, "%02d %2d", current_hour, current_min);
				   }
		       }else{
			   if(current_min<10){
				   lv_label_set_text_fmt(labelClock, "%02d:0%2d", current_hour, current_min);
				   }else{
				   lv_label_set_text_fmt(labelClock, "%02d:%2d", current_hour, current_min);
			   }
		   }	      
      }
	}
}

/************************************************************************/
/* configs                                                              */
/************************************************************************/

static void configure_lcd(void) {
	/**LCD pin configure on SPI*/
	pio_configure_pin(LCD_SPI_MISO_PIO, LCD_SPI_MISO_FLAGS);  //
	pio_configure_pin(LCD_SPI_MOSI_PIO, LCD_SPI_MOSI_FLAGS);
	pio_configure_pin(LCD_SPI_SPCK_PIO, LCD_SPI_SPCK_FLAGS);
	pio_configure_pin(LCD_SPI_NPCS_PIO, LCD_SPI_NPCS_FLAGS);
	pio_configure_pin(LCD_SPI_RESET_PIO, LCD_SPI_RESET_FLAGS);
	pio_configure_pin(LCD_SPI_CDS_PIO, LCD_SPI_CDS_FLAGS);
	
	ili9341_init();
	ili9341_backlight_on();
	
}

static void config_AFEC(Afec *afec, uint32_t afec_id, uint32_t afec_channel,
                            afec_callback_t callback) {
    /*************************************
     * Ativa e configura AFEC
     *************************************/
    /* Ativa AFEC - 0 */
    afec_enable(afec);

    /* struct de configuracao do AFEC */
    struct afec_config afec_cfg;

    /* Carrega parametros padrao */
    afec_get_config_defaults(&afec_cfg);

    /* Configura AFEC */
    afec_init(afec, &afec_cfg);

    /* Configura trigger por software */
    afec_set_trigger(afec, AFEC_TRIG_SW);

    /*** Configuracao espec?fica do canal AFEC ***/
    struct afec_ch_config afec_ch_cfg;
    afec_ch_get_config_defaults(&afec_ch_cfg);
    afec_ch_cfg.gain = AFEC_GAINVALUE_0;
    afec_ch_set_config(afec, afec_channel, &afec_ch_cfg);

    /*
    * Calibracao:
    * Because the internal ADC offset is 0x200, it should cancel it and shift
    down to 0.
    */
    afec_channel_set_analog_offset(afec, afec_channel, 0x200);

    /***  Configura sensor de temperatura ***/
    struct afec_temp_sensor_config afec_temp_sensor_cfg;

    afec_temp_sensor_get_config_defaults(&afec_temp_sensor_cfg);
    afec_temp_sensor_set_config(afec, &afec_temp_sensor_cfg);

    /* configura IRQ */
    afec_set_callback(afec, afec_channel, callback, 1);
    NVIC_SetPriority(afec_id, 4);
    NVIC_EnableIRQ(afec_id);
}

static void configure_console(void) {
	const usart_serial_options_t uart_serial_options = {
		.baudrate = USART_SERIAL_EXAMPLE_BAUDRATE,
		.charlength = USART_SERIAL_CHAR_LENGTH,
		.paritytype = USART_SERIAL_PARITY,
		.stopbits = USART_SERIAL_STOP_BIT,
	};

	/* Configure console UART. */
	stdio_serial_init(CONSOLE_UART, &uart_serial_options);

	/* Specify that stdout should not be buffered. */
	setbuf(stdout, NULL);
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
/************************************************************************/
/* port lvgl                                                            */
/************************************************************************/

void my_flush_cb(lv_disp_drv_t * disp_drv, const lv_area_t * area, lv_color_t * color_p) {
	ili9341_set_top_left_limit(area->x1, area->y1);   ili9341_set_bottom_right_limit(area->x2, area->y2);
	ili9341_copy_pixels_to_screen(color_p,  (area->x2 + 1 - area->x1) * (area->y2 + 1 - area->y1));
	
	/* IMPORTANT!!!
	* Inform the graphics library that you are ready with the flushing*/
	lv_disp_flush_ready(disp_drv);
}

void my_input_read(lv_indev_drv_t * drv, lv_indev_data_t*data) {
	int px, py, pressed;
	
	if (readPoint(&px, &py))
		data->state = LV_INDEV_STATE_PRESSED;
	else
		data->state = LV_INDEV_STATE_RELEASED; 
	
	data->point.x = px;
	data->point.y = py;
}

void configure_lvgl(void) {
	lv_init();
	lv_disp_draw_buf_init(&disp_buf, buf_1, NULL, LV_HOR_RES_MAX * LV_VER_RES_MAX);
	
	lv_disp_drv_init(&disp_drv);            /*Basic initialization*/
	disp_drv.draw_buf = &disp_buf;          /*Set an initialized buffer*/
	disp_drv.flush_cb = my_flush_cb;        /*Set a flush callback to draw to the display*/
	disp_drv.hor_res = LV_HOR_RES_MAX;      /*Set the horizontal resolution in pixels*/
	disp_drv.ver_res = LV_VER_RES_MAX;      /*Set the vertical resolution in pixels*/

	lv_disp_t * disp;
	disp = lv_disp_drv_register(&disp_drv); /*Register the driver and save the created display objects*/
	
	/* Init input on LVGL */
	lv_indev_drv_init(&indev_drv);
	indev_drv.type = LV_INDEV_TYPE_POINTER;
	indev_drv.read_cb = my_input_read;
	lv_indev_t * my_indev = lv_indev_drv_register(&indev_drv);
}

/************************************************************************/
/* main                                                                 */
/************************************************************************/
int main(void) {
	/* board and sys init */
	board_init();
	sysclk_init();
	configure_console();

	/* LCd, touch and lvgl init*/
	configure_lcd();
	configure_touch();
	configure_lvgl();


	
	  // cria semáforo binário
	 xSemaphoreClock = xSemaphoreCreateBinary();
	 if (xSemaphoreClock == NULL)
	 printf("falha em criar o semaforo \n");
	 
	  xSemaphoreDesliga = xSemaphoreCreateBinary();
	  if (xSemaphoreDesliga == NULL)
	  printf("falha em criar o semaforo \n");
	 
	/* Create task to control oled */
	if (xTaskCreate(task_lcd, "LCD", TASK_LCD_STACK_SIZE, NULL, TASK_LCD_STACK_PRIORITY, NULL) != pdPASS) {
		printf("Failed to create lcd task\r\n");
	}
	
	if (xTaskCreate(task_rtc, "RTC", TASK_LCD_STACK_SIZE, NULL, TASK_LCD_STACK_PRIORITY, NULL) != pdPASS) {
		printf("Failed to create rtc task\r\n");
	}
	
	/* Start the scheduler. */
	vTaskStartScheduler();

	while(1){ }
}
