
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
