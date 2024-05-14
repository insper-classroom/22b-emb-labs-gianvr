#include <asf.h>

#include "gfx_mono_ug_2832hsweg04.h"

#include "gfx_mono_text.h"
#include "sysfont.h"

#define BUT_PIO_OLED1 PIOD
#define BUT_PIO_OLED1_ID ID_PIOD
#define BUT_PIO_OLED1_IDX 28
#define BUT_PIO_OLED1_IDX_MASK (1 << BUT_PIO_OLED1_IDX)

#define BUT_PIO_OLED2 PIOC
#define BUT_PIO_OLED2_ID ID_PIOC
#define BUT_PIO_OLED2_IDX 31
#define BUT_PIO_OLED2_IDX_MASK (1 << BUT_PIO_OLED2_IDX)

#define BUT_PIO_OLED3 PIOA
#define BUT_PIO_OLED3_ID ID_PIOA
#define BUT_PIO_OLED3_IDX 19
#define BUT_PIO_OLED3_IDX_MASK (1 << BUT_PIO_OLED3_IDX)

#define LED_PIO PIOC
#define LED_PIO_ID ID_PIOC
#define LED_IDX 8
#define LED_IDX_MASK (1 << LED_IDX)

unsigned int delay_piscada = 100;
unsigned int piscadas = 30;
volatile unsigned int but1_pressed = 0;
volatile unsigned int but3_pressed = 0;
volatile unsigned int parar = 0;
unsigned int piscando = 0;


void but_oled1_callback() {
    if (pio_get(BUT_PIO_OLED1, PIO_INPUT, BUT_PIO_OLED1_IDX_MASK)) {
        but1_pressed = 0;
    } else {
        but1_pressed = 1;
		if(piscando){
			if(delay_piscada>100){
				delay_piscada -= 100;
			}
		}		
    }
}

void but_oled2_callback() {
    parar = 1;
}

void but_oled3_callback() {
    delay_piscada += 100;
    but3_pressed = 1;
}

float freq_hertz(){
	return 1/(2*delay_piscada/1000.0);
}
void pisca_led() {
	piscando = 1;
    char str[128];
    unsigned int ant_delay = delay_piscada;
    parar = 0;
    sprintf(str, "%.2f", freq_hertz());
    gfx_mono_draw_string(str, 35, 16, &sysfont);
    for (unsigned int i = 0; i < piscadas; i++) {
        unsigned int progress = 124 * (i + 1) / piscadas;
        gfx_mono_draw_filled_rect(2, 2, progress, 10, GFX_PIXEL_SET);
        if (ant_delay != delay_piscada) {
            sprintf(str, "%.2f", freq_hertz());
            gfx_mono_draw_string(str, 35, 16, &sysfont);
            ant_delay = delay_piscada;
        }
        pio_clear(LED_PIO, LED_IDX_MASK);
        delay_ms(delay_piscada);
        pio_set(LED_PIO, LED_IDX_MASK);
        delay_ms(delay_piscada);
        if (parar) {
            gfx_mono_draw_filled_rect(2, 2, 124, 10, GFX_PIXEL_CLR);
			piscando = 0;
            break;
        }
    }
    gfx_mono_draw_filled_rect(2, 2, 124, 10, GFX_PIXEL_CLR);
	piscando = 0;
}

