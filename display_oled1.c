#include <stdlib.h>
#include <stdio.h>
#include <pico/multicore.h>
#include <ctype.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "inc/ssd1306_i2c.h"
#include "inc/ssd1306_font.h"
#include "matriz.h"
#include "hardware/clocks.h"

// Arquivo de definição .pio
#include "matrizled.pio.h"

// Pinos e configurações de hardware
#define I2C_PORT i2c1
#define I2C_SDA 14
#define I2C_SCL 15
#define ENDERECO 0x3C // Endereço do controlador SSD1306

#define LED_RED 13  // LED vermelho conectado neste pino
#define LED_GREEN 11  // LED verde
#define LED_BLUE 12  // LED azul

#define RED 191  // Intensidade para a cor vermelha
#define GREEN 0  // Intensidade para a cor verde
#define BLUE 191 // Intensidade para a cor azul

#define BUTTON_A 5  // Pino do botão A
#define BUTTON_B 6  // Pino do botão B

#define DEBOUNCE_TIME_US 200000  // Tempo de debounce de 200ms
#define TIME 100  // Intervalo de atualização

// Variáveis que controlam o estado dos LEDs e botões
static char character;  // Caractere digitado pelo usuário
bool led_buffer[NUM_PIXELS] = {0};  // Estado atual da matriz de LEDs

static volatile uint32_t last_press_A = 0;  // Marca o momento do último acionamento do botão A
static volatile uint32_t last_press_B = 0;  // Marca o momento do último acionamento do botão B

static volatile uint8_t blue = 0;  // Estado atual do LED azul
static volatile uint8_t green = 0;  // Estado atual do LED verde

uint32_t last_time = 0;  // Usado para medir intervalos de tempo

// Configuração inicial dos pinos
void setup() {
    // Configuração do botão A
    gpio_init(BUTTON_A);
    gpio_set_dir(BUTTON_A, GPIO_IN);  // Define o botão como entrada
    gpio_pull_up(BUTTON_A);  // Ativa o resistor pull-up interno

    // Configuração do botão B
    gpio_init(BUTTON_B);
    gpio_set_dir(BUTTON_B, GPIO_IN);  // Define o botão como entrada
    gpio_pull_up(BUTTON_B);  // Ativa o resistor pull-up interno
    
    // Configuração dos LEDs RGB
    gpio_init(LED_RED);
    gpio_set_dir(LED_RED, GPIO_OUT);  // Define LED vermelho como saída
    gpio_put(LED_RED, 0);  // Inicialmente apagado

    gpio_init(LED_GREEN);
    gpio_set_dir(LED_GREEN, GPIO_OUT);  // Define LED verde como saída
    gpio_put(LED_GREEN, 0);  // Inicialmente apagado

    gpio_init(LED_BLUE);
    gpio_set_dir(LED_BLUE, GPIO_OUT);  // Define LED azul como saída
    gpio_put(LED_BLUE, 0);  // Inicialmente apagado
}

// Configuração do barramento I2C
void I2C_init() {
    i2c_init(I2C_PORT, 400 * 1000);  // Configura o I2C para operar a 400 kHz
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);  // Define a função I2C para o pino SDA
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);  // Define a função I2C para o pino SCL
    gpio_pull_up(I2C_SDA);  // Habilita resistor pull-up no pino SDA
    gpio_pull_up(I2C_SCL);  // Habilita resistor pull-up no pino SCL
}

// Inicialização do display SSD1306
void SSD1306_init(ssd1306_t* ssd) {
    oled_init(ssd, WIDTH, HEIGHT, false, ENDERECO, I2C_PORT);  // Configura o display
    oled_config(ssd);  // Aplica as configurações ao display
    oled_send_data(ssd);  // Envia os dados para atualizar o estado do display
    oled_fill(ssd, false);  // Limpa a tela do display
    oled_send_data(ssd);  // Atualiza a exibição após limpar
}

