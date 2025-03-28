#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//Memory-mapped hardware register addresses
#define VGA_BASE 0x08000000 //the control register for a frame buffer
#define FB_CONTROLLER 0xFF203020
#define STATUS_REG 0xFF20302C
#define PS2_DATA 0xFF200100 //https://ecse324.ece.mcgill.ca/fall2021/labs/lab3/lab3.html
#define PS2_CTRL (volatile uint32_t*)0xFF200100
//uint32_t indicates we are dealing with a 32-bit unsigned integer. This aligns with typical bus widths
//this is efficient typecasting

#define FB_WIDTH 320
#define FB_HEIGHT 240
#define FB_STRIDE 512 // 512 is for alignment the hardware video DMA or controller block is configured to fetch 512 halfwords per line
#define CHAR_SIZE 8
#define NUM_STARS 150

// Color definitions
#define COLOR_WHITE 0xFFFF
#define COLOR_BLACK 0x0000
#define COLOR_RED 0xF800
#define COLOR_GREEN 0x07E0
#define COLOR_BLUE 0x001F
#define COLOR_CYAN 0x07FF
#define COLOR_YELLOW 0xFFE0
#define COLOR_ORANGE 0xFD20
#define COLOR_PURPLE 0xF81F
#define COLOR_PINK 0xFE19
#define COLOR_GREY 0x8410
#define COLOR_DARK_BLUE 0x000F
#define COLOR_DARK_GREY 0x2104

// Double buffering setup
/*
A front buffer that the video hardware is currently reading from to display the image on screen.
A back buffer that the CPU (software) writes to in the background (off-screen), so that the user doesn’t see the image being drawn in stages.
*/

uint16_t buffer0[FB_HEIGHT * FB_STRIDE] __attribute__((aligned(512)));
uint16_t buffer1[FB_HEIGHT * FB_STRIDE] __attribute__((aligned(512)));
//__attribute__((aligned(512)) ensures that the starting address of each array is aligned on a 512-byte boundary

volatile uint16_t* front_buffer = (uint16_t*)buffer0;
volatile uint16_t* back_buffer = (uint16_t*)buffer1;

//voltaire because we dont want unexpected change due to optimization

struct Letter {
    int x, y;
    char c; //the displayed character itself
    int active; //whether they are supposed to be displayed
    uint16_t color;
};

struct Star {
    int x, y;
    uint16_t color;
    uint8_t speed; //big enough number for speed
};

// Game state
struct Letter letters[10];
//As the game runs, you spawn letters into this array, update their positions (falling), and check collisions. size 10 by design
struct Star stars[NUM_STARS];
float fall_speed = 1.0;
int spawn_counter = 0;
//A counter that tracks how many frames (or iterations) have passed since the last letter was spawned. Once it hits SPAWN_DELAY, a new letter is spawned, and this counter resets.
const int SPAWN_DELAY = 40;
int score = 0;
int game_over = 0;
//A flag indicating whether the game is over (1 for over, 0 for running). When game_over is set, the game displays the “Game Over” message and waits for a restart.
int game_started = 0;
//A flag indicating if the game is currently in progress (1) or if it’s at the initial “start screen” (0). When set to 1, the main gameplay loop runs; otherwise, the game shows the “Press Space to Start” screen.

