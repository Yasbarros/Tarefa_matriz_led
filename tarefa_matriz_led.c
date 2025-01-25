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


// Função para definir o estado dos LEDs - Animação
static void setLEDS(uint32_t* estado) {
    memcpy(fitaEd, estado, sizeof(fitaEd));
    atualizaFita();
}

/// Animação da cobra
void animacaoCobraExplosiva() {
    uint32_t cobra_corpo = urgb_u32(0, 255, 0);  // Verde
    uint32_t cobra_cabeca = urgb_u32(255, 0, 0); // Vermelho

    // Frames para o movimento da cobra
    for (int i = 0; i < NLEDS; i++) {
        memset(fitaEd, 0, sizeof(fitaEd)); // Limpa os LEDs
        fitaEd[i] = cobra_cabeca; // Cabeça da cobra
        for (int j = 1; j <= i && j < 5; j++) {
            fitaEd[i - j] = cobra_corpo; // Corpo da cobra, com limite de 5 segmentos
        }
        atualizaFita();
        sleep_ms(200); // Tempo entre movimentos
    }

    // Explosão ao atingir o último LED
    for (int i = 0; i < 5; i++) { // Pisca aleatoriamente 5 vezes
        uint32_t explosao_cor = urgb_u32(rand() % 256, rand() % 256, rand() % 256); // Cores aleatórias
        for (int j = 0; j < NLEDS; j++) {
            fitaEd[j] = (i % 2 == 0) ? explosao_cor : 0; // Alterna entre a cor aleatória e apagado
        }
        atualizaFita();
        sleep_ms(200); // Intervalo entre piscadas
    }
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

// Função para gerar uma cor principal aleatória
uint32_t random_color() {
    uint8_t color_index = rand() % 7; // Gera um número de 0 a 6
    switch (color_index) {
        case 0: return urgb_u32(128, 0, 0);   // Vermelho
        case 1: return urgb_u32(0, 128, 0);   // Verde
        case 2: return urgb_u32(0, 0, 128);   // Azul
        case 3: return urgb_u32(0, 128, 128); // Ciano
        case 4: return urgb_u32(128, 0, 128); // Magenta
        case 5: return urgb_u32(128, 128, 0); // Amarelo
        case 6: return urgb_u32(128, 128, 128); // Branco
    }
    return urgb_u32(0, 0, 0); 
}

// Função para alerta visual e sonoro após o fim da contagem regressiva
void alert() {
    uint32_t frame[NLEDS] = {0};
    frame[12] = urgb_u32(128, 0, 0);
    setLEDS(frame);
    sleep_ms(300);
    int step2[] = {6, 7, 8, 11, 13, 16, 17, 18};
    for (int i = 0; i < 8; i++) frame[step2[i]] = urgb_u32(128, 64, 0);
    setLEDS(frame);
    sleep_ms(300);
    int step3[] = {0, 1, 2, 3, 4, 5, 9, 10, 14, 15, 19, 20, 21, 22, 23, 24};
    for (int i = 0; i < 16; i++) frame[step3[i]] = urgb_u32(128, 128, 0);
    setLEDS(frame);
    sleep_ms(300);

    int i = 5;
    while (i--)
    {
        apagaLEDS();
        sleep_ms(250);
        acendeLEDS(random_color());
        emiteSom(250, 100);
    }
}

// Função para exibir uma contagem regressiva de 5 segundos
void contagem_regressiva() {
    uint32_t frames[][NLEDS] = {
        {   // Dígito 0
            1, 1, 1, 1, 1,
            1, 0, 0, 0, 1,
            1, 0, 0, 0, 1,
            1, 0, 0, 0, 1,
            1, 1, 1, 1, 1
        },
        {   // Dígito 1
            0, 0, 1, 0, 0,
            0, 0, 1, 0, 0,
            0, 0, 1, 0, 0,
            0, 0, 1, 0, 0,
            0, 0, 1, 1, 0
        },
        {   // Dígito 2
            1, 1, 1, 1, 1,
            1, 0, 0, 0, 0,
            1, 1, 1, 1, 1,
            0, 0, 0, 0, 1,
            1, 1, 1, 1, 1
        },
        {   // Dígito 3
            1, 1, 1, 1, 1,
            0, 0, 0, 0, 1,
            1, 1, 1, 1, 0,
            0, 0, 0, 0, 1,
            1, 1, 1, 1, 1
        },
        {   // Dígito 4
            1, 0, 0, 0, 0,
            0, 0, 0, 0, 1,
            1, 1, 1, 1, 1,
            1, 0, 0, 0, 1,
            1, 0, 0, 0, 1
        },
        {   // Dígito 5 
            1, 1, 1, 1, 1,
            0, 0, 0, 0, 1,
            1, 1, 1, 1, 1,
            1, 0, 0, 0, 0,
            1, 1, 1, 1, 1
        },
    };

    // Exibe cada frame do dígito 5 até 0 com uma cor principal e um som aleatório
    for (size_t i = sizeof(frames) / sizeof(frames[0]) - 1; i + 1; i--)
    {
        uint32_t color = random_color();
        for (size_t j = 0; j < NLEDS; j++) if(frames[i][j]) frames[i][j] = color;
        setLEDS(frames[i]);
        emiteSom(500, (rand() % 4000) + 100);
        sleep_ms(500);
    }
    apagaLEDS();

    alert();
    
    apagaLEDS();
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
                    contagem_regressiva();
                    break;
                // Para as teclas de '0' a '9', mostra imagem 
                case '1':
                animacaoCobraExplosiva();
                 break;
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