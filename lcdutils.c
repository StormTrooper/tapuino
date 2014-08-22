#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <util/delay.h>
#include "tapuino.h"
#include "ff.h"
#include "config.h"
#include "lcd.h"
#include "lcdutils.h"
#include "memstrings.h"

// right arrow character 
#define DIRECTORY_INDICATOR 0b01111110

char g_char_buffer[MAX_LCD_LINE_LEN + 1] = {0};
static uint8_t g_ticker_enabled  = 0;
static uint8_t g_ticker_index = 0;
static uint32_t g_last_tick = 0;

uint8_t backslashChar[8] = {
    0b00000,
    0b10000,
    0b01000,
    0b00100,
    0b00010,
    0b00001,
    0b00000,
    0b00000
};

void filename_ticker(FILINFO* pfile_info, uint32_t cur_tick) {
  
  static uint32_t last_hold = 0;
  char* ticker_string;

  if (g_ticker_enabled) {
    if (!g_last_tick) g_last_tick = cur_tick;
    
    // how often do we tick?
    if (cur_tick - g_last_tick < (uint32_t) g_ticker_rate) {
      return;
    }
    g_last_tick = cur_tick;

    if (!last_hold) last_hold = cur_tick;
    // how long do we hold?
    if (cur_tick - last_hold < (uint32_t) g_ticker_hold) {
      return;
    }

    ticker_string = pfile_info->lfname[0] ? pfile_info->lfname : pfile_info->fname;
    // is the filename within screen bounds?
    if ((strlen(ticker_string) - g_ticker_index) <= MAX_LCD_LINE_LEN) {
      // how long do we hold at the end?
      if (cur_tick - last_hold < (uint32_t) (g_ticker_hold << 1)) {
        return;
      }
      g_ticker_index = 0;
      g_last_tick = last_hold = 0;
    } else {
      //reset to avoid overflow
      last_hold = cur_tick - g_ticker_hold - 1;
      g_ticker_index++;
    }
    
    lcd_status(&ticker_string[g_ticker_index]);
    if (pfile_info->fattrib & AM_DIR) {
      lcd_show_dir();
    }  
  }
}

void display_filename(FILINFO* pfile_info) {
  char* ticker_string;
  
  ticker_string = pfile_info->lfname[0] ? pfile_info->lfname : pfile_info->fname;
  lcd_status(ticker_string);
  if (pfile_info->fattrib & AM_DIR) {
    lcd_show_dir();
  }  
  g_ticker_enabled = strlen(ticker_string) > (MAX_LCD_LINE_LEN - 1);
  g_ticker_index = 0;
}

void lcd_spinner_internal(uint32_t cur_tick, int8_t perc, uint16_t rate) {
  static uint8_t indicators[] = {'|', '/', '-', 1};
  static uint8_t pos = 0;
  if (cur_tick - g_last_tick < rate) {
    return;
  }
  
  g_last_tick = cur_tick;
  lcd_setCursor(MAX_LCD_LINE_LEN - 7, 0);
  if (perc < 0) {
    strncpy_P(g_char_buffer, S_MAX_BLANK_LINE, 7);
  } else {
    sprintf(g_char_buffer, "%3d%%   ", perc);
  }
  g_char_buffer[5] = MOTOR_IS_OFF() ? 'm' : 'M';
  g_char_buffer[6] = indicators[pos++];
  g_char_buffer[7] = 0;
  lcd_print(g_char_buffer);

  if (pos > 3) {
    pos = 0;
  }
}

void lcd_spinner(uint32_t cur_tick, int8_t perc) {
  lcd_spinner_internal(cur_tick, perc, g_ticker_rate);
}

void lcd_busy_spinner() {
  uint8_t i;
  for (i = 0; i < 100; i++) {
    lcd_spinner_internal(i, -1, 0);
    _delay_ms(20);
  }
}

void lcd_show_dir() {
  lcd_setCursor(MAX_LCD_LINE_LEN - 1, 1);
  lcd_write(DIRECTORY_INDICATOR);
}

void lcd_line(char* msg, int line, uint8_t usepgm) {
  int len;
  strncpy_P(g_char_buffer, S_MAX_BLANK_LINE, MAX_LCD_LINE_LEN);
  
  lcd_setCursor(0, line);
  if (usepgm) {
    len = strlen_P(msg);
    memcpy_P(g_char_buffer, msg, len > MAX_LCD_LINE_LEN ? MAX_LCD_LINE_LEN : len);
  } else {
    len = strlen(msg);
    memcpy(g_char_buffer, msg, len > MAX_LCD_LINE_LEN ? MAX_LCD_LINE_LEN : len);
  }
  lcd_print(g_char_buffer);
}

void lcd_title(char* msg) {
  lcd_line(msg, 0, 0);
}

void lcd_title_P(const char* msg) {
  lcd_line((char*)msg, 0, 1);
}

void lcd_status(char* msg) {
  lcd_line(msg, 1, 0);
}

void lcd_status_P(const char* msg) {
  lcd_line((char*)msg, 1, 1);
}

void lcd_setup() {
  lcd_begin(LCD_I2C_ADDR, MAX_LCD_LINE_LEN, LCD_NUM_LINES, LCD_5x8DOTS);
  lcd_backlight();
  // can't define this as the zeroth character as zero is null in strings :)! :)
  lcd_createChar(1, backslashChar);
}