const uint8_t font[29][8] = {
    {0x3E, 0x63, 0x63, 0x7F, 0x63, 0x63, 0x63, 0x00}, // A
    {0x7E, 0x63, 0x63, 0x7E, 0x63, 0x63, 0x7E, 0x00}, // B
    {0x3E, 0x63, 0x60, 0x60, 0x60, 0x63, 0x3E, 0x00}, // C
    {0x7E, 0x63, 0x63, 0x63, 0x63, 0x63, 0x7E, 0x00}, // D
    {0x7F, 0x60, 0x60, 0x7E, 0x60, 0x60, 0x7F, 0x00}, // E
    {0x3F, 0x60, 0x60, 0x3E, 0x03, 0x03, 0x7E, 0x00}, // S
    {0x7E, 0x63, 0x63, 0x7E, 0x6C, 0x66, 0x63, 0x00}, // R
    {0x3E, 0x63, 0x63, 0x63, 0x63, 0x63, 0x3E, 0x00}, // O
    {0x7F, 0x63, 0x63, 0x7F, 0x60, 0x60, 0x60, 0x00}, // P
    {0x3E, 0x63, 0x60, 0x60, 0x67, 0x63, 0x3F, 0x00}, // G
    {0x63, 0x77, 0x7F, 0x6B, 0x63, 0x63, 0x63, 0x00}, // M
    {0x63, 0x63, 0x63, 0x63, 0x36, 0x1C, 0x08, 0x00}, // V
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // Space
    {0x3E, 0x63, 0x63, 0x63, 0x63, 0x63, 0x3E, 0x00}, // 0
    {0x0C, 0x1C, 0x0C, 0x0C, 0x0C, 0x0C, 0x3F, 0x00}, // 1
    {0x3E, 0x63, 0x03, 0x0E, 0x38, 0x60, 0x7F, 0x00}, // 2
    {0x3E, 0x63, 0x03, 0x1E, 0x03, 0x63, 0x3E, 0x00}, // 3
    {0x06, 0x0E, 0x1E, 0x36, 0x66, 0x7F, 0x06, 0x00}, // 4
    {0x7F, 0x60, 0x7E, 0x03, 0x03, 0x63, 0x3E, 0x00}, // 5
    {0x1E, 0x30, 0x60, 0x7E, 0x63, 0x63, 0x3E, 0x00}, // 6
    {0x7F, 0x63, 0x06, 0x0C, 0x18, 0x18, 0x18, 0x00}, // 7
    {0x3E, 0x63, 0x63, 0x3E, 0x63, 0x63, 0x3E, 0x00}, // 8
    {0x3E, 0x63, 0x63, 0x3F, 0x03, 0x06, 0x3C, 0x00}, // 9
    {0xFF, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x00}, // T
    {0x7F, 0x60, 0x60, 0x7E, 0x60, 0x60, 0x60, 0x00}, // F
    {0x63, 0x63, 0x63, 0x7F, 0x63, 0x63, 0x63, 0x00}, // H
    {0x3E, 0x06, 0x06, 0x06, 0x06, 0x66, 0x3C, 0x00}, // J
    {0xC3, 0xC3, 0x66, 0x3C, 0x18, 0x18, 0x18, 0x00}, // Y (fixed design)
    {0x00, 0x00, 0x18, 0x18, 0x00, 0x18, 0x18, 0x00}  // : (colon)
};
//each letter is 8 by 8 and the 0x values tells whether to draw the corresponding pixels or not

void swap_buffers() {
    while (*(volatile uint32_t*)STATUS_REG & 0x01);
    *(volatile uint32_t*)(FB_CONTROLLER + 4) = (uint32_t)back_buffer;
    *(volatile uint32_t*)FB_CONTROLLER = 1;
    volatile uint16_t* temp = front_buffer;
    front_buffer = back_buffer;
    back_buffer = temp;
}
//used to swap back and front buffer

void clear_backbuffer() {
    for (int y = 0; y < FB_HEIGHT; y++) {
        for (int x = 0; x < FB_WIDTH; x++) {
            back_buffer[y * FB_STRIDE + x] = COLOR_BLACK;
        }
    }
}
//this is used to clear the backbuffer to make it ready for next frame


//This function takes two 16-bit colors (c1 and c2) and calculates a blended (interpolated) color between them based on the current step and total max_steps. Essentially, it smoothly transitions from c1 to c2:
uint16_t interpolate_color(uint16_t c1, uint16_t c2, int step, int max_steps) {
    int r = ((c1 >> 11) * (max_steps - step) + (c2 >> 11) * step) / max_steps;
    int g = (((c1 >> 5) & 0x3F) * (max_steps - step) + ((c2 >> 5) & 0x3F) * step) / max_steps;
    int b = ((c1 & 0x1F) * (max_steps - step) + (c2 & 0x1F) * step) / max_steps;
    //extracting the r,g,b elements from their corresponding bits, respectively
    //then it takes max_step number of frames to change the color
    return (r << 11) | (g << 5) | b; //return the changed color scheme
}

