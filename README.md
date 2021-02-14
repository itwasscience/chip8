# Chip8 Interpreter
Used as an exercise to learn some C programming. 

## Usage:
./chip8 <rom_file>

# Build Requirements
You must have the SDL2-devel libraries installed. See https://www.libsdl.org/download-2.0.php.

# Key Bindings
Keys are mapped as follows:

### US Keyboard 
`0 - 9` and `a - f` for hex digits.

### US Keyboard Extended Keypad
```
0 - 9
A +
B -
C *
D /
E %
F .
```

## Other Notes
If the display is too big or small there is a `WINDOW_SCALE` parameter that multiplies the pixel size by that factor.

**Ex:** `WINDOW_SCALE 10` will result in a display of 320x640 instead of 32x64 and is the default.
