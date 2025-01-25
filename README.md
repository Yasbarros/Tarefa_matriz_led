# Projeto: Matriz de LEDs 5x5 com Raspberry Pi Pico

Este projeto implementa o controle de uma matriz de LEDs 5x5 utilizando o Raspberry Pi Pico. Ele também integra um teclado matricial para interação e controle das cores dos LEDs, assim como um buzzer para emitir sinais sonoros.

## Bibliotecas Utilizadas

### Bibliotecas Padrão

1. `#include <string.h>`  
   **Utilidade:** Manipulação de strings e memória (ex.: `memset`, usado para apagar os LEDs).

2. `#include <stdio.h>`  
   **Utilidade:** Funções de entrada e saída padrão (ex.: `printf`, usado para exibir mensagens no console).

3. `#include <stdlib.h>`  
   **Utilidade:** Funções padrão (ex.: `rand`, usado para gerar padrões aleatórios de cores).

### Bibliotecas do Raspberry Pi Pico

4. `#include "pico/stdlib.h"`  
   **Utilidade:** Contém funções para GPIO, temporização e inicialização do sistema.

5. `#include "hardware/pio.h"`  
   **Utilidade:** Controle do PIO (Programmable Input/Output), usado para manipular os LEDs WS2812.

6. `#include "hardware/dma.h"`  
   **Utilidade:** Controle de DMA (Direct Memory Access), usado para atualizar os LEDs de forma eficiente.

7. `#include "pico/bootrom.h"`  
   **Utilidade:** Funções relacionadas ao bootloader (ex.: `reset_usb_boot` para reiniciar no modo de gravação).

### Biblioteca Personalizada

8. `#include "ws2812.pio.h"`  
   **Utilidade:** Programa PIO e inicialização necessários para controlar os LEDs WS2812.

## Características do Sistema

- **Controle de LEDs:** Os LEDs podem ser acesos em diferentes cores (azul, vermelho, verde, branco) ou exibirem padrões aleatórios.
- **Teclado Matricial:** Um teclado 4x4 é usado para interagir com o sistema.
- **Buzzer:** Um buzzer emite sinais sonoros em determinadas interações.
- **Reinício no modo Bootloader:** Ao pressionar a tecla `*`, o sistema reinicia em modo de gravação.

## Como Usar

1. **Configuração Inicial:** Certifique-se de que os pinos do Raspberry Pi Pico estão conectados corretamente à matriz de LEDs, teclado matricial e buzzer.
2. **Funções do Teclado:**
   - `A`: Apagar todos os LEDs.
   - `B`: Acender todos os LEDs na cor azul.
   - `C`: Acender todos os LEDs na cor vermelha.
   - `D`: Acender todos os LEDs na cor verde.
   - `#`: Acender todos os LEDs na cor branca.
   - `*`: Reiniciar o dispositivo no modo de gravação.
   - `0`: Mostrar um padrão aleatório nos LEDs e emitir um som.
   - `1`: Mostrar uma cobra se moviventando até o ultimo led e explodindo.
   - `3`: Mostrar uma flor crescendo e uma abelha pousando nela.

## Configuração dos Pinos

- **Pino dos LEDs:** `PIN_TX` = 7
- **Teclado Matricial:**
  - Linhas: Pinos 2, 3, 4, 5
  - Colunas: Pinos 6, 10, 8, 9
- **Buzzer:** `BUZZER_PIN` = 21

## Funções Principais

1. **`apagaLEDS`**  
   Apaga todos os LEDs.

2. **`acendeLEDS(cor)`**  
   Acende todos os LEDs com uma cor específica.

3. **`mostraImagemAleatoria`**  
   Exibe padrões aleatórios nos LEDs.

4. **`emiteSom(duracao, frequencia)`**  
   Emite um sinal sonoro com duração e frequência definidas.

5. **`scan_keypad`**  
   Lê o estado do teclado matricial para identificar a tecla pressionada.

6. **`init_gpio`**  
   Configura os pinos para o teclado matricial e buzzer.

## Observações

- Certifique-se de que todas as conexões estejam corretas antes de alimentar o dispositivo.
- Utilize um compilador compatível com o Raspberry Pi Pico para compilar e carregar o programa.