void draw_char(int x, int y, char c, uint16_t color) {
    if (x < -CHAR_SIZE || x >= FB_WIDTH || y < -CHAR_SIZE || y >= FB_HEIGHT)
        return;
        //check if the letter is within the displayable area. If not no need to draw

    int idx = -1; // initialize as non-valid font
    if (c >= 'A' && c <= 'E') idx = c - 'A'; //If the character doesn’t match any known entry, the function returns immediately (i.e., it doesn’t draw unknown characters).
    else if (c == 'S') idx = 5;
    else if (c == 'R') idx = 6;
    else if (c == 'O') idx = 7;
    else if (c == 'P') idx = 8;
    else if (c == 'G') idx = 9;
    else if (c == 'M') idx = 10;
    else if (c == 'V') idx = 11;
    else if (c == ' ') idx = 12;
    else if (c >= '0' && c <= '9') idx = 13 + (c - '0');
    else if (c == 'T') idx = 23;
    else if (c == 'F') idx = 24;
    else if (c == 'H') idx = 25;
    else if (c == 'J') idx = 26;
    else if (c == 'Y') idx = 27;
    else if (c == ':') idx = 28;
    else return;

    // Draw character with black outline
    for (int dy = 0; dy < CHAR_SIZE; dy++) {
        uint8_t row = font[idx][dy];
        for (int dx = 0; dx < CHAR_SIZE; dx++) {
            if (row & (0x80 >> dx)) {
                int sx = x + dx + 1;
                int sy = y + dy + 1;
                if (sx < FB_WIDTH && sy < FB_HEIGHT)
                    back_buffer[sy * FB_STRIDE + sx] = COLOR_BLACK;
            }
        }
    } //print black pixel shadow in the buttom right unless already occupied

    // Draw main character
    for (int dy = 0; dy < CHAR_SIZE; dy++) {
        uint8_t row = font[idx][dy];
        for (int dx = 0; dx < CHAR_SIZE; dx++) {
            if (row & (0x80 >> dx)) {
                int px = x + dx;
                int py = y + dy;
                if (px < FB_WIDTH && py < FB_HEIGHT)
                    back_buffer[py * FB_STRIDE + px] = color;
            }
        }
    }
}

void init_stars() {
    for (int i = 0; i < NUM_STARS; i++) {
        stars[i].x = rand() % FB_WIDTH;
        stars[i].y = rand() % FB_HEIGHT;
        stars[i].speed = 1 + rand() % 3;
        uint16_t colors[] = {COLOR_WHITE, COLOR_CYAN, COLOR_GREY};
        stars[i].color = colors[rand() % 3];
    }
}

void update_stars() {
    for (int i = 0; i < NUM_STARS; i++) {
        stars[i].y += stars[i].speed;
        if (stars[i].y >= FB_HEIGHT) {
            stars[i].y = 0;
            stars[i].x = rand() % FB_WIDTH;
        }
    }
}

void draw_stars() {
    for (int i = 0; i < NUM_STARS; i++) {
        if (stars[i].x >= 0 && stars[i].x < FB_WIDTH && 
            stars[i].y >= 0 && stars[i].y < FB_HEIGHT) {
            back_buffer[stars[i].y * FB_STRIDE + stars[i].x] = stars[i].color;
        }
    }
}

