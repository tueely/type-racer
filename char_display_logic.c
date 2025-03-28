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
