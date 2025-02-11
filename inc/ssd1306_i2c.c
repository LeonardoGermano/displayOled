#include "ssd1306_i2c.h"
#include "ssd1306_font.h"

// Renomeando as funções com o prefixo "oled_"
void oled_init(ssd1306_t *ssd, uint8_t width, uint8_t height, bool external_vcc, uint8_t address, i2c_inst_t *i2c) {
  ssd->width = width;
  ssd->height = height;
  ssd->pages = height / 8U;
  ssd->address = address;
  ssd->i2c_port = i2c;
  ssd->bufsize = ssd->pages * ssd->width + 1;
  ssd->ram_buffer = calloc(ssd->bufsize, sizeof(uint8_t));
  ssd->ram_buffer[0] = 0x40;
  ssd->port_buffer[0] = 0x80;
}

void oled_config(ssd1306_t *ssd) {
  oled_command(ssd, SET_DISP | 0x00);
  oled_command(ssd, SET_MEM_ADDR);
  oled_command(ssd, 0x01);
  oled_command(ssd, SET_DISP_START_LINE | 0x00);
  oled_command(ssd, SET_SEG_REMAP | 0x01);
  oled_command(ssd, SET_MUX_RATIO);
  oled_command(ssd, HEIGHT - 1);
  oled_command(ssd, SET_COM_OUT_DIR | 0x08);
  oled_command(ssd, SET_DISP_OFFSET);
  oled_command(ssd, 0x00);
  oled_command(ssd, SET_COM_PIN_CFG);
  oled_command(ssd, 0x12);
  oled_command(ssd, SET_DISP_CLK_DIV);
  oled_command(ssd, 0x80);
  oled_command(ssd, SET_PRECHARGE);
  oled_command(ssd, 0xF1);
  oled_command(ssd, SET_VCOM_DESEL);
  oled_command(ssd, 0x30);
  oled_command(ssd, SET_CONTRAST);
  oled_command(ssd, 0xFF);
  oled_command(ssd, SET_ENTIRE_ON);
  oled_command(ssd, SET_NORM_INV);
  oled_command(ssd, SET_CHARGE_PUMP);
  oled_command(ssd, 0x14);
  oled_command(ssd, SET_DISP | 0x01);
}

void oled_command(ssd1306_t *ssd, uint8_t command) {
  ssd->port_buffer[1] = command;
  i2c_write_blocking(
    ssd->i2c_port,
    ssd->address,
    ssd->port_buffer,
    2,
    false
  );
}

void oled_send_data(ssd1306_t *ssd) {
  oled_command(ssd, SET_COL_ADDR);
  oled_command(ssd, 0);
  oled_command(ssd, ssd->width - 1);
  oled_command(ssd, SET_PAGE_ADDR);
  oled_command(ssd, 0);
  oled_command(ssd, ssd->pages - 1);
  i2c_write_blocking(
    ssd->i2c_port,
    ssd->address,
    ssd->ram_buffer,
    ssd->bufsize,
    false
  );
}

void oled_pixel(ssd1306_t *ssd, uint8_t x, uint8_t y, bool value) {
  uint16_t index = (y >> 3) + (x << 3) + 1;
  uint8_t pixel = (y & 0b111);
  if (value)
    ssd->ram_buffer[index] |= (1 << pixel);
  else
    ssd->ram_buffer[index] &= ~(1 << pixel);
}

void oled_fill(ssd1306_t *ssd, bool value) {
    for (uint8_t y = 0; y < ssd->height; ++y) {
        for (uint8_t x = 0; x < ssd->width; ++x) {
            oled_pixel(ssd, x, y, value);
        }
    }
}

void oled_rect(ssd1306_t *ssd, uint8_t top, uint8_t left, uint8_t width, uint8_t height, bool value, bool fill) {
  for (uint8_t x = left; x < left + width; ++x) {
    oled_pixel(ssd, x, top, value);
    oled_pixel(ssd, x, top + height - 1, value);
  }
  for (uint8_t y = top; y < top + height; ++y) {
    oled_pixel(ssd, left, y, value);
    oled_pixel(ssd, left + width - 1, y, value);
  }

  if (fill) {
    for (uint8_t x = left + 1; x < left + width - 1; ++x) {
      for (uint8_t y = top + 1; y < top + height - 1; ++y) {
        oled_pixel(ssd, x, y, value);
      }
    }
  }
}

void oled_line(ssd1306_t *ssd, uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, bool value) {
    int dx = abs(x1 - x0);
    int dy = abs(y1 - y0);

    int sx = (x0 < x1) ? 1 : -1;
    int sy = (y0 < y1) ? 1 : -1;

    int err = dx - dy;

    while (true) {
        oled_pixel(ssd, x0, y0, value);
        if (x0 == x1 && y0 == y1) break;

        int e2 = err * 2;

        if (e2 > -dy) {
            err -= dy;
            x0 += sx;
        }

        if (e2 < dx) {
            err += dx;
            y0 += sy;
        }
    }
}

void oled_hline(ssd1306_t *ssd, uint8_t x0, uint8_t x1, uint8_t y, bool value) {
  for (uint8_t x = x0; x <= x1; ++x)
    oled_pixel(ssd, x, y, value);
}

void oled_vline(ssd1306_t *ssd, uint8_t x, uint8_t y0, uint8_t y1, bool value) {
  for (uint8_t y = y0; y <= y1; ++y)
    oled_pixel(ssd, x, y, value);
}

void oled_draw_char(ssd1306_t *ssd, char c, uint8_t x, uint8_t y) {
  uint16_t index = 0;
  if (c >= 'A' && c <= 'Z') {
    index = (c - 'A' + 11) * 8;
  } else if (c >= '0' && c <= '9') {
    index = (c - '0' + 1) * 8;
  } else if (c >= 'a' && c <= 'z') {
    index = (c - 'a' + 37) * 8;
  }

  for (uint8_t i = 0; i < 8; ++i) {
    uint8_t line = font[index + i];
    for (uint8_t j = 0; j < 8; ++j) {
      oled_pixel(ssd, x + i, y + j, line & (1 << j));
    }
  }
}

void oled_draw_string(ssd1306_t *ssd, const char *str, uint8_t x, uint8_t y) {
  while (*str) {
    oled_draw_char(ssd, *str++, x, y);
    x += 8;
    if (x + 8 >= ssd->width) {
      x = 0;
      y += 8;
    }
    if (y + 8 >= ssd->height) {
      break;
    }
  }
}