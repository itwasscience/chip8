#include "SDL2/SDL.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "chip8.h"

/**
 * Initializes the Chip8 standard font sprites into the first 0x50 bytes of memory.
 * @param chip8
 */
void chip8_init_fonts(struct chip8 *chip8) {
    for (size_t i = 0; i < FONT_LENGTH; ++i) {
        chip8->memory[i] = FONT_MAP[i];
    }
}

/**
 * Returns a new instance of a Chip8 CPU with some default values set.
 * @return
 */
struct chip8 new_chip8() {
    struct chip8 chip8 = {
            .memory = {0},
            .registers = {0},
            .reg_i = 0,
            .delay_timer = 0,
            .sound_timer = 0,
            .display = {0},
            .input = -1,
            .pc = 0x200,
            .sp = 0x0,
            .waiting_for_keypress = false,
            .draw_ready = false,
    };
    chip8_init_fonts(&chip8);
    return chip8;
}

/**
 * Blanks the video memory of the Chip8 CPU.
 * @param chip8
 */
void chip8_clear_display(struct chip8 *chip8) {
    for (size_t i = 0; i < SCREEN_PIXELS; i++) {
        chip8->display[i] = 0;
    }
}

/**
 * Executes a single step of the Chip8 CPU and updates the internal state.
 * @param chip8
 */
