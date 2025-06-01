#include "Application.h"

static int32_t g_line_cnt;

uint8_t red_screen[8];
uint8_t green_screen[8];

T_din_symbol red_dsym;
T_din_symbol green_dsym;

//------------------------------------------------------------------------------
// Copy symbol data to red screen buffer
//------------------------------------------------------------------------------
static void Copy_red_screen(int32_t code)
{
  red_screen[0] = Symbols[Remap_sym_code(code)][0];
  red_screen[1] = Symbols[Remap_sym_code(code)][1];
  red_screen[2] = Symbols[Remap_sym_code(code)][2];
  red_screen[3] = Symbols[Remap_sym_code(code)][3];
  red_screen[4] = Symbols[Remap_sym_code(code)][4];
  red_screen[5] = Symbols[Remap_sym_code(code)][5];
  red_screen[6] = Symbols[Remap_sym_code(code)][6];
  red_screen[7] = Symbols[Remap_sym_code(code)][7];
}

//------------------------------------------------------------------------------
// Copy symbol data to green screen buffer
//------------------------------------------------------------------------------
static void Copy_green_screen(int32_t code)
{
  green_screen[0] = Symbols[Remap_sym_code(code)][0];
  green_screen[1] = Symbols[Remap_sym_code(code)][1];
  green_screen[2] = Symbols[Remap_sym_code(code)][2];
  green_screen[3] = Symbols[Remap_sym_code(code)][3];
  green_screen[4] = Symbols[Remap_sym_code(code)][4];
  green_screen[5] = Symbols[Remap_sym_code(code)][5];
  green_screen[6] = Symbols[Remap_sym_code(code)][6];
  green_screen[7] = Symbols[Remap_sym_code(code)][7];
}
//------------------------------------------------------------------------------
// Set display symbol with specified color
//------------------------------------------------------------------------------
void Display_set_symbol(int32_t code, int32_t color)
{
  if (code >= Get_symbols_count())
    return;
  switch (color)
  {
    case 0:

      red_dsym.state_period = 0;
      Copy_red_screen(code);
      // memset(green_screen, 0, sizeof(green_screen));
      break;
    case 1:
      green_dsym.state_period = 0;
      Copy_green_screen(code);
      // memset(red_screen,  0, sizeof(red_screen));
      break;
    case 2:
      red_dsym.state_period   = 0;
      green_dsym.state_period = 0;
      Copy_red_screen(code);
      Copy_green_screen(code);
      break;
  }
}

//------------------------------------------------------------------------------
// Copy data to red screen buffer
//------------------------------------------------------------------------------
void Display_copy_to_red_screen(uint8_t *ptr)
{
  red_dsym.state_period = 0;
  red_screen[0]         = ptr[0];
  red_screen[1]         = ptr[1];
  red_screen[2]         = ptr[2];
  red_screen[3]         = ptr[3];
  red_screen[4]         = ptr[4];
  red_screen[5]         = ptr[5];
  red_screen[6]         = ptr[6];
  red_screen[7]         = ptr[7];
}
//------------------------------------------------------------------------------
// Copy data to green screen buffer
//------------------------------------------------------------------------------
void Display_copy_to_green_screen(uint8_t *ptr)
{
  green_dsym.state_period = 0;
  green_screen[0]         = ptr[0];
  green_screen[1]         = ptr[1];
  green_screen[2]         = ptr[2];
  green_screen[3]         = ptr[3];
  green_screen[4]         = ptr[4];
  green_screen[5]         = ptr[5];
  green_screen[6]         = ptr[6];
  green_screen[7]         = ptr[7];
}


/*-----------------------------------------------------------------------------------------------------

-----------------------------------------------------------------------------------------------------*/
void Set_dinamic_symbol(int32_t sym_num, int32_t period, int32_t step_num, int32_t deltax, int32_t deltay, int32_t startx, int32_t starty, int32_t color)
{
  T_din_symbol *pdsym1 = 0;
  T_din_symbol *pdsym2 = 0;
  if (color == 0)
  {
    pdsym1 = &red_dsym;
  }
  else if (color == 1)
  {
    pdsym1 = &green_dsym;
  }
  else if (color == 2)
  {
    pdsym1 = &red_dsym;
    pdsym2 = &green_dsym;
  }

  if (pdsym1 != 0)
  {
    pdsym1->state_period = 0;
    pdsym1->start_x      = startx;
    pdsym1->start_y      = starty;
    pdsym1->state_cnt    = period;
    pdsym1->step_cnt     = step_num;
    pdsym1->step_count   = step_num;
    pdsym1->symbol_num   = sym_num;
    pdsym1->x_delta      = deltax;
    pdsym1->y_delta      = deltay;
    pdsym1->x_pos        = pdsym1->start_x;
    pdsym1->y_pos        = pdsym1->start_y;
    pdsym1->state_period = period;
  }

  if (pdsym2 != 0)
  {
    pdsym2->state_period = 0;
    pdsym2->start_x      = startx;
    pdsym2->start_y      = starty;
    pdsym2->state_cnt    = period;
    pdsym2->step_cnt     = step_num;
    pdsym2->step_count   = step_num;
    pdsym2->symbol_num   = sym_num;
    pdsym2->x_delta      = deltax;
    pdsym2->y_delta      = deltay;
    pdsym2->x_pos        = pdsym1->start_x;
    pdsym2->y_pos        = pdsym1->start_y;
    pdsym2->state_period = period;
  }
}

