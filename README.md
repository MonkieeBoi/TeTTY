# TeTTY

Yet another tui Tetris (more like [Jstris](https://jstris.jezevec10.com/)) clone. This one however aims to follow guideline Tetris,
meaning SRS, hold, harddrop and all that good stuff.

# Showcase

[demo.mp4](https://github.com/user-attachments/assets/3efef1ad-505c-4e45-96d4-c199f255cf56)

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

# Build Dependencies

- ncurses
- inih

# Build Instructions

1. **Clone repository**:
   ```bash
   git clone https://github.com/MonkieeBoi/TeTTY.git
   ```
2. **Compile using Make**:

   ```bash
   make
   ```

# Running the Program

```bash
./tetty
```