void chip8_step(struct chip8 *chip8) {
    u_int16_t opcode = (chip8->memory[chip8->pc] << 8) | (chip8->memory[chip8->pc + 1]);
    // These are "helpers" to reduce repeated logic in opcode parsing.
    // See http://devernay.free.fr/hacks/chip8/C8TECH10.HTM#3.0
    u_int8_t reg_x = (opcode & 0x0F00) >> 8;
    u_int8_t reg_y = (opcode & 0x00F0) >> 4;
    u_int8_t kk = (opcode & 0x00FF);
    switch (opcode & 0xF000) {
        case 0x0000:
            switch (opcode & 0x00FF) {
                case 0x00E0:
                    chip8_clear_display(chip8);
                    chip8->draw_ready = true;
                    chip8->pc += 2;
                    break;
                case 0x00EE:
                    chip8->pc = chip8->stack[chip8->sp] + 2;
                    // This prevents underflow with invalid RET invocations
                    if (chip8->sp > 0) { chip8->sp--; }
                    if (chip8->sp > 255) { printf("STACK OVERFLOW"); }
                    break;
            }
            break;
        case 0x1000: // JMP 1nnn
            chip8->pc = opcode & 0x0FFF;
            break;
        case 0x2000: // CALL 2nnn
            chip8->stack[++chip8->sp] = chip8->pc;
            chip8->pc = opcode & 0x0FFF;
            break;
        case 0x3000: // Skip Next Instruction if equal: Vx, byte
            chip8->pc += (chip8->registers[reg_x] == kk) ? 4 : 2;
            break;
        case 0x4000: // Skip Next Instruction if not equal: Vx, byte
            chip8->pc += (chip8->registers[reg_x] != kk) ? 4 : 2;
            break;
        case 0x5000: // Skip Next Instruction if Vx == Vy
            chip8->pc += (chip8->registers[reg_x] == chip8->registers[reg_y]) ? 4 : 2;
            break;
        case 0x6000: // Load Vx, byte
            chip8->registers[reg_x] = kk;
            chip8->pc += 2;
            break;
        case 0x7000: // Add Vx, byte
            chip8->registers[reg_x] += kk;
            chip8->pc += 2;
            break;
        case 0x8000: // Vx with Vy Opcodes
            switch (opcode & 0x000F) {
                case 0x0: // LD Vx, Vy
                    chip8->registers[reg_x] = chip8->registers[reg_y];
                    chip8->pc += 2;
                    break;
                case 0x1: // OR Vx, Vy
                    chip8->registers[reg_x] = chip8->registers[reg_x] | chip8->registers[reg_y];
                    chip8->pc += 2;
                    break;
                case 0x2: // AND Vx, Vy
                    chip8->registers[reg_x] = chip8->registers[reg_x] & chip8->registers[reg_y];
                    chip8->pc += 2;
                    break;
                case 0x3: // XOR Vx, Vy
                    chip8->registers[reg_x] = chip8->registers[reg_x] ^ chip8->registers[reg_y];
                    chip8->pc += 2;
                    break;
                case 0x4: // ADD Vx, Vy
                    if (chip8->registers[reg_y] > (0xFF - chip8->registers[reg_x])) {
                        chip8->registers[0xF] = 1;
                    } else {
                        chip8->registers[0xF] = 0;
                    }
                    // Overflow is acceptable and anticipated.
                    chip8->registers[reg_x] += chip8->registers[reg_y];
                    chip8->pc += 2;
                    break;
                case 0x5: // SUB Vx, Vy
                    if (chip8->registers[reg_y] > chip8->registers[reg_x]) {
                        chip8->registers[0xF] = 0;
                    } else {
                        chip8->registers[0xF] = 1;
                    }
                    // Overflow is acceptable and anticipated.
                    chip8->registers[reg_x] -= chip8->registers[reg_y];
                    chip8->pc += 2;
                    break;
                case 0x6: // SHR Vx {, Vy}
                    chip8->registers[0xF] = chip8->registers[reg_x] & 0x01; // LSB prior to shift
                    chip8->registers[reg_x] = chip8->registers[reg_x] >> 1;
                    chip8->pc += 2;
                    break;
                case 0x7: // SUBN Vx, Vy
                    if (chip8->registers[reg_x] > chip8->registers[reg_y]) {
                        chip8->registers[0xF] = 0;
                    } else {
                        chip8->registers[0xF] = 1;
                    }
                    chip8->registers[reg_x] = chip8->registers[reg_y] - chip8->registers[reg_x];
                    chip8->pc += 2;
                    break;
                case 0xE: // SHR Vx {, Vy}
                    chip8->registers[0xF] = chip8->registers[reg_x] > 7; // MSB prior to shift.
                    chip8->registers[reg_x] = chip8->registers[reg_x] << 1;
                    chip8->pc += 2;
                    break;
                default:
                    printf("UNSUPPORTED OPCODE:, %04x\n", opcode);
            }
            break;
        case 0x9000: // Skip Next Instruction if Vx != Vy
            chip8->pc += (chip8->registers[reg_x] != chip8->registers[reg_y]) ? 4 : 2;
            break;
        case 0xA000: // LD I, addr
            chip8->reg_i = opcode & 0x0FFF;
            chip8->pc += 2;
            break;
        case 0xB000: // JP V0, addr
            chip8->pc = (opcode & 0x0FFF) + chip8->registers[0];
            break;
        case 0xC000: // Rnd Vx, byte - This ain't crypto.
            chip8->registers[reg_x] = (rand() % 255) & kk; // NOLINT(cert-msc30-c, cert-msc50-cpp)
            chip8->pc += 2;
            break;
        case 0xD000: // Draws a sprite with collision detection
            chip8->registers[0xF] = 0;
            u_int8_t x_pos = chip8->registers[reg_x];
            u_int8_t y_pos = chip8->registers[reg_y];
            u_int8_t height = opcode & 0x000F;
            for (size_t y = 0; y < height; y++) {
                u_int8_t pixel_row = chip8->memory[chip8->reg_i + y];
                // Scan through each row per pixel bit for XOR / Collisions
                for (size_t x = 0; x < 8; x++) {
                    if (pixel_row & (0x80 >> x)) {
                        if (chip8->display[(y_pos + y) * SCREEN_WIDTH + x_pos + x]) {
                            chip8->registers[0xF] = 1;
                        }
                        chip8->display[(y_pos + y) * SCREEN_WIDTH + x_pos + x] ^= 1;
                    }
                }
            }
            chip8->draw_ready = true;
            chip8->pc += 2;
            break;
        case 0xE000: // Skips based on Keys
            switch (opcode & 0x00FF) {
                case 0x9E: // Skip if keypress matches Vx
                    if (chip8->input != 1) {
                        chip8->pc += (chip8->registers[reg_x] == chip8->input) ? 4 : 2;
                    } else {
                        chip8->pc += 2;
                    }
                    break;
                case 0xA1: // Skip if key is NOT pressed matching Vx
                    if (chip8->input != -1) {
                        chip8->pc += (chip8->registers[reg_x] != chip8->input) ? 4 : 2;
                    } else {
                        chip8->pc += 4;
                    }
                    break;
                default:
                    printf("UNSUPPORTED OPCODE:, %04x\n", opcode);
            }
            break;
        case 0xF000:
            switch (opcode & 0x00FF) {
                case 0x07: //LD Vx, Delay Timer
                    chip8->registers[reg_x] = chip8->delay_timer;
                    chip8->pc += 2;
                    break;
                case 0x0A: // Pause for Keypress - This is handled outside step
                    chip8->waiting_for_keypress = true;
                    chip8->pc += 2;
                    break;
                case 0x15: //LD Delay Timer, Vx
                    chip8->delay_timer = chip8->registers[reg_x];
                    chip8->pc += 2;
                    break;
                case 0x18: //LD Sound Timer, Vx
                    chip8->sound_timer = chip8->registers[reg_x];
                    chip8->pc += 2;
                    break;
                case 0x1E: // ADD I, Vx
                    chip8->reg_i = chip8->reg_i + chip8->registers[reg_x];
                    chip8->pc += 2;
                    break;
                case 0x29: // Load Sprite Digit to I - Sprites are in memory addrs 0x00 -> 0x49
                    chip8->reg_i = chip8->memory[chip8->registers[reg_x] * FONT_CHAR_LENGTH];
                    chip8->pc += 2;
                    break;
                case 0x33: // Load BCD of VX into memory [I, I+1, I+2]
                    chip8->memory[chip8->reg_i] = chip8->registers[reg_x] / 100;
                    chip8->memory[chip8->reg_i + 1] = (chip8->registers[reg_x] / 10) % 10;
                    chip8->memory[chip8->reg_i + 2] = chip8->registers[reg_x] % 10;
                    chip8->pc += 2;
                case 0x55: // Saves registers to memory[I]..memory[I + Vx]
                    for (size_t i = 0; i <= reg_x; i++) {
                        chip8->memory[chip8->reg_i + i] = chip8->registers[i];
                    }
                    chip8->pc += 2;
                    break;
                case 0x65: // Loads registers from memory[I]..memory[I + Vx]
                    for (size_t i = 0; i <= reg_x; i++) {
                        chip8->registers[i] = chip8->memory[chip8->reg_i + i];
                    }
                    chip8->pc += 2;
                    break;
                default:
                    printf("UNSUPPORTED OPCODE:, %04x\n", opcode);
            }
            break;
        default:
            printf("UNSUPPORTED OPCODE:, %04x\n", opcode);
    }
}