//------------------------------------------------------------------------------
// Dynamic symbol animation procedure
//------------------------------------------------------------------------------
static void Dynamyc_simbol_procedure(uint8_t *screen, T_din_symbol *pdsym)
{
  int32_t row, col, y, x;
  uint8_t s, a;

  if (pdsym->state_period == 0)
    return;

  if (g_line_cnt == 7)
  {  // Decrease period counter; symbol animation is updated when this counter reaches zero

    if (pdsym->state_cnt == 0)
    {
      // Clear screen
      screen[0]        = 0;
      screen[1]        = 0;
      screen[2]        = 0;
      screen[3]        = 0;
      screen[4]        = 0;
      screen[5]        = 0;
      screen[6]        = 0;
      screen[7]        = 0;

      pdsym->state_cnt = pdsym->state_period;  // Reset period counter

      // Draw symbol at new position

      //  Rendering
      for (row = 0; row < 8; row++)
      {
        y = row + pdsym->y_pos;
        // Check if row is within display bounds vertically
        if ((y >= 0) && (y < 8))
        {
          s = Symbols[pdsym->symbol_num][row];
          for (col = 0; col < 8; col++)
          {
            x = col + pdsym->x_pos;
            // Check if column is within display bounds horizontally
            if ((x >= 0) && (x < 8))
            {
              // Move pixel from source position row, col to screen position x, y
              a         = ((s >> col) & 1) << x;
              screen[y] = screen[y] ^ a;
            }
          }
        }
      }

      // Update symbol position
      pdsym->x_pos += pdsym->x_delta;
      pdsym->y_pos += pdsym->y_delta;

      if (pdsym->step_cnt != 0)
      {
        pdsym->step_cnt--;
      }
      else
      {
        pdsym->step_cnt = pdsym->step_count;
        pdsym->x_pos    = pdsym->start_x;
        pdsym->y_pos    = pdsym->start_y;
      }
    }
    else
    {
      pdsym->state_cnt--;
    }
  }
}

/*-----------------------------------------------------------------------------------------------------

-----------------------------------------------------------------------------------------------------*/
static void Display_rotation_0(uint32_t k, uint8_t *a, uint8_t *c)
{
  *a = red_screen[k];
  *c = green_screen[k];
}

/*-----------------------------------------------------------------------------------------------------

-----------------------------------------------------------------------------------------------------*/
static void Display_rotation_90(uint32_t k, uint8_t *a, uint8_t *c)
{
  *a = (((red_screen[0] >> k) & 1) << 7) | (((red_screen[1] >> k) & 1) << 6) | (((red_screen[2] >> k) & 1) << 5) | (((red_screen[3] >> k) & 1) << 4) | (((red_screen[4] >> k) & 1) << 3) | (((red_screen[5] >> k) & 1) << 2) | (((red_screen[6] >> k) & 1) << 1) | (((red_screen[7] >> k) & 1) << 0);
  *c = (((green_screen[0] >> k) & 1) << 7) | (((green_screen[1] >> k) & 1) << 6) | (((green_screen[2] >> k) & 1) << 5) | (((green_screen[3] >> k) & 1) << 4) | (((green_screen[4] >> k) & 1) << 3) | (((green_screen[5] >> k) & 1) << 2) | (((green_screen[6] >> k) & 1) << 1) | (((green_screen[7] >> k) & 1) << 0);
}

/*-----------------------------------------------------------------------------------------------------

-----------------------------------------------------------------------------------------------------*/
static void Display_rotation_180(uint32_t k, uint8_t *a, uint8_t *c)
{
  uint8_t  b;
  uint32_t i;

  *a = red_screen[7 - k];
  *c = green_screen[7 - k];
  //  Reverse bits
  b  = 0;
  for (i = 0; i < 8; i++)
  {
    b <<= 1;
    if (*a & 1)
      b++;
    *a >>= 1;
  }
  *a = b;
  b  = 0;
  for (i = 0; i < 8; i++)
  {
    b <<= 1;
    if (*c & 1)
      b++;
    *c >>= 1;
  }
  *c = b;
}

