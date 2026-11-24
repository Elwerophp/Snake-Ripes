#include <stdint.h>

// Direcciones de la matriz LED y los botones D-Pad
#define LED_MATRIX_BASE   0xf0000000U
#define D_PAD_UP_ADDR     0xf0000644U
#define D_PAD_DOWN_ADDR   0xf0000648U
#define D_PAD_LEFT_ADDR   0xf000064cU
#define D_PAD_RIGHT_ADDR  0xf0000650U

// Tamano de la pantalla (20x20 LEDs)
#define MATRIX_WIDTH      20
#define MATRIX_HEIGHT     20

// Cuadricula de cuadros de 2x2 (10x10 cuadros en total)
#define BLOCK_SIZE        2
#define GRID_WIDTH        (MATRIX_WIDTH / BLOCK_SIZE)
#define GRID_HEIGHT       (MATRIX_HEIGHT / BLOCK_SIZE)

// Colores
#define COLOR_OFF         0x00000000U
#define COLOR_RED         0x00FF0000U
#define COLOR_GREEN       0x0000FF00U
#define COLOR_BLUE        0x000000FFU

// Direcciones
#define DIR_NONE          0
#define DIR_UP            1
#define DIR_DOWN          2
#define DIR_LEFT          3
#define DIR_RIGHT         4

#define MAX_SNAKE_BLOCKS  100
#define INITIAL_BLOCKS    1

typedef struct {
    int x;
    int y;
} Point;

static volatile uint32_t * const led_matrix = (volatile uint32_t *)LED_MATRIX_BASE;
static volatile uint32_t * const dpad_up    = (volatile uint32_t *)D_PAD_UP_ADDR;
static volatile uint32_t * const dpad_down  = (volatile uint32_t *)D_PAD_DOWN_ADDR;
static volatile uint32_t * const dpad_left  = (volatile uint32_t *)D_PAD_LEFT_ADDR;
static volatile uint32_t * const dpad_right = (volatile uint32_t *)D_PAD_RIGHT_ADDR;

Point snake[MAX_SNAKE_BLOCKS];
int snake_blocks = INITIAL_BLOCKS;
int current_dir  = DIR_RIGHT;
int next_dir     = DIR_RIGHT;
int game_over    = 0;

Point apple;
uint32_t rng_state    = 123456789U;
uint32_t tick_counter = 0U;

void set_led(int px, int py, uint32_t color) {
    if (px >= 0 && px < MATRIX_WIDTH && py >= 0 && py < MATRIX_HEIGHT) {
        led_matrix[py * MATRIX_WIDTH + px] = color;
    }
}

void draw_block(int gx, int gy, uint32_t color) {
    if (gx < 0 || gx >= GRID_WIDTH || gy < 0 || gy >= GRID_HEIGHT) {
        return;
    }

    int base_x = gx * BLOCK_SIZE;
    int base_y = gy * BLOCK_SIZE;

    set_led(base_x,     base_y,     color);
    set_led(base_x + 1, base_y,     color);
    set_led(base_x,     base_y + 1, color);
    set_led(base_x + 1, base_y + 1, color);
}

void clear_matrix(void) {
    for (int py = 0; py < MATRIX_HEIGHT; py++) {
        for (int px = 0; px < MATRIX_WIDTH; px++) {
            set_led(px, py, COLOR_OFF);
        }
    }
}

int read_dpad(void) {
    uint32_t up    = (*dpad_up)    & 0x1U;
    uint32_t down  = (*dpad_down)  & 0x1U;
    uint32_t left  = (*dpad_left)  & 0x1U;
    uint32_t right = (*dpad_right) & 0x1U;

    if (up)    return DIR_UP;
    if (down)  return DIR_DOWN;
    if (left)  return DIR_LEFT;
    if (right) return DIR_RIGHT;
    return DIR_NONE;
}

