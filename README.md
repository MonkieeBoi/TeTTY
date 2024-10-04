# TeTTY

Yet another tui Tetris (more like [Jstris](https://jstris.jezevec10.com/)) clone. This one however aims to follow guideline Tetris,
meaning SRS, hold, harddrop and all that good stuff.

# Showcase

[demo.mp4](https://github.com/MonkieeBoi/TeTTY/assets/53400613/586f96bd-8d89-4f3a-8559-389a4d78d1ff)

# Todo

- [X] Come up with a name
- [X] Board
- [ ] Handle pieces
    - [X] Define
    - [X] Spawn
    - [X] Draw (probably with ncurses)
    - [ ] Move
        - [X] DAS
        - [ ] ARR
        - [X] Gravity
    - [X] Rotate (SRS)
    - [X] Harddrop
    - [ ] Lock Delay
- [X] Add hold
- [X] Add 7-bag
- [X] Handle line clearing
- [X] Draw queue
- [X] Draw ghost piece
- [X] Draw key overlay
- [X] Draw stats

# Instructions (in progress)

TeTTY is a C program that uses `ncurses` for terminal UI and `inih` for parsing INI configuration files. Below is a guide on how to build and run the project.

## Requirements

Before you can build and run the project, ensure the following libraries and tools are installed:

1. **GNU Compiler Collection (gcc)**: Used for compiling the C program.
2. **Make**: To manage the build process.
3. **ncurses**: A library for handling terminal interfaces.
4. **inih**: A library for INI file parsing.
5. **AddressSanitizer (optional)**: For memory error detection during runtime (available through `gcc`).

### Installing Required Libraries (Ubuntu)

```bash
sudo apt-get update
sudo apt-get install build-essential libncurses5-dev libncursesw5-dev
```

If the `inih` library is not installed, clone it from GitHub or install it from your package manager.

```bash
# Example of cloning inih from GitHub
git clone https://github.com/benhoyt/inih.git
```

Ensure that both `ncurses` and `inih` are available and linkable on your system.

## Build Instructions

1. **Clone the repository**:
   ```bash
   git clone https://github.com/MonkieeBoi/TeTTY.git
   ```

2. **Compile the project using Make**:

   You can use `make` to compile the code:
   ```bash
   make
   ```

   This will compile the source files in the `src/` directory and output object files to the `build/` folder.

## Running the Program

Once the program is successfully compiled and linked, you can run it as follows:

```bash
./tetty
```

Don't forget to make the terminal big enough to render TeTTY, or you will get an error saying "Screen dimensions smaller than..."
