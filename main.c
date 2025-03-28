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
