#include <stdint.h>
#include <stdio.h>  // Added for snprintf
#include <stdlib.h>
#include <string.h>

#define VGA_BASE 0x08000000
#define FB_CONTROLLER 0xFF203020
#define STATUS_REG 0xFF20302C
#define PS2_DATA 0xFF200100
#define PS2_CTRL (volatile uint32_t*)0xFF200100

#define FB_WIDTH 320
#define FB_HEIGHT 240
#define FB_STRIDE 512
#define CHAR_SIZE 8

// Global variables moved to top
float fall_speed = 1.0;
int spawn_counter = 0;
const int SPAWN_DELAY = 45;

volatile uint16_t* front_buffer = (uint16_t*)VGA_BASE;
uint16_t back_buffer[FB_HEIGHT][FB_STRIDE] __attribute__((aligned(512)));

struct Letter {
  int x, y;
  char c;
  int active;
};
struct Letter letters[10];
int score = 0;
int game_over = 0;
int game_started = 0;

const uint8_t font[27][8] = {
    {0x3E, 0x63, 0x63, 0x7F, 0x63, 0x63, 0x63, 0x00},  // A
    {0x7E, 0x63, 0x63, 0x7E, 0x63, 0x63, 0x7E, 0x00},  // B
    {0x3E, 0x63, 0x60, 0x60, 0x60, 0x63, 0x3E, 0x00},  // C
    {0x7E, 0x63, 0x63, 0x63, 0x63, 0x63, 0x7E, 0x00},  // D
    {0x7F, 0x60, 0x60, 0x7E, 0x60, 0x60, 0x7F, 0x00},  // E
    {0x3F, 0x60, 0x60, 0x3E, 0x03, 0x03, 0x7E, 0x00},  // S
    {0x7E, 0x63, 0x63, 0x7E, 0x6C, 0x66, 0x63, 0x00},  // R
    {0x3E, 0x63, 0x63, 0x63, 0x63, 0x63, 0x3E, 0x00},  // O
    {0x7F, 0x63, 0x63, 0x7F, 0x60, 0x60, 0x60, 0x00},  // P
    {0x3E, 0x63, 0x60, 0x60, 0x67, 0x63, 0x3F, 0x00},  // G
    {0x63, 0x77, 0x7F, 0x6B, 0x63, 0x63, 0x63, 0x00},  // M
    {0x63, 0x63, 0x63, 0x63, 0x36, 0x1C, 0x08, 0x00},  // V
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},  // Space
    {0x3E, 0x63, 0x63, 0x63, 0x63, 0x63, 0x3E, 0x00},  // 0
    {0x0C, 0x1C, 0x0C, 0x0C, 0x0C, 0x0C, 0x3F, 0x00},  // 1
    {0x3E, 0x63, 0x03, 0x0E, 0x38, 0x60, 0x7F, 0x00},  // 2
    {0x3E, 0x63, 0x03, 0x1E, 0x03, 0x63, 0x3E, 0x00},  // 3
    {0x06, 0x0E, 0x1E, 0x36, 0x66, 0x7F, 0x06, 0x00},  // 4
    {0x7F, 0x60, 0x7E, 0x03, 0x03, 0x63, 0x3E, 0x00},  // 5
    {0x1E, 0x30, 0x60, 0x7E, 0x63, 0x63, 0x3E, 0x00},  // 6
    {0x7F, 0x63, 0x06, 0x0C, 0x18, 0x18, 0x18, 0x00},  // 7
    {0x3E, 0x63, 0x63, 0x3E, 0x63, 0x63, 0x3E, 0x00},  // 8
    {0x3E, 0x63, 0x63, 0x3F, 0x03, 0x06, 0x3C, 0x00},  // 9
    {0xFF, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x00},  // T
    {0x7F, 0x60, 0x60, 0x7E, 0x60, 0x60, 0x60, 0x00},  // F
    {0x63, 0x63, 0x63, 0x7F, 0x63, 0x63, 0x63, 0x00},  // H
    {0x3E, 0x06, 0x06, 0x06, 0x06, 0x66, 0x3C, 0x00}   // J
};

#define COLOR_WHITE 0xFFFF
#define COLOR_GREEN 0x07E0
#define COLOR_RED 0xF800
#define COLOR_BLACK 0x0000

void swap_buffers() {
  while (*(volatile uint32_t*)STATUS_REG & 0x01);
  *(volatile uint32_t*)(FB_CONTROLLER + 4) = (uint32_t)back_buffer;
  *(volatile uint32_t*)FB_CONTROLLER = 1;
}

void clear_backbuffer() {
  for (int y = 0; y < FB_HEIGHT; y++)
    for (int x = 0; x < FB_WIDTH; x++) back_buffer[y][x] = COLOR_BLACK;
}

void draw_char(int x, int y, char c, uint16_t color) {
  if (x < 0 || x >= FB_WIDTH - CHAR_SIZE || y < 0 || y >= FB_HEIGHT - CHAR_SIZE)
    return;

  int idx = -1;
  if (c >= 'A' && c <= 'E')
    idx = c - 'A';
  else if (c == 'S')
    idx = 5;
  else if (c == 'R')
    idx = 6;
  else if (c == 'O')
    idx = 7;
  else if (c == 'P')
    idx = 8;
  else if (c == 'G')
    idx = 9;
  else if (c == 'M')
    idx = 10;
  else if (c == 'V')
    idx = 11;
  else if (c == ' ')
    idx = 12;
  else if (c >= '0' && c <= '9')
    idx = 13 + (c - '0');
  else if (c == 'T')
    idx = 23;
  else if (c == 'F')
    idx = 24;
  else if (c == 'H')
    idx = 25;
  else if (c == 'J')
    idx = 26;
  else
    return;

  for (int dy = 0; dy < CHAR_SIZE; dy++) {
    uint8_t row = font[idx][dy];
    for (int dx = 0; dx < CHAR_SIZE; dx++) {
      if (row & (0x80 >> dx)) {
        back_buffer[y + dy][x + dx] = color;
      }
    }
  }
}

