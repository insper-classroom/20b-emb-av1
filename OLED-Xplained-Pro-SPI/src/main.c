#include <asf.h>

//#include "gfx_mono_ug_2832hsweg04.h"
//#include "gfx_mono_text.h"
//#include "sysfont.h"

/**
 * IOS
 */
#define BUT1_PIO          PIOD
#define BUT1_PIO_ID       ID_PIOD
#define BUT1_PIO_IDX      28
#define BUT1_PIO_IDX_MASK (1u << BUT1_PIO_IDX)

//BOTÃO PLAY/PAUSE
#define BUT2_PIO           PIOC
#define BUT2_PIO_ID	       ID_PIOC
#define BUT2_PIO_IDX       31
#define BUT2_PIO_IDX_MASK (1u << BUT2_PIO_IDX)

#define BUT3_PIO           PIOA
#define BUT3_PIO_ID        ID_PIOA
#define BUT3_PIO_IDX       19
#define BUT3_PIO_IDX_MASK (1u << BUT3_PIO_IDX)

// LED1
#define LED1_PIO      PIOA
#define LED1_PIO_ID   ID_PIOA
#define LED1_IDX      0
#define LED1_IDX_MASK (1 << LED1_IDX)

// LED2
#define LED2_PIO      PIOC
#define LED2_PIO_ID   ID_PIOC
#define LED2_IDX      30
#define LED2_IDX_MASK (1 << LED2_IDX)

// LED3
#define LED3_PIO      PIOB
#define LED3_PIO_ID   ID_PIOB
#define LED3_IDX      2
#define LED3_IDX_MASK (1 << LED3_IDX)

// sequencia
enum LED {LED1 = 1, LED2, LED3} LEDS;

int seq0[] = {LED1, LED3, LED2};
int seq0_len = sizeof(seq0)/sizeof(seq0[0]);

// flags
volatile char flag_but1 = 0;
volatile char flag_but2 = 0;
volatile char flag_but3 = 0;
volatile char flag_rtt  = 0;

// prototype
void BUT1_callback(void);
void BUT2_callback(void);
void BUT3_callback(void);
int genius_play(int seq[], int seq_len, int delay);
int user_play(int seq[], int seq_len);

/**************************************************************/
/* IRQ                                                        */
/**************************************************************/

void RTT_Handler (void) {
  uint32_t ul_status;

  /* Get RTT status - ACK */
  ul_status = rtt_get_status(RTT);

  /* IRQ due to Time has changed */
  if ((ul_status & RTT_SR_RTTINC) == RTT_SR_RTTINC) {  }

  /* IRQ due to Alarm */
  if ((ul_status & RTT_SR_ALMS) == RTT_SR_ALMS) {
      flag_rtt = 1;                
   }
}

void BUT1_callback (void) {
  flag_but1 = 1;
}

void BUT2_callback (void) {
  flag_but2 = 1;
}

void BUT3_callback (void) {
  flag_but3 = 1;
}

/**************************************************************/
/* funcoes                                                    */
/**************************************************************/

