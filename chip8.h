//
// Created by zoey on 2/13/21.
//

#ifndef LEARNING_C_CHIP8_H
#define LEARNING_C_CHIP8_H

#endif //LEARNING_C_CHIP8_H

#define MAX_ROM_SIZE 0x1000 - 0x0200 // Reserved for fonts
#define FONT_CHAR_LENGTH 5
#define FONT_LENGTH 80
#define SCREEN_WIDTH 64
#define SCREEN_HEIGHT 32
#define WINDOW_SCALE 10
#define WINDOW_WIDTH SCREEN_WIDTH * WINDOW_SCALE
#define WINDOW_HEIGHT SCREEN_HEIGHT * WINDOW_SCALE
#define SCREEN_PIXELS SCREEN_WIDTH * SCREEN_HEIGHT
#define STACK_DEPTH 10

const static u_int8_t FONT_MAP[FONT_LENGTH] = {
        0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
        0x20, 0x60, 0x20, 0x20, 0x70, // 1
        0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
        0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
        0x90, 0x90, 0xF0, 0x10, 0x10, // 4
        0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
        0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
        0xF0, 0x10, 0x20, 0x40, 0x40, // 7
        0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
        0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
        0xF0, 0x90, 0xF0, 0x90, 0x90, // A
        0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
        0xF0, 0x80, 0x80, 0x80, 0xF0, // C
        0xE0, 0x90, 0x90, 0x90, 0xE0, // D
        0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
        0xF0, 0x80, 0xF0, 0x80, 0x80  // F
};

struct chip8 {
    u_int8_t memory[4096];
    u_int8_t registers[0x10];
    u_int16_t reg_i;
    u_int8_t delay_timer;
    u_int8_t sound_timer;
    u_int8_t display[SCREEN_PIXELS];
    int8_t input; // -1 for no input
    // Internals
    u_int16_t stack[STACK_DEPTH];
    u_int16_t pc;
    u_int16_t sp;
    bool waiting_for_keypress;
    bool draw_ready;
};

struct chip8 new_chip8();
void chip8_init_fonts(struct chip8 *chip8);
void chip8_step(struct chip8 *chip8);
void chip8_clear_display(struct chip8 *chip8);