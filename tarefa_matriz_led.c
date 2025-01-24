#include <string.h>  // Funções para manipulação de strings e memória (ex.: memset, usado em apagaLEDS)
#include <stdio.h>   // Funções de entrada e saída padrão (ex.: printf, usado para mensagens no console)
#include <stdlib.h>  // Funções padrão (ex.: rand, usado para gerar números aleatórios)
#include "pico/stdlib.h"  // Funções padrão do Raspberry Pi Pico (GPIO, temporização, inicialização)
#include "hardware/pio.h"  // Controle do PIO (Programmable Input/Output, usado para os LEDs WS2812)
#include "hardware/dma.h"  // Controle do DMA (Direct Memory Access, usado para atualizar LEDs)
#include "pico/bootrom.h"  // Funções relacionadas ao bootloader (ex.: reset_usb_boot para reinício no modo bootloader)
#include "ws2812.pio.h"  // Programa PIO e inicialização para controlar os LEDs WS2812


#define PIN_TX 7
#define NLEDS 25
#define WIDTH 5
#define HEIGHT 5

#define ROWS 4
#define COLS 4
uint8_t row_pins[ROWS] = {2, 3, 4, 5};
uint8_t col_pins[COLS] = {6, 10, 8, 9};

const char keys[ROWS][COLS] = {
    {'1', '2', '3', 'A'},
    {'4', '5', '6', 'B'},
    {'7', '8', '9', 'C'},
    {'*', '0', '#', 'D'}
};

#define BUZZER_PIN 21  // Definindo o pino do buzzer

static PIO pio;
static int sm;
static uint dma_chan;
static uint32_t fitaEd[NLEDS];

// Função para representar a cor em formato RGB
static inline uint32_t urgb_u32(uint8_t r, uint8_t g, uint8_t b) {
    return ((uint32_t)(g) << 24) | ((uint32_t)(r) << 16) | ((uint32_t)(b) << 8);
}

// Função para atualizar os LEDs
static void atualizaFita() {
    dma_channel_wait_for_finish_blocking(dma_chan);
    while (!pio_sm_is_tx_fifo_empty(pio, sm)) {
        sleep_us(10);
    }
    dma_channel_config c = dma_channel_get_default_config(dma_chan);
    channel_config_set_read_increment(&c, true);
    channel_config_set_write_increment(&c, false);
    channel_config_set_dreq(&c, pio_get_dreq(pio, sm, true));
    dma_channel_configure(dma_chan, &c, &pio->txf[sm], fitaEd, NLEDS, true);
    sleep_us(300);
}

// Apaga os LEDs
static void apagaLEDS() {
    memset(fitaEd, 0, sizeof(fitaEd));
    atualizaFita();
}

// Acende os LEDs com uma cor específica
static void acendeLEDS(uint32_t cor) {
    for (int i = 0; i < NLEDS; i++) {
        fitaEd[i] = cor;
    }
    atualizaFita();
}

// Função para mostrar uma imagem ou padrão aleatório nos LEDs
void mostraImagemAleatoria() {
    for (int i = 0; i < NLEDS; i++) {
        fitaEd[i] = urgb_u32(rand() % 256, rand() % 256, rand() % 256);
    }
    atualizaFita();
}

// Função para gerar um sinal sonoro
void emiteSom(uint32_t duracao_ms, uint32_t frequencia_hz) {
    uint32_t periodo = 1000000 / frequencia_hz;  // Calcula o período do sinal (em microssegundos)
    uint32_t ciclos = (duracao_ms * 1000) / periodo;  // Calcula quantos ciclos serão emitidos

    for (uint32_t i = 0; i < ciclos; i++) {
        gpio_put(BUZZER_PIN, 1);  // Ativa o buzzer
        sleep_us(periodo / 2);    // Espera metade do período
        gpio_put(BUZZER_PIN, 0);  // Desativa o buzzer
        sleep_us(periodo / 2);    // Espera a outra metade do período
    }
}

// Inicializa os pinos da matriz de teclado
void init_gpio() {
    for (int i = 0; i < ROWS; i++) {
        gpio_init(row_pins[i]);
        gpio_set_dir(row_pins[i], GPIO_OUT);
        gpio_put(row_pins[i], 1);
    }
    for (int i = 0; i < COLS; i++) {
        gpio_init(col_pins[i]);
        gpio_set_dir(col_pins[i], GPIO_IN);
        gpio_pull_up(col_pins[i]);
    }

    gpio_init(BUZZER_PIN);  // Inicializa o pino do buzzer
    gpio_set_dir(BUZZER_PIN, GPIO_OUT);  // Define o pino como saída
}

// Função para escanear o teclado e retornar a tecla pressionada
char scan_keypad() {
    for (int row = 0; row < ROWS; row++) {
        gpio_put(row_pins[row], 0);
        for (int col = 0; col < COLS; col++) {
            if (gpio_get(col_pins[col]) == 0) {
                gpio_put(row_pins[row], 1);
                return keys[row][col];
            }
        }
        gpio_put(row_pins[row], 1);
    }
    return 0;
}

int main() {
    stdio_init_all();

    pio = pio0;
    sm = 0;
    dma_chan = dma_claim_unused_channel(true);
    uint offset = pio_add_program(pio, &ws2812_program);
    ws2812_program_init(pio, sm, offset, PIN_TX, 800000, false);

    apagaLEDS();
    init_gpio();

    while (1) {
        char key = scan_keypad();

        if (key) {
            printf("Tecla pressionada: %c\n", key);
            switch (key) {
                case 'A':
                    apagaLEDS();  // Apaga LEDs
                    break;
                case 'B':
                    acendeLEDS(urgb_u32(0, 0, 255));  // Azul
                    break;
                case 'C':
                    acendeLEDS(urgb_u32(204, 0, 0));  // Vermelho
                    break;
                case 'D':
                    acendeLEDS(urgb_u32(0, 128, 0));  // Verde
                    break;
                case '#':
                    acendeLEDS(urgb_u32(51, 51, 51));  // Branco
                    break;
                case '*':
                    printf("Reiniciando para modo de gravação...\n");
                    apagaLEDS();  // Apaga todos os leds antes de entrar em modo bootloader
                    sleep_ms(100);
                    reset_usb_boot(0, 0);  // Reinicia no modo bootloader
                    break;
                case '0':  // Exemplo de som para animação
                    mostraImagemAleatoria();
                    emiteSom(500, 1000);  // Emite som de 500ms com 1000Hz
                    break;
                // Para as teclas de '1' a '9', mostra imagem aleatória
                case '1':
                case '2':
                case '3':
                case '4':
                case '5':
                case '6':
                case '7':
                case '8':
                case '9':
                    mostraImagemAleatoria();  // Mostra imagem aleatória
                    break;
                default:
                    break;
            }

            sleep_ms(200);
        }

        sleep_ms(50);
    }
}