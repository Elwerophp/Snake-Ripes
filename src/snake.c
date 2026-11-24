#include <stdint.h>

// Perifericos mapeados en memoria (I/O exports de Ripes)
#define LED_MATRIX_BASE   0xf0000000U
#define D_PAD_UP_ADDR     0xf0000644U
#define D_PAD_DOWN_ADDR   0xf0000648U
#define D_PAD_LEFT_ADDR   0xf000064cU
#define D_PAD_RIGHT_ADDR  0xf0000650U

// Dimensiones fisicas de la matriz LED (20x20)
#define MATRIX_WIDTH      20
#define MATRIX_HEIGHT     20

// Cuadricula logica de bloques 2x2 (10x10 celdas logicas)
#define BLOCK_SIZE        2
#define GRID_WIDTH        (MATRIX_WIDTH / BLOCK_SIZE)
#define GRID_HEIGHT       (MATRIX_HEIGHT / BLOCK_SIZE)

// Colores RGB (0x00RRGGBB)
#define COLOR_OFF         0x00000000U
#define COLOR_RED         0x00FF0000U
#define COLOR_GREEN       0x0000FF00U

// Direcciones de movimiento
#define DIR_NONE          0
#define DIR_UP            1
#define DIR_DOWN          2
#define DIR_LEFT          3
#define DIR_RIGHT         4

// Configuracion de la serpiente (cada segmento es un bloque 2x2 = 4 LEDs)
#define MAX_SNAKE_BLOCKS  100
#define INITIAL_BLOCKS    1

typedef struct {
    int x; // Coordenada logica (0..9)
    int y; // Coordenada logica (0..9)
} Point;

// Punteros volatiles a hardware (MMIO)
static volatile uint32_t * const led_matrix = (volatile uint32_t *)LED_MATRIX_BASE;
static volatile uint32_t * const dpad_up    = (volatile uint32_t *)D_PAD_UP_ADDR;
static volatile uint32_t * const dpad_down  = (volatile uint32_t *)D_PAD_DOWN_ADDR;
static volatile uint32_t * const dpad_left  = (volatile uint32_t *)D_PAD_LEFT_ADDR;
static volatile uint32_t * const dpad_right = (volatile uint32_t *)D_PAD_RIGHT_ADDR;

// Estado de la serpiente y de la manzana
Point snake[MAX_SNAKE_BLOCKS];
int snake_blocks = INITIAL_BLOCKS;
int current_dir  = DIR_RIGHT;
int next_dir     = DIR_RIGHT;

Point apple;
uint32_t rng_state    = 123456789U;
uint32_t tick_counter = 0U;

// Enciende un LED fisico en (px, py)
void set_led(int px, int py, uint32_t color) {
    if (px >= 0 && px < MATRIX_WIDTH && py >= 0 && py < MATRIX_HEIGHT) {
        led_matrix[py * MATRIX_WIDTH + px] = color;
    }
}

// Dibuja un bloque de 2x2 LEDs en la celda logica (gx, gy)
void draw_block(int gx, int gy, uint32_t color) {
    int base_x = gx * BLOCK_SIZE;
    int base_y = gy * BLOCK_SIZE;

    set_led(base_x,     base_y,     color);
    set_led(base_x + 1, base_y,     color);
    set_led(base_x,     base_y + 1, color);
    set_led(base_x + 1, base_y + 1, color);
}

// Apaga toda la matriz LED
void clear_matrix(void) {
    for (int py = 0; py < MATRIX_HEIGHT; py++) {
        for (int px = 0; px < MATRIX_WIDTH; px++) {
            set_led(px, py, COLOR_OFF);
        }
    }
}

// Lee los 4 botones del D-Pad una sola vez por consulta
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

// Actualiza la proxima direccion evitando giros opuestos de 180 grados
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

// Controla la velocidad de avance mientras sigue escuchando el D-Pad
void step_delay(void) {
    for (int poll = 0; poll < 10; poll++) {
        tick_counter++;
        update_direction();
        for (volatile uint32_t i = 0; i < 2000; i++) {
        }
    }
}

// Generador pseudoaleatorio ligero (LCG)
uint32_t next_random(void) {
    rng_state = rng_state * 1664525U + 1013904223U + tick_counter;
    return rng_state;
}

// Verifica si la celda logica (gx, gy) esta ocupada por la serpiente
int is_on_snake(int gx, int gy) {
    for (int i = 0; i < snake_blocks; i++) {
        if (snake[i].x == gx && snake[i].y == gy) {
            return 1;
        }
    }
    return 0;
}

// Genera una manzana verde de 2x2 dentro de los limites y fuera de la serpiente
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

// Inicializa la serpiente con 1 bloque de 2x2 (4 LEDs rojos) en el centro
void init_snake(void) {
    snake_blocks = INITIAL_BLOCKS;
    current_dir  = DIR_RIGHT;
    next_dir     = DIR_RIGHT;

    snake[0].x = 4;
    snake[0].y = 5;
}

// Dibuja todos los bloques 2x2 de la serpiente en rojo
void draw_snake(void) {
    for (int i = 0; i < snake_blocks; i++) {
        draw_block(snake[i].x, snake[i].y, COLOR_RED);
    }
}

// Avanza la serpiente un bloque en la direccion actual
void move_snake(void) {
    current_dir = next_dir;

    // Borrar el ultimo bloque (cola) antes de mover
    Point tail = snake[snake_blocks - 1];
    draw_block(tail.x, tail.y, COLOR_OFF);

    // Desplazar segmentos del cuerpo hacia adelante
    for (int i = snake_blocks - 1; i > 0; i--) {
        snake[i] = snake[i - 1];
    }

    // Calcular nueva posicion de la cabeza
    if (current_dir == DIR_UP) {
        snake[0].y--;
    } else if (current_dir == DIR_DOWN) {
        snake[0].y++;
    } else if (current_dir == DIR_LEFT) {
        snake[0].x--;
    } else if (current_dir == DIR_RIGHT) {
        snake[0].x++;
    }

    // Envolvente temporal para probar antes de la etapa de colisiones
    if (snake[0].x < 0)            snake[0].x = GRID_WIDTH - 1;
    if (snake[0].x >= GRID_WIDTH)  snake[0].x = 0;
    if (snake[0].y < 0)            snake[0].y = GRID_HEIGHT - 1;
    if (snake[0].y >= GRID_HEIGHT) snake[0].y = 0;

    // Dibujar nueva cabeza
    draw_block(snake[0].x, snake[0].y, COLOR_RED);

    // Si alcanza la manzana, generar una nueva en otra celda libre
    if (snake[0].x == apple.x && snake[0].y == apple.y) {
        spawn_apple();
    }
}

int main(void) {
    clear_matrix();
    init_snake();
    draw_snake();
    spawn_apple();

    while (1) {
        step_delay();
        move_snake();
    }

    return 0;
}