void spawn_letter() {
  static char last_char = '\0';

  // Combined vertical spacing check
  for (int j = 0; j < 10; j++) {
    if (letters[j].active && letters[j].y < (FB_HEIGHT / 4)) {
      return;
    }
  }

  for (int i = 0; i < 10; i++) {
    if (!letters[i].active) {
      int attempts = 0;
      char new_char;
      do {
        new_char = "FGHJ"[rand() % 4];
        attempts++;
      } while (new_char == last_char && attempts < 10);
      last_char = new_char;

      int valid_pos = 0;
      int new_x;
      for (int tries = 0; tries < 10; tries++) {
        new_x = rand() % (FB_WIDTH - CHAR_SIZE);
        valid_pos = 1;
        for (int j = 0; j < 10; j++) {
          if (letters[j].active && abs(new_x - letters[j].x) < CHAR_SIZE * 2 &&
              letters[j].y < CHAR_SIZE * 2) {
            valid_pos = 0;
            break;
          }
        }
        if (valid_pos) break;
      }

      if (!valid_pos) return;

      letters[i].x = new_x;
      letters[i].y = 0;
      letters[i].c = new_char;
      letters[i].active = 1;
      break;
    }
  }
}

void update_letters() {
  for (int i = 0; i < 10; i++) {
    if (letters[i].active) {
      letters[i].y += (int)fall_speed;
      if (letters[i].y >= FB_HEIGHT - CHAR_SIZE) game_over = 1;
    }
  }
  fall_speed += 0.002;
}

char get_key() {
  static int release_next = 0;
  static uint8_t current_key = 0;

  if ((*PS2_CTRL & 0x8000) == 0) return 0;
  uint8_t scan_code = *PS2_CTRL & 0xFF;

  if (release_next) {
    release_next = 0;
    current_key = 0;
    return 0;
  }

  if (scan_code == 0xF0) {
    release_next = 1;
    return 0;
  }

  if (current_key == scan_code) return 0;

  current_key = scan_code;

  switch (scan_code) {
    case 0x2B:
      return 'F';
    case 0x34:
      return 'G';
    case 0x33:
      return 'H';
    case 0x3B:
      return 'J';
    case 0x29:
      return ' ';
    default:
      return 0;
  }
}

void check_collision(char key) {
  if (!key || game_over) return;

  int lowest_y = -1;
  // Find lowest active letter
  for (int i = 0; i < 10; i++) {
    if (letters[i].active && letters[i].y > lowest_y) {
      lowest_y = letters[i].y;
    }
  }

  if (lowest_y == -1) return;

  // Check all letters at lowest y
  int valid = 0;
  for (int i = 0; i < 10; i++) {
    if (letters[i].active && letters[i].y == lowest_y) {
      if (letters[i].c == key) {
        letters[i].active = 0;
        score++;
        valid = 1;
      }
    }
  }

  if (!valid) game_over = 1;
}

void draw_score() {
  char str[16];
  snprintf(str, sizeof(str), "SCORE: %04d", score);
  for (int i = 0; str[i]; i++) draw_char(i * CHAR_SIZE, 0, str[i], COLOR_WHITE);
}

int main() {
  *(volatile uint32_t*)(FB_CONTROLLER + 4) = (uint32_t)back_buffer;
  srand(42);

  while (1) {
    clear_backbuffer();

    if (!game_started) {
      char* msg = "PRESS SPACE TO START";
      int len = strlen(msg);
      int x = (FB_WIDTH - len * CHAR_SIZE) / 2;
      int y = FB_HEIGHT / 2;
      for (int i = 0; i < len; i++)
        draw_char(x + i * CHAR_SIZE, y, msg[i], COLOR_WHITE);

      if (get_key() == ' ') {
        game_over = 0;
        game_started = 1;
        score = 0;
        fall_speed = 1.0;  // Reset speed
        memset(letters, 0, sizeof(letters));
      }
    } else if (!game_over) {
      // Fixed spawning logic
      spawn_counter++;
      if (spawn_counter >= SPAWN_DELAY) {
        spawn_letter();
        spawn_counter = 0;
      }

      check_collision(get_key());
      update_letters();

      // Removed redundant spawn_letter call
      for (int i = 0; i < 10; i++)
        if (letters[i].active)
          draw_char(letters[i].x, letters[i].y, letters[i].c, COLOR_GREEN);

      draw_score();
    } else {
      char* msg = "GAME OVER";
      int len = strlen(msg);
      int x = (FB_WIDTH - len * CHAR_SIZE) / 2;
      int y = FB_HEIGHT / 2;
      for (int i = 0; i < len; i++)
        draw_char(x + i * CHAR_SIZE, y, msg[i], COLOR_RED);

      char* msg2 = "PRESS SPACE TO RESTART";
      int len2 = strlen(msg2);
      int x2 = (FB_WIDTH - len2 * CHAR_SIZE) / 2;
      int y2 = y + CHAR_SIZE;
      for (int i = 0; i < len2; i++)
        draw_char(x2 + i * CHAR_SIZE, y2, msg2[i], COLOR_RED);

      if (get_key() == ' ') {
        game_over = 0;
        game_started = 1;
        score = 0;
        fall_speed = 1.0;  // Reset speed
        memset(letters, 0, sizeof(letters));
      }
    }

    swap_buffers();
    for (volatile int i = 0; i < 50000; i++);
  }
  return 0;
}