void spawn_letter() {
    static char last_char = '\0'; //initialize it as null
    const char chars[] = "FGHJ"; //These arrays define which characters can spawn (F, G, H, or J) and what color they can be (chosen from the provided color array).
    const uint16_t colors[] = {COLOR_GREEN, COLOR_CYAN, COLOR_ORANGE, COLOR_PURPLE};

    for (int j = 0; j < 10; j++) {
        if (letters[j].active && letters[j].y < FB_HEIGHT/4) return;
    }
    // This loop checks all currently active letters. If any letter is still near the top quarter (FB_HEIGHT/4) of the screen, the function does nothing (return).

    for (int i = 0; i < 10; i++) {
        if (!letters[i].active) {
            int attempts = 0;
            char new_char;
            do {
                new_char = chars[rand() % 4];
                attempts++;
            } while (new_char == last_char && attempts < 10);
            last_char = new_char;

            letters[i].x = rand() % (FB_WIDTH - CHAR_SIZE*2) + CHAR_SIZE;
            letters[i].y = -CHAR_SIZE;
            letters[i].c = new_char;
            letters[i].color = colors[rand() % 4];
            letters[i].active = 1;
            break;
        }
        //This loop searches through the letters array for an inactive entry (i.e., active == 0). That entry will hold the new letter.
        /*A random character is chosen from "FGHJ".
        The do ... while ensures we don’t keep choosing the same new_char as last_char more than 10 times in a row.
        Once selected, last_char is updated to this new letter.*/
    }
}

void update_letters() {
    for (int i = 0; i < 10; i++) {
        if (letters[i].active) {
            letters[i].y += fall_speed;
            if (letters[i].y >= FB_HEIGHT - CHAR_SIZE) game_over = 1;
        }
    }
    fall_speed += 0.0015;
}

char get_key() {
    static int release_next = 0;
    static uint8_t current_key = 0;

    if ((*PS2_CTRL & 0x8000) == 0) return 0;
    uint8_t scan_code = *PS2_CTRL & 0xFF;
    //checks bit 15 (0x8000) of PS2DATA not CTRL!!! Cuz starts at same location to see if data is available. If that bit is 0, it means no new data is ready from the PS/2 device, so the function returns 0 (meaning “no key”).

    if (release_next) {
        release_next = 0;
        current_key = 0;
        return 0;
    }
    /*When a PS/2 key release occurs, the keyboard typically sends 0xF0 followed by the scan code of the key being released.
    release_next is a flag to indicate the function expects the next byte to be the release code’s actual scan code.*/

    if (scan_code == 0xF0) {
        release_next = 1;
        return 0;
    }

    if (current_key == scan_code) return 0;
    current_key = scan_code;
    /*If the newly read scan_code is the same as current_key, we return 0. That means “don’t report this key again” if it’s simply being held down.
    Otherwise, update current_key to this new scan code.*/

    switch (scan_code) {
        case 0x2B: return 'F';
        case 0x34: return 'G';
        case 0x33: return 'H';
        case 0x3B: return 'J';
        case 0x29: return ' ';
        default: return 0;
    }
    /*Each PS/2 scan code is mapped to an ASCII character.
    Only a small subset of keys is handled here:
    0x2B → 'F'
    0x34 → 'G'
    0x33 → 'H'
    0x3B → 'J'
    0x29 → ' ' (space)
    If the scan code isn’t recognized, default: return 0; means “no valid character.”*/
}

void check_collision(char key) {
    if (!key || game_over || key == ' ') return; 
    /*If no key was pressed (key == 0),
    If the game is already over, or
    If the key is space, then do nothing */

    int lowest_y = -1;
    for (int i = 0; i < 10; i++) {
        if (letters[i].active && letters[i].y > lowest_y) {
            lowest_y = letters[i].y;
        }
    }
    /*Iterate through all 10 possible letters and track the maximum y position in lowest_y.
    If lowest_y remains -1, it means no active letters were found, so there’s nothing to check. Return immediately.*/

    if (lowest_y == -1) return;

    int valid = 0;
    for (int i = 0; i < 10; i++) {
        if (letters[i].active && letters[i].y == lowest_y) {
            if (letters[i].c == key) {
                letters[i].active = 0;
                score += 100;
                valid = 1; //keeps the game alive
            } else {
                game_over = 1;
                return;
            }
        }
    }

    /*Among all active letters, focus on those that exactly match lowest_y. 
    If the letter’s character (letters[i].c) matches the pressed key, we:
    Deactivate that letter (letters[i].active = 0).
    Increase the score by 100.
    Mark valid = 1 (meaning we found a correct match).
    Otherwise, if it’s the wrong key, we end the game by setting game_over = 1 and returning.*/

    if (!valid) game_over = 1; 
}