void update_direction(void) {
    int input = read_dpad();

    if (input == DIR_UP && current_dir != DIR_DOWN) {
        next_dir = DIR_UP;
    } else if (input == DIR_DOWN && current_dir != DIR_UP) {
        next_dir = DIR_DOWN;
    } else if (input == DIR_LEFT && current_dir != DIR_RIGHT) {
        next_dir = DIR_LEFT;
    } else if (input == DIR_RIGHT && current_dir != DIR_LEFT) {
        next_dir = DIR_RIGHT;
    }
}

void step_delay(void) {
    for (int poll = 0; poll < 10; poll++) {
        tick_counter++;
        update_direction();
        for (volatile uint32_t i = 0; i < 2000; i++) {
        }
    }
}

uint32_t next_random(void) {
    rng_state = rng_state * 1664525U + 1013904223U + tick_counter;
    return rng_state;
}

int is_on_snake(int gx, int gy) {
    for (int i = 0; i < snake_blocks; i++) {
        if (snake[i].x == gx && snake[i].y == gy) {
            return 1;
        }
    }
    return 0;
}

void spawn_apple(void) {
    int total_cells = GRID_WIDTH * GRID_HEIGHT;
    int start_idx   = (int)(next_random() % (uint32_t)total_cells);

    for (int offset = 0; offset < total_cells; offset++) {
        int idx = (start_idx + offset) % total_cells;
        int gx  = idx % GRID_WIDTH;
        int gy  = idx / GRID_WIDTH;

        if (!is_on_snake(gx, gy)) {
            apple.x = gx;
            apple.y = gy;
            draw_block(apple.x, apple.y, COLOR_GREEN);
            return;
        }
    }
}

void init_snake(void) {
    snake_blocks = INITIAL_BLOCKS;
    current_dir  = DIR_RIGHT;
    next_dir     = DIR_RIGHT;
    game_over    = 0;

    snake[0].x = 4;
    snake[0].y = 5;
}

void draw_snake(uint32_t color) {
    for (int i = 0; i < snake_blocks; i++) {
        draw_block(snake[i].x, snake[i].y, color);
    }
}

// Revisa si la nueva posicion choca con un borde o con su propio cuerpo
int check_collision(int nx, int ny, int growing) {
    if (nx < 0 || nx >= GRID_WIDTH || ny < 0 || ny >= GRID_HEIGHT) {
        return 1;
    }

    int limit = growing ? snake_blocks : (snake_blocks - 1);
    for (int i = 0; i < limit; i++) {
        if (snake[i].x == nx && snake[i].y == ny) {
            return 1;
        }
    }

    return 0;
}

void move_snake(void) {
    current_dir = next_dir;

    int new_x = snake[0].x;
    int new_y = snake[0].y;

    if (current_dir == DIR_UP) {
        new_y--;
    } else if (current_dir == DIR_DOWN) {
        new_y++;
    } else if (current_dir == DIR_LEFT) {
        new_x--;
    } else if (current_dir == DIR_RIGHT) {
        new_x++;
    }

    int ate_apple = (new_x == apple.x && new_y == apple.y);

    if (check_collision(new_x, new_y, ate_apple)) {
        game_over = 1;
        draw_snake(COLOR_BLUE);
        return;
    }

    if (ate_apple && snake_blocks < MAX_SNAKE_BLOCKS) {
        for (int i = snake_blocks; i > 0; i--) {
            snake[i] = snake[i - 1];
        }
        snake[0].x = new_x;
        snake[0].y = new_y;
        snake_blocks++;

        draw_block(snake[0].x, snake[0].y, COLOR_RED);
        spawn_apple();
    } else {
        Point tail = snake[snake_blocks - 1];
        draw_block(tail.x, tail.y, COLOR_OFF);

        for (int i = snake_blocks - 1; i > 0; i--) {
            snake[i] = snake[i - 1];
        }
        snake[0].x = new_x;
        snake[0].y = new_y;

        draw_block(snake[0].x, snake[0].y, COLOR_RED);
    }
}

int main(void) {
    clear_matrix();
    init_snake();
    draw_snake(COLOR_RED);
    spawn_apple();

    while (1) {
        if (!game_over) {
            step_delay();
            move_snake();
        }
    }

    return 0;
}
