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

#define COLOR_WHITE 0xFFFF
#define COLOR_GREEN 0x07E0
#define COLOR_RED 0xF800
#define COLOR_BLACK 0x0000