/**
 * Draws a pixel on the screen. Pixels support scaling on modern displays as bigger squares.
 * +-----------------+
 * |(0,0)  | (64, 0) |
 * |(32,0) | (64, 32)|
 * +-----------------+
 * @param renderer A reference to the SDL renderer
 * @param x Coordinate x, 0 is the left of the screen.
 * @param y Coordinate y, 0 is at the top of the screen.
 */
void chip8_draw_pixel(SDL_Renderer *renderer, int x, int y) {
    for (int i = 0; i < WINDOW_SCALE; i++) {
        for (int j = 0; j < WINDOW_SCALE; j++) {
            SDL_RenderDrawPoint(renderer, x * WINDOW_SCALE + i, y * WINDOW_SCALE + j);
        }
    }
}

/**
 * Renders the Chip8 display memory onto the SDL2 canvas.
 * @param renderer A reference to the SDL renderer
 * @param chip8 A reference to the Chip8 CPU
 */
void chip8_render(SDL_Renderer *renderer, struct chip8 *chip8) {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
    SDL_RenderClear(renderer);

    for (size_t i = 0; i < SCREEN_HEIGHT; i++) {
        for (size_t j = 0; j < SCREEN_WIDTH; j++) {
            if (chip8->display[i * SCREEN_WIDTH + j]) {
                SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
            } else {
                SDL_SetRenderDrawColor(renderer, 10, 10, 10, 255);
            }
            chip8_draw_pixel(renderer, j, i);
        }
    }
    SDL_RenderPresent(renderer);
    chip8->draw_ready = false;
}

/**
 * Loads a ROM file into the memory of the Chip8 CPU.
 *
 * Note that programs start at 0x200 per convention. The first 0x200 bytes of RAM are
 * reserved for the font sprites and the Chip8 interpreter code (historically).
 * @param filename A string with the filename of the ROM.
 * @param chip8 A reference to the Chip8 CPU to load the ROM into.
 */
bool chip8_load_rom(char *filename, struct chip8 *chip8) {
    char rom_buffer[MAX_ROM_SIZE];
    FILE *stream = fopen(filename, "r");
    if (stream) {
        fread(&rom_buffer, sizeof(char), MAX_ROM_SIZE, stream);
        fclose(stream);
        for (size_t i = 0; i < MAX_ROM_SIZE; i++) {
            chip8->memory[i + 0x200] = rom_buffer[i];
        }
        return true;
    } else {
        fprintf(stderr, "Could not open rom filename: %s.", filename);
        return false;
    }
}

/**
 * The main entry point for the interpreter. Responsible for building the SDL2 interface
 * and running the game loop.
 * @param argc
 * @param argv
 * @return
 */