void init (void) {
	board_init();
	sysclk_init();
	delay_init();
//	SysTick_Config(sysclk_get_cpu_hz() / 1000); // 1 ms

//	gfx_mono_ssd1306_init();

	pmc_enable_periph_clk(BUT1_PIO_ID);
	pmc_enable_periph_clk(BUT2_PIO_ID);
	pmc_enable_periph_clk(BUT3_PIO_ID);

	pio_set_input(BUT1_PIO,BUT1_PIO_IDX_MASK,PIO_DEFAULT);
	pio_set_input(BUT2_PIO,BUT2_PIO_IDX_MASK,PIO_DEFAULT);
	pio_set_input(BUT3_PIO,BUT3_PIO_IDX_MASK,PIO_DEFAULT);

	pio_pull_up(BUT1_PIO,BUT1_PIO_IDX_MASK,1);
	pio_pull_up(BUT2_PIO,BUT2_PIO_IDX_MASK,1);
	pio_pull_up(BUT3_PIO,BUT3_PIO_IDX_MASK,1);

	pio_enable_interrupt(BUT1_PIO, BUT1_PIO_IDX_MASK);
	pio_enable_interrupt(BUT2_PIO, BUT2_PIO_IDX_MASK);
	pio_enable_interrupt(BUT3_PIO, BUT3_PIO_IDX_MASK);

  pio_get_interrupt_status(BUT1_PIO);
  pio_get_interrupt_status(BUT2_PIO);
  pio_get_interrupt_status(BUT3_PIO);

	NVIC_EnableIRQ(BUT1_PIO_ID);
	NVIC_SetPriority(BUT1_PIO_ID, 5); // Priority 2

	NVIC_EnableIRQ(BUT2_PIO_ID);
	NVIC_SetPriority(BUT2_PIO_ID, 5); // Priority 2

	NVIC_EnableIRQ(BUT3_PIO_ID);
	NVIC_SetPriority(BUT3_PIO_ID, 5); // Priority 2

	pio_handler_set(BUT1_PIO, BUT1_PIO_ID, BUT1_PIO_IDX_MASK, PIO_IT_EDGE, BUT1_callback);
	pio_handler_set(BUT2_PIO, BUT2_PIO_ID, BUT2_PIO_IDX_MASK, PIO_IT_EDGE, BUT2_callback);
	pio_handler_set(BUT3_PIO, BUT3_PIO_ID, BUT3_PIO_IDX_MASK, PIO_IT_EDGE, BUT3_callback);

  pmc_enable_periph_clk(LED1_PIO_ID);
	pio_set_output(LED1_PIO, LED1_IDX_MASK, 1, 0, 0);

  pmc_enable_periph_clk(LED2_PIO_ID);
	pio_set_output(LED2_PIO, LED2_IDX_MASK, 1, 0, 0);

  pmc_enable_periph_clk(LED3_PIO_ID);
	pio_set_output(LED3_PIO, LED3_IDX_MASK, 1, 0, 0);
}

void RTT_init(uint16_t pllPreScale, uint32_t IrqNPulses) {
  uint32_t ul_previous_time;

  /* Configure RTT for a 1 second tick interrupt */
  rtt_sel_source(RTT, false);
  rtt_init(RTT, pllPreScale);

  ul_previous_time = rtt_read_timer_value(RTT);
  while (ul_previous_time == rtt_read_timer_value(RTT));

  rtt_write_alarm_time(RTT, IrqNPulses+ul_previous_time);

  /* Enable RTT interrupt */
  NVIC_DisableIRQ(RTT_IRQn);
  NVIC_ClearPendingIRQ(RTT_IRQn);
  NVIC_SetPriority(RTT_IRQn, 4);
  NVIC_EnableIRQ(RTT_IRQn);
  rtt_enable_interrupt(RTT, RTT_MR_ALMIEN | RTT_MR_RTTINCIEN);
}

void play_led1 (int time) {
}

void play_led2 (int time) {
}

void play_led3 (int time) {
}

void start_rtt (void) {
  uint16_t pllPreScale = (int) (((float) 32768) / 4.0);
  uint32_t irqRTTvalue = 12;

  // reinicia RTT para gerar um novo IRQ
  RTT_init(pllPreScale, irqRTTvalue);

  // reseta flag
  flag_rtt = 0;
}

/**
 * Função para tocar a sequência
 * seq[]  : Vetor contendo a sequêncua
 * seq_len: Tamanho da sequência
 * delay  : Tempo em ms entre um led e outro
 **/
int genius_play (int seq[], int seq_len, int delay) {

}

/**
 * Função que aguardar pelos inputs do usuário
 * e verifica se sequencia está correta ou houve
 * um erro.
 *
 * A função deve retornar:
 *  0: se a sequência foi correta
 *  1: se teve algum erro na sequência (retornar imediatamente)
 */
int user_play (int seq[], int seq_len) {

}

/**
 * Função que exibe nos LEDs que o jogador acertou
 * Deve manter todos os LEDs acesos por um tempo
 * e então apagar
 */
void player_sucess (void) {

}

/**
 * Função que exibe nos LEDs que o jogador errou
 * Deve piscar os LEDs até o usuário apertar um botão
 */
void player_error (void) {

}

int main (void) {
 
  init();

  /* Insert application code here, after the board has been initialized. */
	while(1) {
	}
}
