#ifndef matriz_H
#define matriz_H

#include <stdint.h>
#include <stdbool.h>

#define PIN_MATRIX 7 // Pino de saída da matriz WS2812

// Define o número de pixels na matriz (5x5)
#define NUM_PIXELS 25

// Função para converter valores RGB (0.0 a 1.0) para um único uint32_t compatível com WS2812
static inline uint32_t matrix_rgb(uint8_t r, uint8_t g, uint8_t b);

// Define a função que configura a matriz de LEDs com base no caractere recebido
void set_character_matrix(char character, bool *led_buffer);

// Função que acende ou apaga os LEDs da matriz
void set_matrix_led(uint8_t r, uint8_t g, uint8_t b, bool *led_buffer);

#endif // matriz_H