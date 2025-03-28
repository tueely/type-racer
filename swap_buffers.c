void swap_buffers() {
  while (*(volatile uint32_t*)STATUS_REG & 0x01);
  *(volatile uint32_t*)(FB_CONTROLLER + 4) = (uint32_t)back_buffer;
  *(volatile uint32_t*)FB_CONTROLLER = 1;
}

void clear_backbuffer() {
  for (int y = 0; y < FB_HEIGHT; y++)
    for (int x = 0; x < FB_WIDTH; x++) back_buffer[y][x] = COLOR_BLACK;
}