/*-----------------------------------------------------------------------------------------------------

-----------------------------------------------------------------------------------------------------*/
static void Display_rotation_270(uint32_t k, uint8_t *a, uint8_t *c)
{
  uint8_t  b;
  uint32_t i, j;
  uint8_t  rr[8];
  uint8_t  gg[8];
  for (j = 0; j < 8; j++)
  {
    //  Reverse bits
    b  = 0;
    *a = red_screen[j];
    for (i = 0; i < 8; i++)
    {
      b <<= 1;
      if (*a & 1)
        b++;
      *a >>= 1;
    }
    rr[j] = b;
  }

  for (j = 0; j < 8; j++)
  {
    //  Reverse bits
    b  = 0;
    *a = green_screen[j];
    for (i = 0; i < 8; i++)
    {
      b <<= 1;
      if (*a & 1)
        b++;
      *a >>= 1;
    }
    gg[j] = b;
  }

  // Rotate by 270 degrees
  *a = (((rr[0] >> k) & 1) << 0) | (((rr[1] >> k) & 1) << 1) | (((rr[2] >> k) & 1) << 2) | (((rr[3] >> k) & 1) << 3) | (((rr[4] >> k) & 1) << 4) | (((rr[5] >> k) & 1) << 5) | (((rr[6] >> k) & 1) << 6) | (((rr[7] >> k) & 1) << 7);

  *c = (((gg[0] >> k) & 1) << 0) | (((gg[1] >> k) & 1) << 1) | (((gg[2] >> k) & 1) << 2) | (((gg[3] >> k) & 1) << 3) | (((gg[4] >> k) & 1) << 4) | (((gg[5] >> k) & 1) << 5) | (((gg[6] >> k) & 1) << 6) | (((gg[7] >> k) & 1) << 7);
}

//------------------------------------------------------------------------------
// SPI send procedure called after each line data transmission via SPI1_send_word
//------------------------------------------------------------------------------
static void Displ_SPI_send_proc(void)
{
  // Disable line output
  TLC5920DLG4_Blank_high();

  // Установка сигналов выбора строки на трех пинах PB8, PB9, PB10 (CSEL0, CSEL1, CSEL2 на чипе TLC5920DLG4)
  GPIOB->ODR = (g_line_cnt << 8);

  // Даем сигнал перемещения данных строки из сдвигового регистра на выход на LED матрицу
  TLC5920DLG4_Latch_high();
  TLC5920DLG4_Latch_low();
  // Включаем сигнал строки
  TLC5920DLG4_Blank_low();

  // Advance to next line
  if (g_line_cnt >= 7)
  {
    g_line_cnt = 0;
  }
  else
  {
    g_line_cnt++;
  }

  // Process dynamic symbol animation after completing full frame (8 lines)
  // Animation is updated at 8-line intervals
  Dynamyc_simbol_procedure(&red_screen[0], &red_dsym);
  Dynamyc_simbol_procedure(&green_screen[0], &green_dsym);
}

//------------------------------------------------------------------------------
// Display state machine handler
//------------------------------------------------------------------------------
void Display_state_machine(void)
{
  uint32_t i;
  uint32_t k;
  int16_t  w;
  uint8_t  a = 0;
  uint8_t  b = 0;
  uint8_t  c = 0;
  uint8_t  spi_data[2];  // Массив для передачи данных через SPI

  // Calculate line number from 0 to 7 for next output
  k = g_line_cnt & 7;

  app_vars.rotated   = ((GPIOA->IDR >> 7) & 2) | ((GPIOA->IDR >> 2) & 1);

  // If rotation is enabled, rotate display by 90 degree increments
  switch (app_vars.rotated & 3)
  {
    case 0:
      Display_rotation_0(k, &a, &c);
      break;
    case 1:
      Display_rotation_90(k, &a, &c);
      break;
    case 2:
      Display_rotation_180(k, &a, &c);
      break;
    case 3:
      Display_rotation_270(k, &a, &c);
      break;
  }

  // Interleave red (a) and green (c) data
  b = 0x01;
  w = 0;
  for (i = 0; i < 8; i++)
  {
    w = w << 1;
    if (a & b)
    {
      w = w | 1;
    }
    w = w << 1;
    if (c & b)
    {
      w = w | 1;
    }
    b = b << 1;
  }

  // Передача 16-битного слова через 8-битный SPI
  // Разделяем 16-битное слово на старший и младший байты
  spi_data[0] = (uint8_t)((w >> 8) & 0xFF);  // Старший байт
  spi_data[1] = (uint8_t)(w & 0xFF);         // Младший байт

  // Передача данных через HAL SPI
  HAL_SPI_Transmit(&hspi1, spi_data, 2, HAL_MAX_DELAY);
  Displ_SPI_send_proc();
}