// Leitura de caracteres via entrada padrão
void read_caracter() {
    while (true) {
        int ch = getchar();  // Obtém um caractere do terminal
        if (ch != EOF) {  // Verifica se o caractere é válido
            character = (char)ch;  // Salva o caractere lido
        }
        sleep_ms(500);  // Aguarda 500 ms antes de tentar novamente
    }
}

// Gerenciador de interrupções para os botões
static void button_handler(uint gpio, uint32_t events) {
    uint32_t current_time = to_us_since_boot(get_absolute_time());  // Tempo atual em microssegundos

    // Interrupção do botão A
    if (gpio == BUTTON_A && (events & GPIO_IRQ_EDGE_FALL)) {
        if (current_time - last_press_A > DEBOUNCE_TIME_US) {  // Valida o tempo de debounce
            last_press_A = current_time;  // Atualiza o último momento em que o botão foi pressionado
            if (green == 0) {
                green = 1;
                gpio_put(LED_GREEN, 1);  // Liga o LED verde
            } else {
                green = 0;
                gpio_put(LED_GREEN, 0);  // Desliga o LED verde
            }
        }
    } 
    // Interrupção do botão B
    else if (gpio == BUTTON_B && (events & GPIO_IRQ_EDGE_FALL)) {
        if (current_time - last_press_B > DEBOUNCE_TIME_US) {  // Valida o tempo de debounce
            last_press_B = current_time;  // Atualiza o último momento em que o botão foi pressionado
            if (blue == 0) {
                blue = 1;
                gpio_put(LED_BLUE, 1);  // Liga o LED azul
            } else {
                blue = 0;
                gpio_put(LED_BLUE, 0);  // Desliga o LED azul
            }
        }
    }
}

// Função principal
int main() {
    PIO pio = pio0;
    int sm = 0;
    ssd1306_t ssd;
    bool cor = true;

    stdio_init_all();  // Inicializa a comunicação via USB
    setup();  // Executa a configuração inicial dos periféricos
    I2C_init();  // Configura a interface I2C
    SSD1306_init(&ssd);  // Configura o display OLED SSD1306

    multicore_launch_core1(read_caracter);  // Executa a leitura de caracteres no segundo núcleo

    uint offset = pio_add_program(pio, &matrizled_program);  // Adiciona o programa de controle da matriz
    matrizled_program_init(pio, sm, offset, PIN_MATRIX);  // Configura a matriz de LEDs

    // Habilita interrupções para os botões
    gpio_set_irq_enabled_with_callback(BUTTON_A, GPIO_IRQ_EDGE_FALL, true, &button_handler);
    gpio_set_irq_enabled_with_callback(BUTTON_B, GPIO_IRQ_EDGE_FALL, true, &button_handler);

    while (true) {
        // Limpa e alterna a exibição no display
        oled_fill(&ssd, !cor);

        // Atualiza o display com o caractere lido
        oled_draw_string(&ssd, "Caractere", 12, 10); 
        oled_draw_string(&ssd, "Digitado", 12, 20); 
        oled_draw_string(&ssd, (char[]){character, '\0'}, 99, 15);  
        oled_line(&ssd, 98, 25, 109, 25, 1);
        
        // Atualiza o status dos LEDs no display
        if (green == 0) {
            oled_draw_string(&ssd, "Led Verde  off ", 12, 35); 
            printf("Status do LED verde: desligado\n");
        } else {
            oled_draw_string(&ssd, "Led Verde  on ", 12, 35); 
            printf("Status do LED verde: ligado\n");
        }

        if (blue == 0) {
            oled_draw_string(&ssd, "Led Azul   off ", 12, 45); 
            printf("Status do LED azul: desligado\n");
        } else {
            oled_draw_string(&ssd, "Led Azul   on ", 12, 45); 
            printf("Status do LED azul: ligado\n");
        }

        // Envia dados para o display
        oled_send_data(&ssd);

        // Atualiza a matriz de LEDs com base no caractere
        set_character_matrix(character, led_buffer);  
        set_matrix_led(RED, GREEN, BLUE, led_buffer);  

        sleep_ms(1000);  // Aguarda antes de atualizar novamente
    }
}