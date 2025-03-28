
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
