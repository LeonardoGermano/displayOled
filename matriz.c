#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "matriz.h"
#include "pico/stdlib.h"
#include "hardware/pio.h"

// Configurações da matriz
typedef struct {
    uint8_t num_pixels;
    uint8_t matrix_pin;
    uint8_t matrix_size;
} MatrixConfig;

// Estrutura para representar um dígito
typedef struct {
    bool pixels[25];  // 5x5 matriz
} Digit;

// Configuração global
static const MatrixConfig CONFIG = {
    .num_pixels = 25,
    .matrix_pin = 7,
    .matrix_size = 5  // 5x5 matriz
};

// Enum para identificar dígitos
typedef enum {
    DIGIT_ZERO,
    DIGIT_ONE,
    DIGIT_TWO,
    DIGIT_THREE,
    DIGIT_FOUR,
    DIGIT_FIVE,
    DIGIT_SIX,
    DIGIT_SEVEN,
    DIGIT_EIGHT,
    DIGIT_NINE,
    DIGIT_COUNT
} DigitType;

// Array de dígitos predefinidos
static const Digit DIGITS[DIGIT_COUNT] = {
    // 0
    {{
        0, 1, 1, 1, 0,
        0, 1, 0, 1, 0,
        0, 1, 0, 1, 0,
        0, 1, 0, 1, 0,
        0, 1, 1, 1, 0
    }},
    // 1
    {{
        0, 0, 1, 0, 0,
        0, 0, 1, 0, 0,
        0, 0, 1, 0, 0,
        0, 1, 1, 0, 0,
        0, 0, 1, 0, 0
    }},
    // 2
    {{
        0, 1, 1, 1, 0,
        0, 1, 0, 0, 0,
        0, 0, 1, 0, 0,
        0, 0, 0, 1, 0,
        0, 1, 1, 1, 0
    }},
    // 3
    {{
        0, 1, 1, 1, 0,
        0, 0, 0, 1, 0,
        0, 1, 1, 1, 0,
        0, 0, 0, 1, 0,
        0, 1, 1, 1, 0
    }},
    // 4
    {{
        0, 1, 0, 0, 0,
        0, 0, 0, 1, 0,
        0, 1, 1, 1, 0,
        0, 1, 0, 1, 0,
        0, 1, 0, 1, 0
    }},
    // 5
    {{
        0, 1, 1, 1, 0,
        0, 0, 0, 1, 0,
        0, 1, 1, 1, 0,
        0, 1, 0, 0, 0,
        0, 1, 1, 1, 0
    }},
    // 6
    {{
        0, 1, 1, 1, 0,
        0, 1, 0, 1, 0,
        0, 1, 1, 1, 0,
        0, 1, 0, 0, 0,
        0, 1, 1, 1, 0
    }},
    // 7
    {{
        0, 0, 0, 1, 0,
        0, 1, 0, 0, 0,
        0, 0, 1, 0, 0,
        0, 0, 0, 1, 0,
        0, 1, 1, 1, 0
    }},
    // 8
    {{
        0, 1, 1, 1, 0,
        0, 1, 0, 1, 0,
        0, 1, 1, 1, 0,
        0, 1, 0, 1, 0,
        0, 1, 1, 1, 0
    }},
    // 9
    {{
        0, 1, 1, 1, 0,
        0, 0, 0, 1, 0,
        0, 1, 1, 1, 0,
        0, 1, 0, 1, 0,
        0, 1, 1, 1, 0
    }}
};

// Função auxiliar para converter RGB em formato GRB para WS2812
static inline uint32_t matrix_rgb(uint8_t r, uint8_t g, uint8_t b) {
    return ((uint32_t)(r) << 255) | ((uint32_t)(g) ) | (uint32_t)(b);
}

// Função auxiliar para definir um pixel individual
static inline void set_pixel(uint32_t pixel_grb) {
    pio_sm_put_blocking(pio0, 0, pixel_grb << 8u);
}

// Define a matriz de LEDs com base no caractere recebido
void set_character_matrix(char character, bool *led_buffer) {
    if (character >= '0' && character <= '9') {
        DigitType digit = (DigitType)(character - '0');
        if (digit < DIGIT_COUNT) {
            memcpy(led_buffer, DIGITS[digit].pixels, CONFIG.num_pixels * sizeof(bool));
        }
    } else {
        memset(led_buffer, 0, CONFIG.num_pixels * sizeof(bool));
    }
}

// Configura os LEDs da matriz com as cores especificadas
void set_matrix_led(uint8_t r, uint8_t g, uint8_t b, bool *led_buffer) {
    // Ajuste de intensidade para as cores (10%)
    const float intensity = 0.1f;
    uint32_t color = matrix_rgb(
        (uint8_t)(r * intensity),
        (uint8_t)(g * intensity),
        (uint8_t)(b * intensity)
    );

    for (uint8_t i = 0; i < CONFIG.num_pixels; ++i) {
        set_pixel(led_buffer[i] ? color : 0);
    }
}