int main(int argc, char *argv[argc + 1]) {
    if (! argv[1]) {
        printf("Usage: ./chip8 <rom_file>");
        return EXIT_SUCCESS;
    }

    struct chip8 chip8 = new_chip8();
    if (! chip8_load_rom(argv[1], &chip8)) {
        return EXIT_FAILURE;
    }

    SDL_Event event;
    SDL_Renderer *renderer;
    SDL_Window *window;

    SDL_Init(SDL_INIT_VIDEO);
    SDL_CreateWindowAndRenderer(WINDOW_WIDTH, WINDOW_HEIGHT, 0, &window, &renderer);

    // Event Loop
    struct timespec start_time = {0, 0}, tick_time = {0, 0};
    clock_gettime(CLOCK_MONOTONIC_RAW, &start_time);
    bool quit = false;
    while (!quit) {
        if (chip8.draw_ready) {
            chip8_render(renderer, &chip8);
            chip8.draw_ready = false;
        }
        while (SDL_PollEvent(&event) != 0) {
            switch (event.type) {
                case SDL_QUIT:
                    quit = true;
                    break;
                case SDL_KEYUP:
                    chip8.input = -1;
                    break;
                case SDL_KEYDOWN:
                    chip8.waiting_for_keypress = false;
                    switch (event.key.keysym.scancode) {
                        case SDL_SCANCODE_ESCAPE:
                            quit = true;
                            break;
                        case SDL_SCANCODE_0:
                        case SDL_SCANCODE_KP_0:
                            chip8.input = 0x0;
                            break;
                        case SDL_SCANCODE_1:
                        case SDL_SCANCODE_KP_1:
                            chip8.input = 0x1;
                            break;
                        case SDL_SCANCODE_2:
                        case SDL_SCANCODE_KP_2:
                            chip8.input = 0x2;
                            break;
                        case SDL_SCANCODE_3:
                        case SDL_SCANCODE_KP_3:
                            chip8.input = 0x3;
                            break;
                        case SDL_SCANCODE_4:
                        case SDL_SCANCODE_KP_4:
                            chip8.input = 0x4;
                            break;
                        case SDL_SCANCODE_5:
                        case SDL_SCANCODE_KP_5:
                            chip8.input = 0x5;
                            break;
                        case SDL_SCANCODE_6:
                        case SDL_SCANCODE_KP_6:
                            chip8.input = 0x6;
                            break;
                        case SDL_SCANCODE_7:
                        case SDL_SCANCODE_KP_7:
                            chip8.input = 0x7;
                            break;
                        case SDL_SCANCODE_8:
                        case SDL_SCANCODE_KP_8:
                            chip8.input = 0x8;
                            break;
                        case SDL_SCANCODE_9:
                        case SDL_SCANCODE_KP_9:
                            chip8.input = 0x9;
                            break;
                        case SDL_SCANCODE_A:
                        case SDL_SCANCODE_KP_PLUS:
                            chip8.input = 0xA;
                            break;
                        case SDL_SCANCODE_B:
                        case SDL_SCANCODE_KP_MINUS:
                            chip8.input = 0xB;
                            break;
                        case SDL_SCANCODE_C:
                        case SDL_SCANCODE_KP_MULTIPLY:
                            chip8.input = 0xC;
                            break;
                        case SDL_SCANCODE_D:
                        case SDL_SCANCODE_KP_DIVIDE:
                            chip8.input = 0xD;
                            break;
                        case SDL_SCANCODE_E:
                        case SDL_SCANCODE_KP_PERCENT:
                            chip8.input = 0xE;
                            break;
                        case SDL_SCANCODE_F:
                        case SDL_SCANCODE_KP_PERIOD:
                            chip8.input = 0xF;
                            break;
                        default:;
                    }
                    break;
                default:;
            }
        }
        // This approximates a 60hz clock.
        if (tick_time.tv_nsec - start_time.tv_nsec > 16000000) {
            if (chip8.delay_timer > 0) { chip8.delay_timer--; }
            if (chip8.sound_timer > 0) { chip8.sound_timer--; }
        }

        // A CPU at 1.8Mhz is around 556 nanoseconds per cycle (but we have processing time too)
        // We can change this to take a delta if we care later.
        nanosleep((const struct timespec[]) {{0, 500}}, NULL);

        if (!chip8.waiting_for_keypress) {
            chip8_step(&chip8);
        }
        clock_gettime(CLOCK_MONOTONIC_RAW, &tick_time);
    }
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return EXIT_SUCCESS;
}
