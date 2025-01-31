#include <stdio.h>
#include <stdint.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/timer.h"
#include "pico/cyw43_arch.h"
#include "ws2818b.pio.h"

#define DEBOUNCE_DELAY 1000 // Atraso de 200ms para debouncing

#define MATRIZ 7
#define NUM_PIXELS 25

#define LED_RED 13
#define LED_GREEN 12
#define LED_BLUE 11

#define BUTTON_A 5
#define BUTTON_B 6

// Definição de pixel GRB
struct pixel_t
{
  uint8_t R, G, B; // Três valores de 8-bits compõem um pixel.
};
typedef struct pixel_t pixel_t;
typedef pixel_t npLED_t; // Mudança de nome de "struct pixel_t" para "npLED_t" por clareza.

// Variaveis Globais
npLED_t leds[NUM_PIXELS]; // Declaração do buffer de pixels que formam a matriz.
PIO np_pio;               // Variáveis para uso da máquina PIO.
uint sm;
uint8_t numberLED; // Variavel de estado do número
static volatile uint32_t last_interrupt_time = 0;

// Variavel que armazena simbolo dos números
const npLED_t numberRGB[10][NUM_PIXELS] = {
    {{255, 40, 0}, {255, 40, 0}, {255, 40, 0}, {255, 40, 0}, {255, 40, 0}, {255, 40, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {255, 40, 0}, {255, 40, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {255, 40, 0}, {255, 40, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {255, 40, 0}, {255, 40, 0}, {255, 40, 0}, {255, 40, 0}, {255, 40, 0}, {255, 40, 0}},
    {{0, 0, 0}, {255, 40, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {255, 40, 0}, {0, 0, 0}, {0, 0, 0}, {255, 40, 0}, {0, 0, 0}, {255, 40, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {255, 40, 0}, {255, 40, 0}, {0, 0, 0}, {0, 0, 0}, {255, 40, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}},
    {{255, 40, 0}, {255, 40, 0}, {255, 40, 0}, {255, 40, 0}, {255, 40, 0}, {0, 0, 0}, {255, 40, 0}, {255, 40, 0}, {255, 40, 0}, {255, 40, 0}, {255, 40, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {255, 40, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {255, 40, 0}, {0, 0, 0}, {255, 40, 0}, {255, 40, 0}, {255, 40, 0}, {0, 0, 0}},
    {{255, 40, 0}, {255, 40, 0}, {255, 40, 0}, {255, 40, 0}, {255, 40, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {255, 40, 0}, {255, 40, 0}, {255, 40, 0}, {255, 40, 0}, {255, 40, 0}, {255, 40, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {255, 40, 0}, {255, 40, 0}, {255, 40, 0}, {255, 40, 0}, {255, 40, 0}, {255, 40, 0}},
    {{255, 40, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {255, 40, 0}, {255, 40, 0}, {255, 40, 0}, {255, 40, 0}, {255, 40, 0}, {255, 40, 0}, {255, 40, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {255, 40, 0}, {255, 40, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {255, 40, 0}},
    {{255, 40, 0}, {255, 40, 0}, {255, 40, 0}, {255, 40, 0}, {255, 40, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {255, 40, 0}, {255, 40, 0}, {255, 40, 0}, {255, 40, 0}, {255, 40, 0}, {255, 40, 0}, {255, 40, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {255, 40, 0}, {255, 40, 0}, {255, 40, 0}, {255, 40, 0}, {255, 40, 0}},
    {{255, 40, 0}, {255, 40, 0}, {255, 40, 0}, {255, 40, 0}, {0, 0, 0}, {255, 40, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {255, 40, 0}, {0, 0, 0}, {255, 40, 0}, {255, 40, 0}, {255, 40, 0}, {255, 40, 0}, {0, 0, 0}, {255, 40, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {255, 40, 0}, {255, 40, 0}, {0, 0, 0}, {0, 0, 0}},
    {{255, 40, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {255, 40, 0}, {255, 40, 0}, {255, 40, 0}, {255, 40, 0}, {255, 40, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {255, 40, 0}, {255, 40, 0}, {255, 40, 0}, {255, 40, 0}, {255, 40, 0}, {255, 40, 0}},
    {{255, 40, 0}, {255, 40, 0}, {255, 40, 0}, {255, 40, 0}, {255, 40, 0}, {255, 40, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {255, 40, 0}, {255, 40, 0}, {255, 40, 0}, {255, 40, 0}, {255, 40, 0}, {255, 40, 0}, {255, 40, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {255, 40, 0}, {255, 40, 0}, {255, 40, 0}, {255, 40, 0}, {255, 40, 0}, {255, 40, 0}},
    {{255, 40, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {255, 40, 0}, {255, 40, 0}, {255, 40, 0}, {255, 40, 0}, {255, 40, 0}, {255, 40, 0}, {255, 40, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {255, 40, 0}, {255, 40, 0}, {255, 40, 0}, {255, 40, 0}, {255, 40, 0}, {255, 40, 0}}};

/**
 * Inicializa a máquina PIO para controle da matriz de LEDs.
 */
void npInitMatriz()
{
  // Cria programa PIO.
  uint offset = pio_add_program(pio0, &ws2818b_program);
  np_pio = pio0;

  // Toma posse de uma máquina PIO.
  sm = pio_claim_unused_sm(np_pio, false);
  if (sm < 0)
  {
    np_pio = pio1;
    sm = pio_claim_unused_sm(np_pio, true); // Se nenhuma máquina estiver livre, panic!
  }

  // Inicia programa na máquina PIO obtida.
  ws2818b_program_init(np_pio, sm, offset, MATRIZ, 800000.f);

  // Limpa buffer de pixels.
  for (uint i = 0; i < NUM_PIXELS; ++i)
  {
    leds[i].R = 0;
    leds[i].G = 0;
    leds[i].B = 0;
  }
}

/**
 * Atribui uma cor RGB a um LED.
 */
void buffer_SetLED(const uint index, const uint8_t r, const uint8_t g, const uint8_t b)
{
  leds[index].R = r;
  leds[index].G = g;
  leds[index].B = b;
}

/**
 * Limpa o buffer de pixels.
 */
void buffer_Clear()
{
  for (uint i = 0; i < NUM_PIXELS; ++i)
    buffer_SetLED(i, 0, 0, 0);
}

/**
 * Atualiza os LEDS com as informções do buffer.
 */
void buffer_WriteLEDs()
{
  // Escreve cada dado de 8-bits dos pixels em sequência no buffer da máquina PIO.
  for (uint i = 0; i < NUM_PIXELS; ++i)
  {
    pio_sm_put_blocking(np_pio, sm, leds[i].G);
    pio_sm_put_blocking(np_pio, sm, leds[i].R);
    pio_sm_put_blocking(np_pio, sm, leds[i].B);
  }
  sleep_us(100); // Espera 100us, sinal de RESET do datasheet.
}

/**
 * Troca o valor exibido na matriz de LEDS por um novo valor.
 */
void Matriz_changeNumber(const npLED_t newDraw[])
{

  // Coloca os novos valores fornecidos no buffer
  for (size_t i = 0; i < NUM_PIXELS; i++)
  {
    buffer_SetLED(i, newDraw[i].R, newDraw[i].G, newDraw[i].B);
  }

  // Escreve nos LEDs
  buffer_WriteLEDs();
}

/**
 * Incrementa o valor exibido na matriz de LEDS.
 */
void led_Increment()
{
  printf("BUTTON_B Pressed! Increasing LED.\n");
  if (numberLED != 9)
  {
    numberLED++;
  }
}

/**
 * Decrementa o valor exibido na matriz de LEDS.
 */
void led_Decrement()
{
  printf("BUTTON_A Pressed! Decreasing LED.\n");
  if (numberLED != 0)
  {
    numberLED--;
  }
}

/**
 * Função que organiza os callbacks do sistema
 */
void gpio_callback(uint gpio, uint32_t events)
{
  // static uint32_t last_interrupt_time = 0;
  uint32_t current_time = to_ms_since_boot(get_absolute_time());

  // Verifica se o tempo de debouncing passou
  if (current_time - last_interrupt_time > DEBOUNCE_DELAY)
  {
    if (gpio == BUTTON_A)
      led_Decrement();
    if (gpio == BUTTON_B)
      led_Increment();

    // Atualiza o tempo da última interrupção
    last_interrupt_time = current_time;
  }
}

/**
 * Inicializa a os operadores e entra no loop continuo de funcionamento
 * Função principal do sistema.
 */
int main()
{
  stdio_init_all();

  // Inicia os LEDS
  gpio_init(LED_RED);
  gpio_set_dir(LED_RED, GPIO_OUT);
  gpio_init(LED_GREEN);
  gpio_set_dir(LED_GREEN, GPIO_OUT);
  gpio_init(LED_BLUE);
  gpio_set_dir(LED_BLUE, GPIO_OUT);

  // Inicia os Botões
  gpio_init(BUTTON_A);
  gpio_set_dir(BUTTON_A, GPIO_IN);
  gpio_pull_up(BUTTON_A);

  gpio_init(BUTTON_B);
  gpio_set_dir(BUTTON_B, GPIO_IN);
  gpio_pull_up(BUTTON_B);

  // Inicia Matriz de LEDS com PIO
  npInitMatriz();

  // Configuração da interrupção com callback
  gpio_set_irq_enabled_with_callback(BUTTON_A, GPIO_IRQ_EDGE_FALL, true, &gpio_callback);
  gpio_set_irq_enabled(BUTTON_B, GPIO_IRQ_EDGE_FALL, true);

  // Setando o Numero no LED
  numberLED = 0;
  Matriz_changeNumber(numberRGB[numberLED]);

  // Programa Inciado!
  printf("Programa Inciado!\n");

  // Program looping
  while (true)
  {
    // BLINKING
    gpio_put(LED_RED, true);
    sleep_ms(200);
    gpio_put(LED_RED, false);
    sleep_ms(200);
    Matriz_changeNumber(numberRGB[numberLED]);
  }
}