void draw_bordered_text(int x, int y, const char* text, uint16_t color) { //Char* is just a string lol
    int len = strlen(text); //Determines how many characters are in text.
    for (int i = 0; i < len; i++) {
        draw_char(x + i*CHAR_SIZE, y, text[i], color);
    }
}

void draw_score() {
    char str[16];
    snprintf(str, sizeof(str), "SCORE:%04d", score);
    int len = strlen(str);
    int box_width = len * CHAR_SIZE + 20;
    int box_x = (FB_WIDTH - box_width) / 2;

    /*Creates a text string like "SCORE:0123" if score is 123.
    The format "%04d" means the integer is zero-padded to 4 digits (0123, 0099, etc.).
    snprintf safely writes into str without overflowing, up to the specified buffer size (sizeof(str)).*/
    
    // Draw background box
    for (int y = 0; y < CHAR_SIZE * 2; y++) {
        for (int x = box_x; x < box_x + box_width; x++) {
            back_buffer[y * FB_STRIDE + x] = interpolate_color(COLOR_DARK_BLUE, COLOR_BLUE, y, CHAR_SIZE*2);
        }
    }
    
    // Draw centered text
    int text_x = box_x + (box_width - len * CHAR_SIZE)/2;
    draw_bordered_text(text_x, 5, str, COLOR_YELLOW);

    /*The code above it drew a background rectangle starting at y = 0, with a height of CHAR_SIZE * 2 = 16 pixels (for an 8×8 font).
    Hardcoding 5 just gives the text a small vertical offset down from the top of that 16-pixel-tall box.
    In other words, instead of drawing the text exactly at y = 0 (the top), it’s moved down slightly to y = 5, so it visually sits somewhere in the middle or near the middle of that rectangle.*/
}

void draw_centered_box(const char* text, int y_offset, uint16_t color1, uint16_t color2) {
    int len = strlen(text);
    int box_width = len * CHAR_SIZE + 40;
    int box_height = CHAR_SIZE * 4;
    int box_x = (FB_WIDTH - box_width) / 2;
    int box_y = (FB_HEIGHT - box_height) / 2 + y_offset;

    // Draw box background
    //This loop draws a rectangular box with a vertical color gradient and a white border. 
    for (int y = box_y; y < box_y + box_height; y++) {
        uint16_t color = interpolate_color(color1, color2, y - box_y, box_height);
        //This loop goes from y = box_y down to y = box_y + box_height - 1. Each row calculates a gradient color by calling
        for (int x = box_x; x < box_x + box_width; x++) {
            if (x == box_x || x == box_x + box_width - 1 || 
                y == box_y || y == box_y + box_height - 1) {
                back_buffer[y * FB_STRIDE + x] = COLOR_WHITE;
            } else {
                back_buffer[y * FB_STRIDE + x] = color;
            }

            /*This loop fills each pixel in that row from x = box_x to x = box_x + box_width - 1.
            The if (...) checks if we are on the border: Left edge: x == box_x Right edge: x == box_x + box_width - 1 Top edge: y == box_y Bottom edge: y == box_y + box_height - 1
            If we are on the border, it draws white (COLOR_WHITE). Otherwise, it uses the gradient color computed for this row (color).*/
        }
    }
    

    // Draw centered text
    int text_x = (FB_WIDTH - len * CHAR_SIZE) / 2;
    int text_y = box_y + CHAR_SIZE;
    for (int i = 0; i < len; i++) {
        draw_char(text_x + i*CHAR_SIZE, text_y, text[i], COLOR_WHITE);
    }
}

