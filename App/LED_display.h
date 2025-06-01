#ifndef __LED_DISPLAY_H
#define __LED_DISPLAY_H


typedef struct
{
  uint32_t state_period;  // Step sequence period
  uint32_t state_cnt;     // Period counter
  uint32_t step_count;    // Number of steps
  uint32_t step_cnt;      // Step counter
  int32_t  start_x;       // Initial horizontal coordinate of symbol's top-left corner
  int32_t  start_y;       // Initial vertical coordinate of symbol's top-left corner
  int32_t  y_delta;       // Vertical increment per step
  int32_t  x_delta;       // Horizontal increment per step
  int32_t  x_pos;         // Current horizontal coordinate of symbol's top-left corner
  int32_t  y_pos;         // Current vertical coordinate of symbol's top-left corner
  uint32_t symbol_num;    // Symbol number to display

} T_din_symbol;

void Display_state_machine(void);
void Display_set_symbol(int32_t code, int32_t color);
void Display_copy_to_red_screen(uint8_t *ptr);
void Display_copy_to_green_screen(uint8_t *ptr);

void Set_dinamic_symbol(int32_t sym_num, int32_t period, int32_t step_num, int32_t deltax, int32_t deltay, int32_t startx, int32_t starty, int32_t color);

int32_t Remap_sym_code(int32_t code);
#endif
