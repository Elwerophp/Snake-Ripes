#include <stdint.h>

// Periferico LED Matrix 0 (I/O exports de Ripes)
#define LED_MATRIX_BASE   0xf0000000U

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

// Configuracion de la serpiente (cada segmento es un bloque 2x2 = 4 LEDs)
#define MAX_SNAKE_BLOCKS  100
#define INITIAL_BLOCKS    1

typedef struct {
    int x; // Coordenada logica (0..9)
    int y; // Coordenada logica (0..9)
} Point;

// Puntero volatil a la memoria mapeada de la matriz LED
static volatile uint32_t * const led_matrix = (volatile uint32_t *)LED_MATRIX_BASE;

// Estado de la serpiente
Point snake[MAX_SNAKE_BLOCKS];
int snake_blocks = INITIAL_BLOCKS;

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

// Inicializa la serpiente con 1 bloque de 2x2 (4 LEDs rojos) en el centro
void init_snake(void) {
    snake_blocks = INITIAL_BLOCKS;
    snake[0].x = 4; // LEDs fisicos x = 8, 9
    snake[0].y = 5; // LEDs fisicos y = 10, 11
}

// Dibuja todos los bloques 2x2 de la serpiente en rojo
void draw_snake(void) {
    for (int i = 0; i < snake_blocks; i++) {
        draw_block(snake[i].x, snake[i].y, COLOR_RED);
    }
}

int main(void) {
    clear_matrix();
    init_snake();
    draw_snake();

    while (1) {
        // Espera activa: en la siguiente etapa agregaremos D-Pad y movimiento
    }

    return 0;
}