int main() {
    srand(42); //Sets the seed for the random number generator. Here it’s a fixed seed (42), so the game will spawn letters and stars in the same pattern every time.
    init_stars(); //Initializes the star field with random positions, speeds, and colors. This ensures when you start, the background “stars” are scattered.
    *(volatile uint32_t*)(FB_CONTROLLER + 4) = (uint32_t)front_buffer;
    /*Informs the framebuffer controller which memory buffer (front_buffer) to use for video output initially. 
    This is specific to the hardware’s register layout (the “+4” offset typically points to the address register).*/

    while (1) {
        clear_backbuffer();
        update_stars();
        draw_stars();

        if (!game_started && !game_over) {
            // Start screen
            draw_centered_box("TYPE RACER", -40, COLOR_CYAN, COLOR_BLUE);
            draw_centered_box("PRESS SPACE", 40, COLOR_GREEN, COLOR_DARK_GREY);
            
            if (get_key() == ' ') {
                game_started = 1;
                memset(letters, 0, sizeof(letters)); //clears any letters from previous sessions.
                score = 0;
                fall_speed = 1.0;
                init_stars();
            }
        } else if (game_over) {
            // Game over screen
            draw_centered_box("GAME OVER", -40, COLOR_RED, COLOR_DARK_GREY);
            
            char score_str[16];
            snprintf(score_str, sizeof(score_str), "SCORE:%04d", score);
            draw_centered_box(score_str, 20, COLOR_DARK_GREY, COLOR_BLUE);
            
            draw_centered_box("PRESS SPACE", 60, COLOR_GREEN, COLOR_DARK_GREY);
            
            // Handle restart
            if (get_key() == ' ') {
                // Flush input buffer
                while (*PS2_CTRL & 0x8000) { 
                    volatile uint32_t dummy = *PS2_CTRL; 
                    (void)dummy;
                }
                /*it is repeatedly reading from the PS/2 data register as long as the “data ready” (or RVALID) bit (bit 15, which is 0x8000) is set. 
                Each read from that register removes one scan code (one byte of keyboard data) from the PS/2 hardware’s internal buffer or FIFO. By assigning the read result to dummy and then doing nothing with it ((void)dummy;), the code effectively throws away those scan codes.
                The key point is that a read from this memory-mapped register will pop data off the PS/2 controller’s queue. So this loop continues until there is no more data pending in the hardware (i.e., (*PS2_CTRL & 0x8000) is 0), thereby “flushing” any leftover key presses.*/
                game_over = 0;
                game_started = 1;
                memset(letters, 0, sizeof(letters));
                score = 0;
                fall_speed = 1.0;
                spawn_counter = 0;
                init_stars();

                /*(*PS2_CTRL & 0x8000) checks if any data is available in the PS/2 register.
                 * While data is available, it reads (*PS2_CTRL) into a dummy variable, discarding it.
                 * This ensures no “stale” or extra scan codes remain in the hardware buffer, so subsequent calls to get_key() won’t immediately pick up old input.

*/
            }
        } else {
            // Gameplay
            spawn_counter++; //Each iteration (or “frame”) of the game loop, spawn_counter goes up by 1.
            if (spawn_counter >= SPAWN_DELAY) {
                spawn_letter(); //When spawn_counter reaches or exceeds SPAWN_DELAY, it means enough frames have passed to spawn a new letter
                //The function spawn_letter() is called, which picks a random letter and color, then places it at the top of the screen to begin falling.
                spawn_counter = 0;
                //Immediately after spawning, spawn_counter is set back to 0 to “restart” the timing cycle.
            }

            check_collision(get_key());
            update_letters();

            // Draw active letters
            for (int i = 0; i < 10; i++) {
                if (letters[i].active) {
                    draw_char(letters[i].x, letters[i].y, letters[i].c, letters[i].color);
                }
            }

            draw_score();
        }

        swap_buffers();
        for (volatile int i = 0; i < 30000; i++);
    }
    return 0;
}