void io_init(void) {
    pmc_enable_periph_clk(LED_PIO_ID);
    pio_configure(LED_PIO, PIO_OUTPUT_0, LED_IDX_MASK, PIO_DEFAULT);

    pmc_enable_periph_clk(BUT_PIO_OLED1);
    pmc_enable_periph_clk(BUT_PIO_OLED2);
    pmc_enable_periph_clk(BUT_PIO_OLED3);

    pio_configure(BUT_PIO_OLED1, PIO_INPUT, BUT_PIO_OLED1_IDX_MASK, PIO_PULLUP | PIO_DEBOUNCE);
    pio_configure(BUT_PIO_OLED2, PIO_INPUT, BUT_PIO_OLED2_IDX_MASK, PIO_PULLUP | PIO_DEBOUNCE);
    pio_configure(BUT_PIO_OLED3, PIO_INPUT, BUT_PIO_OLED3_IDX_MASK, PIO_PULLUP | PIO_DEBOUNCE);

    pio_set_debounce_filter(BUT_PIO_OLED1, BUT_PIO_OLED1_IDX_MASK, 60);
    pio_set_debounce_filter(BUT_PIO_OLED2, BUT_PIO_OLED2_IDX_MASK, 60);
    pio_set_debounce_filter(BUT_PIO_OLED3, BUT_PIO_OLED3_IDX_MASK, 60);

    pio_handler_set(BUT_PIO_OLED1,
                    BUT_PIO_OLED1_ID,
                    BUT_PIO_OLED1_IDX_MASK,
                    PIO_IT_EDGE,
                    but_oled1_callback);

    pio_handler_set(BUT_PIO_OLED2,
                    BUT_PIO_OLED2_ID,
                    BUT_PIO_OLED2_IDX_MASK,
                    PIO_IT_FALL_EDGE,
                    but_oled2_callback);

    pio_handler_set(BUT_PIO_OLED3,
                    BUT_PIO_OLED3_ID,
                    BUT_PIO_OLED3_IDX_MASK,
                    PIO_IT_FALL_EDGE,
                    but_oled3_callback);

    pio_enable_interrupt(BUT_PIO_OLED1, BUT_PIO_OLED1_IDX_MASK);
    pio_enable_interrupt(BUT_PIO_OLED2, BUT_PIO_OLED2_IDX_MASK);
    pio_enable_interrupt(BUT_PIO_OLED3, BUT_PIO_OLED3_IDX_MASK);

    pio_get_interrupt_status(BUT_PIO_OLED1);
    pio_get_interrupt_status(BUT_PIO_OLED2);
    pio_get_interrupt_status(BUT_PIO_OLED3);

    NVIC_EnableIRQ(BUT_PIO_OLED1_ID);
    NVIC_EnableIRQ(BUT_PIO_OLED2_ID);
    NVIC_EnableIRQ(BUT_PIO_OLED3_ID);

    NVIC_SetPriority(BUT_PIO_OLED1_ID, 4);
    NVIC_SetPriority(BUT_PIO_OLED2_ID, 4);
    NVIC_SetPriority(BUT_PIO_OLED3_ID, 4);
}

int main(void) {
    board_init();
    sysclk_init();
    WDT->WDT_MR = WDT_MR_WDDIS;
    delay_init();

    // Init OLED
    gfx_mono_ssd1306_init();

    char str[128];
    sprintf(str, "%.2f", freq_hertz());
    gfx_mono_draw_string(str, 35, 16, &sysfont);
	gfx_mono_draw_string(" Hz", 75, 16, &sysfont);

    pio_set(LED_PIO, LED_IDX_MASK);

    io_init();

    while (1) {
        unsigned int hertz = sysclk_get_cpu_hz();
        if (but3_pressed) {
            sprintf(str, "%.2f", freq_hertz());
            gfx_mono_draw_string(str, 35, 16, &sysfont);
            but3_pressed = 0;
        }
        if (but1_pressed) {
            for (unsigned int i = 0; i < hertz/25; i++) {
                if (pio_get(BUT_PIO_OLED1, PIO_INPUT, BUT_PIO_OLED1_IDX_MASK)) {
                    if (delay_piscada > 100) {
                        delay_piscada -= 100;
                    }
                    break;
                }
            }
            if (!pio_get(BUT_PIO_OLED1, PIO_INPUT, BUT_PIO_OLED1_IDX_MASK)) {
                delay_piscada += 100;
            }
            pisca_led();
        } else {
            pmc_sleep(SAM_PM_SMODE_SLEEP_WFI);
        }
    }
}