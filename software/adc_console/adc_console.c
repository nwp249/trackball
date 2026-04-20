#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/time.h"
#include "hardware/gpio.h"
#include "hardware/adc.h"

// GPIO port macros
#define RESET 16
#define DATA_COLS 19
#define DATA_ROWS 20
#define DATA_CLK 18
#define SHIFT_CLK 17
#define PORT_MASK 0x001F0000


// Points
const uint8_t points_2x2[7][2] = {{1, 2},{2, 3},{3,4},{4,5},{5,6},{6,7},{7,8}};
const uint adc_max = (1 << 12) - 1;

// Sets a 2x2 point with the top left corner at the coordinate to show joystick input
void set_2x2_point(const uint8_t * rows, const uint8_t * cols);

// Sets a 1x1 point at specified coordinate
void set_1x1_point(uint8_t row, uint8_t col);

// Clears the shift registers
void clear_registers();

// Clocks the shift registers
void shift();

// Sets the data registers
void set();

// Cycles through all matrix dots
void demo();

// Cycles through 2x2 zones
void demo2();


int main(void) {
    stdio_init_all();
    adc_init();
    // Make sure GPIO is high-impedance, no pullups etc
    adc_gpio_init(26);
    adc_gpio_init(27);
    // Initalize reset, data, and clock lines for output, set to low
    gpio_init_mask(PORT_MASK);
    gpio_set_dir_out_masked(PORT_MASK);
    gpio_put_masked(PORT_MASK, 0);
    clear_registers();
    set();

    while(1){
        adc_select_input(0);
        uint adc_x_raw = adc_max - adc_read();
        adc_select_input(1);
        uint adc_y_raw = adc_read();

        // scale from 0-6
        float x_coord = adc_x_raw * 6 / (float)adc_max;
        float y_coord = adc_y_raw * 6 / (float)adc_max;

        // should be integers 0-6 now, rounded
        uint x = (x_coord - (int)x_coord > 0.5 ? (int)(x_coord + 1) : (int)x_coord);
        uint y = (y_coord - (int)y_coord > 0.5 ? (int)(y_coord + 1) : (int)y_coord);

        set_2x2_point(points_2x2[x], points_2x2[y]);
    }

}

// Sets a 2x2 point on the matrix
void set_2x2_point(const uint8_t * rows, const uint8_t * cols){
    uint8_t row_mask = (1 << (rows[0] - 1) | 1 << (rows[1] - 1));
    uint8_t col_mask = (1 << (cols[0] - 1) | 1 << (cols[1] - 1));
    col_mask = col_mask ^ 0xFF; // column has to be inverse

    clear_registers();

    for (short i = 8; i >= 0; i--){
        gpio_put(DATA_COLS, ((col_mask >> i) & 1));
        gpio_put(DATA_ROWS, ((row_mask >> i) & 1));
        shift();
    }

    set();
}

// Sets a point on the matrix, perhaps more accurately changes it
void set_1x1_point(uint8_t row, uint8_t col){
    uint8_t row_mask = 1 << (row - 1);
    uint8_t col_mask = 1 << (col - 1);
    col_mask = col_mask ^ 0xFF; // column has to be inverse

    clear_registers();

    for (short i = 8; i >= 0; i--){
        gpio_put(DATA_COLS, ((col_mask >> i) & 1));
        gpio_put(DATA_ROWS, ((row_mask >> i) & 1));
        shift();
    }

    set();

}

// Clears the shift registers, does not affect data register (output)
void clear_registers(){
    gpio_put(RESET, 0);
    sleep_us(0.5); // Input is 5v, datasheet says maximum switching is around 50ns, doing 500 to be safe
    gpio_put(RESET, 1);
}

// Clock the shift registers
void shift(){
    gpio_put(SHIFT_CLK, 1);
    sleep_us(0.1); // again, should only take < 50ns, doing 100 here
    gpio_put(SHIFT_CLK, 0);
}

// Sets the data registers
void set(){
    gpio_put(DATA_CLK, 1);
    sleep_us(0.1); // again, should only take < 50ns, doing 100 here
    gpio_put(DATA_CLK, 0);
}

// Cycle through all points on the matrix
void demo(){
    for (short i = 1; i <= 8; i++){
        for (short j = 1; j <= 8; j++){
            set_1x1_point(i, j);
            sleep_ms(100);
        }
    }
}

// Cycle through all 2x2 zones
void demo2(){
    for (short i = 0; i < 7; i++){
        for (short j = 0; j < 7; j++){
            set_2x2_point(points_2x2[i], points_2x2[j]);
            sleep_ms(100);
        }
    }
}
