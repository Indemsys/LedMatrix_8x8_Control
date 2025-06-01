#include "Application.h"

uint32_t   display_idle_mode;  // Flag indicating display is in idle/demo mode
T_app_vars app_vars;

/* Флаг для отладки - включение отправки цифр */
volatile uint32_t can_debug_send_digits = 0;
static void       SendDigitViaCAN(uint32_t tick_counter);
static void       AddCANStatusIndicator(uint8_t *green_data, uint8_t *red_data);
/*-----------------------------------------------------------------------------------------------------
  Function for drawing horizontal line in symbol_data buffer
-----------------------------------------------------------------------------------------------------*/
static void Display_draw_hor_line(uint8_t *symbol_data, uint8_t row)
{
  if (row < 8)
  {
    symbol_data[row] |= 0xFF;  // Draw full horizontal line (all pixels in row)
  }
}

/*-----------------------------------------------------------------------------------------------------

-----------------------------------------------------------------------------------------------------*/
static void HandleDisplayIdleMode(uint32_t tick_counter)
{
  // New logic: green line draws symbol 0.5 sec → symbol stays 1 sec → red line erases 0.5 sec
  // Full cycle: 2 seconds (0.5 sec drawing + 1 sec display + 0.5 sec erasing)
  uint32_t cycle_time    = configTICK_RATE_HZ * 2;            // 2 seconds in ticks
  uint32_t cycle_pos     = tick_counter % cycle_time;         // position in current cycle
  uint32_t symbol_index  = (tick_counter / cycle_time) % 10;  // symbol index in remap_array

  uint32_t draw_time     = configTICK_RATE_HZ / 2;            // 0.5 sec for drawing
  uint32_t display_time  = configTICK_RATE_HZ;                // 1 sec display
  uint32_t erase_time    = configTICK_RATE_HZ / 2;            // 0.5 sec for erasing
  // Get current symbol data
  uint32_t symbol_code   = remap_array[symbol_index].to_sym;
  uint32_t remapped_code = Remap_sym_code(symbol_code);
  uint8_t  green_data[8];  // Buffer for green color
  uint8_t  red_data[8];    // Buffer for red color
  if (cycle_pos < draw_time)
  {
    // Phase 1 (first 0.5 seconds): green line draws symbol
    uint32_t line_ticks = draw_time / 8;    // ticks per line position
    if (line_ticks == 0) line_ticks = 1;    // Protection from division by zero
    uint32_t pos = cycle_pos / line_ticks;  // position from 0 to 7

    // Fill green buffer
    for (uint32_t i = 0; i < 8; i++)
    {
      if (pos >= 8 || i >= (7 - pos))
      {
        // On line and below - show already drawn part of symbol in green
        green_data[i] = Symbols[remapped_code][i];
      }
      else
      {
        // Above line - not drawn yet (empty)
        green_data[i] = 0;
      }
    }

    // Add green line to green buffer only if line is still on screen
    if (pos < 8)
    {
      uint32_t line_row = 7 - pos;
      Display_draw_hor_line(green_data, line_row);
    }  // Clear red buffer
    for (uint32_t i = 0; i < 8; i++)
    {
      red_data[i] = 0;
    }

    // Add CAN status indicator before displaying
    AddCANStatusIndicator(green_data, red_data);

    // Show green buffer
    Display_copy_to_green_screen(green_data);
    Display_copy_to_red_screen(red_data);
  }
  else if (cycle_pos < (draw_time + display_time))
  {  // Phase 2 (next 1 second): symbol is fully displayed in green
    for (uint32_t i = 0; i < 8; i++)
    {
      green_data[i] = Symbols[remapped_code][i];
      red_data[i]   = 0;  // Red buffer empty
    }

    // Add CAN status indicator before displaying
    AddCANStatusIndicator(green_data, red_data);

    // Show full symbol in green without line
    Display_copy_to_green_screen(green_data);
    Display_copy_to_red_screen(red_data);
  }
  else
  {
    // Phase 3 (last 0.5 seconds): red line erases symbol
    uint32_t erase_pos  = cycle_pos - (draw_time + display_time);
    uint32_t line_ticks = erase_time / 8;   // ticks per line position
    if (line_ticks == 0) line_ticks = 1;    // Protection from division by zero
    uint32_t pos = erase_pos / line_ticks;  // position from 0 to 7
    if (pos > 7) pos = 7;                   // Limit maximum value

    // Red line moves from bottom to top (pos=0 -> row 7, pos=7 -> row 0)
    uint32_t line_row = 7 - pos;

    // Fill green buffer (remaining part of symbol)
    for (uint32_t i = 0; i < 8; i++)
    {
      if (i < line_row)
      {
        // Above line - show remaining part of symbol in green
        green_data[i] = Symbols[remapped_code][i];
      }
      else
      {
        // On line and below - erased (empty)
        green_data[i] = 0;
      }
    }

    // Fill red buffer with line only
    for (uint32_t i = 0; i < 8; i++)
    {
      red_data[i] = 0;
    }
    Display_draw_hor_line(red_data, line_row);

    // Add CAN status indicator before displaying
    AddCANStatusIndicator(green_data, red_data);

    // Show both buffers
    Display_copy_to_green_screen(green_data);
    Display_copy_to_red_screen(red_data);
  }
}

/*-----------------------------------------------------------------------------------------------------
 * Function: AddCANStatusIndicator
 *
 * Description: Добавляет индикатор статуса CAN шины в правый нижний угол экрана
 *              Использует минимальный код для экономии Flash памяти
 *
 * Input:       green_data - буфер зеленого экрана
 *              red_data - буфер красного экрана
 *
 * Output:      Нет (модифицирует буферы напрямую)
 *
 * Called by:   - HandleDisplayIdleMode() во всех фазах анимации
 *
 * Note:        Цветовое кодирование: нет пикселя=норма, красный=bus off,
 *              зеленый=нет ACK, желтый=восстановление
 *-----------------------------------------------------------------------------------------------------*/
static void AddCANStatusIndicator(uint8_t *green_data, uint8_t *red_data)
{
  const ONBUS_Status_t    *onbus = Get_ONBUS_status();
  const CAN_Error_Stats_t *stats = CAN_get_error_stats();

  // If ACK received and bus connected - no indicator (normal operation)
  if (onbus->ack_received && !stats->bus_off_count)
  {
    return;  // No indicator needed
  }

  // Determine indicator color based on CAN status
  if (stats->recovery_in_progress)
  {
    // Recovery in progress - yellow pixel (red + green)
    red_data[7] |= 0x01;
    green_data[7] |= 0x01;
  }
  else if (stats->bus_off_count > 0)
  {
    // Bus disconnected (bus off) - red pixel
    red_data[7] |= 0x01;
  }
  else if (!onbus->ack_received)
  {
    // Bus connected but no ACK - green pixel
    green_data[7] |= 0x01;
  }
}

/*------------------------------------------------------------------------------
  Initialization task
 ------------------------------------------------------------------------------*/
void Main_cycle(void)
{
  uint32_t tick_counter;  // Counter for timing calculations

  app_vars.node_addr = GPIOA->IDR & 0x03;
  if (app_vars.node_addr == 3) can_debug_send_digits = 1;

  // Create FreeRTOS tasks using static allocation instead of dynamic
  xCanTxTaskHandle = xTaskCreateStatic(
  Task_can_transmiter,     // Task function
  "CANTx",                 // Task name
  CAN_TX_TASK_STACK_SIZE,  // Stack size
  NULL,                    // Parameters
  osPriorityAboveNormal,   // Priority
  xCanTxTaskStack,         // Stack buffer
  &xCanTxTaskTCBBuffer     // TCB buffer
  );

  xCanRxTaskHandle = xTaskCreateStatic(
  Task_can_receiver,       // Task function
  "CANRx",                 // Task name
  CAN_RX_TASK_STACK_SIZE,  // Stack size
  NULL,                    // Parameters
  osPriorityAboveNormal,   // Priority
  xCanRxTaskStack,         // Stack buffer
  &xCanRxTaskTCBBuffer     // TCB buffer
  );

  vTaskDelay(pdMS_TO_TICKS(10));

  Display_set_symbol(12, 2);  // Display initial symbol

  tick_counter      = 0;
  display_idle_mode = 1;

  while (1)
  {
    IWDG->KR = 0xAAAA;  // Reset IWDG watchdog

    tick_counter++;

    if (display_idle_mode)
    {
      HandleDisplayIdleMode(tick_counter);
    }

    // Вызов функции отправки цифр по CAN (работает только если установлен флаг через отладчик)
    SendDigitViaCAN(tick_counter);

    Display_state_machine();
    vTaskDelay(1);  // Execute every OS tick instead of every 10ms
  }
}

/*--------------------------- CAN Display Protocol Command Handlers -------------------*/

/* Temporary storage for multi-part dynamic symbol configuration */
T_din_symbol tmp_dsym;

/*-----------------------------------------------------------------------------------------------------
 * Function: Handle_CAN_SetSymbol
 *
 * Description: Обрабатывает команду PDISPLx_SET_SYMBOL - установка символа на дисплей
 *              Отключает режим idle дисплея и устанавливает указанный символ с заданным цветом
 *
 * Input:       data - массив данных CAN сообщения
 *              data[1] - код символа для отображения
 *              data[2] - цвет символа (0=выкл, 1=зел, 2=кр, 3=жел)
 *
 * Output:      Нет
 *
 * Called by:   - Task_can_receiver() при получении команды PDISPLx_SET_SYMBOL
 *
 * Note:        Функция отключает демо-режим дисплея (display_idle_mode = 0)
 *              Вызывает Display_set_symbol() для установки символа на дисплее
 *-----------------------------------------------------------------------------------------------------*/
void Handle_CAN_SetSymbol(const uint8_t *data)
{
  extern uint32_t display_idle_mode;
  display_idle_mode = 0;
  Display_set_symbol(data[1], data[2]);
}

/*-----------------------------------------------------------------------------------------------------
 * Function: Handle_CAN_SetSymbolPattern1
 *
 * Description: Обрабатывает команду PDISPLx_SET_SYMBOL_PTRN1 - загрузка первой части паттерна символа
 *              Копирует первые 4 байта растрового изображения символа
 *
 * Input:       data - массив данных CAN сообщения
 *              data[1] - номер символа для редактирования (0-255)
 *              data[4-7] - первые 4 строки растрового изображения символа
 *
 * Output:      Нет
 *
 * Called by:   - Task_can_receiver() при получении команды PDISPLx_SET_SYMBOL_PTRН1
 *
 * Note:        Символ состоит из 8 байт (8x8 пикселей), эта функция загружает строки 0-3
 *              Для полной загрузки символа требуется также вызов Handle_CAN_SetSymbolPattern2
 *-----------------------------------------------------------------------------------------------------*/
void Handle_CAN_SetSymbolPattern1(const uint8_t *data)
{
  memcpy(&Symbols[data[1]][0], &data[4], 4);
}

/*-----------------------------------------------------------------------------------------------------
 * Function: Handle_CAN_SetSymbolPattern2
 *
 * Description: Обрабатывает команду PDISPLx_SET_SYMBOL_PTRН2 - загрузка второй части паттерна символа
 *              Копирует последние 4 байта растрового изображения символа
 *
 * Input:       data - массив данных CAN сообщения
 *              data[1] - номер символа для редактирования (0-255)
 *              data[4-7] - последние 4 строки растрового изображения символа
 *
 * Output:      Нет
 *
 * Called by:   - Task_can_receiver() при получении команды PDISPLx_SET_SYMBOL_PTRН2
 *
 * Note:        Символ состоит из 8 байт (8x8 пикселей), эта функция загружает строки 4-7
 *              Должна вызываться после Handle_CAN_SetSymbolPattern1 для полной загрузки символа
 *-----------------------------------------------------------------------------------------------------*/
void Handle_CAN_SetSymbolPattern2(const uint8_t *data)
{
  memcpy(&Symbols[data[1]][4], &data[4], 4);
}

/*-----------------------------------------------------------------------------------------------------
 * Function: Handle_CAN_DynamicSymbolSet1
 *
 * Description: Обрабатывает команду PDISPLx_DIN_SYMBOL_SET1 - первая часть настройки динамического символа
 *              Устанавливает номер символа, период состояния и количество шагов анимации
 *
 * Input:       data - массив данных CAN сообщения
 *              data[1] - номер символа для анимации
 *              data[2-3] - период состояния (16-бит, младший байт первый)
 *              data[4-5] - количество шагов анимации (16-бит, младший байт первый)
 *
 * Output:      Нет
 *
 * Called by:   - Task_can_receiver() при получении команды PDISPLx_DIN_SYMBOL_SET1
 *
 * Note:        Данные сохраняются во временной структуре tmp_dsym
 *              Требуются все 4 команды SET1-SET4 для полной настройки динамического символа
 *-----------------------------------------------------------------------------------------------------*/
void Handle_CAN_DynamicSymbolSet1(const uint8_t *data)
{
  tmp_dsym.symbol_num   = data[1];
  tmp_dsym.state_period = *(short *)(&data[2]);
  tmp_dsym.step_count   = *(short *)(&data[4]);
}

/*-----------------------------------------------------------------------------------------------------
 * Function: Handle_CAN_DynamicSymbolSet2
 *
 * Description: Обрабатывает команду PDISPLx_DIN_SYMBOL_SET2 - вторая часть настройки динамического символа
 *              Устанавливает приращения координат по X и Y для каждого шага анимации
 *
 * Input:       data - массив данных CAN сообщения
 *              data[2-3] - приращение X координаты за шаг (16-бит, младший байт первый)
 *              data[4-5] - приращение Y координаты за шаг (16-бит, младший байт первый)
 *
 * Output:      Нет
 *
 * Called by:   - Task_can_receiver() при получении команды PDISPLx_DIN_SYMBOL_SET2
 *
 * Note:        Данные сохраняются во временной структуре tmp_dsym
 *              Должна вызываться после Handle_CAN_DynamicSymbolSet1
 *-----------------------------------------------------------------------------------------------------*/
void Handle_CAN_DynamicSymbolSet2(const uint8_t *data)
{
  tmp_dsym.x_delta = *(short *)(&data[2]);
  tmp_dsym.y_delta = *(short *)(&data[4]);
}

/*-----------------------------------------------------------------------------------------------------
 * Function: Handle_CAN_DynamicSymbolSet3
 *
 * Description: Обрабатывает команду PDISPLx_DIN_SYMBOL_SET3 - третья часть настройки динамического символа
 *              Устанавливает начальные координаты символа на дисплее
 *
 * Input:       data - массив данных CAN сообщения
 *              data[2-3] - начальная X координата (16-бит, младший байт первый)
 *              data[4-5] - начальная Y координата (16-бит, младший байт первый)
 *
 * Output:      Нет
 *
 * Called by:   - Task_can_receiver() при получении команды PDISPLx_DIN_SYMBOL_SET3
 *
 * Note:        Данные сохраняются во временной структуре tmp_dsym
 *              Должна вызываться после Handle_CAN_DynamicSymbolSet2
 *-----------------------------------------------------------------------------------------------------*/
void Handle_CAN_DynamicSymbolSet3(const uint8_t *data)
{
  tmp_dsym.start_x = *(short *)(&data[2]);
  tmp_dsym.start_y = *(short *)(&data[4]);
}

/*-----------------------------------------------------------------------------------------------------
 * Function: Handle_CAN_DynamicSymbolSet4
 *
 * Description: Обрабатывает команду PDISPLx_DIN_SYMBOL_SET4 - завершающая настройка динамического символа
 *              Отключает режим idle дисплея и запускает анимацию с собранными параметрами
 *
 * Input:       data - массив данных CAN сообщения
 *              data[1] - цвет символа (0=выкл, 1=зел, 2=кр, 3=жел)
 *
 * Output:      Нет
 *
 * Called by:   - Task_can_receiver() при получении команды PDISPLx_DIN_SYMBOL_SET4
 *
 * Note:        Функция отключает демо-режим дисплея (display_idle_mode = 0)
 *              Использует все параметры, собранные командами SET1-SET3
 *              Вызывает Set_dinamic_symbol() для запуска анимации
 *              Должна вызываться последней после SET1, SET2, SET3
 *-----------------------------------------------------------------------------------------------------*/
void Handle_CAN_DynamicSymbolSet4(const uint8_t *data)
{
  extern uint32_t display_idle_mode;
  display_idle_mode = 0;
  Set_dinamic_symbol(tmp_dsym.symbol_num, tmp_dsym.state_period, tmp_dsym.step_count,
                     tmp_dsym.x_delta, tmp_dsym.y_delta, tmp_dsym.start_x, tmp_dsym.start_y, data[1]);
}

/*-----------------------------------------------------------------------------------------------------
 * Function: Handle_CAN_SetRedScreen
 *
 * Description: Обрабатывает команду PDISPLx_SET_RED_SYMB - копирование данных в красный экран
 *              Отключает режим idle дисплея и копирует 8 байт данных в красный буфер экрана
 *
 * Input:       data - массив данных CAN сообщения
 *              data[0-7] - 8 байт данных для копирования в красный экран
 *
 * Output:      Нет
 *
 * Called by:   - Task_can_receiver() при получении команды с ID PDISPLx_SET_RED_SYMB
 *
 * Note:        Функция отключает демо-режим дисплея (display_idle_mode = 0)
 *              Вызывает Display_copy_to_red_screen() для копирования данных
 *              Команда имеет отдельный CAN ID, отличный от PDISPLx_REQ
 *-----------------------------------------------------------------------------------------------------*/
void Handle_CAN_SetRedScreen(const uint8_t *data)
{
  extern uint32_t display_idle_mode;
  display_idle_mode = 0;
  Display_copy_to_red_screen((uint8_t *)data);
}

/*-----------------------------------------------------------------------------------------------------
 * Function: Handle_CAN_SetGreenScreen
 *
 * Description: Обрабатывает команду PDISPLx_SET_GREEN_SYMB - копирование данных в зеленый экран
 *              Отключает режим idle дисплея и копирует 8 байт данных в зеленый буфер экрана
 *
 * Input:       data - массив данных CAN сообщения
 *              data[0-7] - 8 байт данных для копирования в зеленый экран
 *
 * Output:      Нет
 *
 * Called by:   - Task_can_receiver() при получении команды с ID PDISPLx_SET_GREEN_SYMB
 *
 * Note:        Функция отключает демо-режим дисплея (display_idle_mode = 0)
 *              Вызывает Display_copy_to_green_screen() для копирования данных
 *              Команда имеет отдельный CAN ID, отличный от PDISPLx_REQ
 *-----------------------------------------------------------------------------------------------------*/
void Handle_CAN_SetGreenScreen(const uint8_t *data)
{
  extern uint32_t display_idle_mode;
  display_idle_mode = 0;
  Display_copy_to_green_screen((uint8_t *)data);
}

/*-----------------------------------------------------------------------------------------------------
 * Function: SendDigitViaCAN
 *
 * Description: Отправляет коды цифр от 0 до 9 по CAN каждые 0.25 секунды
 *              Функция работает только если через отладчик установлен флаг can_debug_send_digits = 1
 *
 * Input:       tick_counter - счетчик тиков для отслеживания времени
 *
 * Output:      Нет
 *
 * Called by:   - Main_cycle() для периодической отправки цифр
 *
 * Note:        Для активации функции нужно установить флаг can_debug_send_digits = 1 через отладчик
 *              Использует PDISPLx_SET_SYMBOL для отправки команды установки символа
 *              Можно наблюдать отображение цифр на соседнем дисплее
 *-----------------------------------------------------------------------------------------------------*/
static void SendDigitViaCAN(uint32_t tick_counter)
{
  static uint32_t last_send_time = 0;
  static uint8_t  current_digit  = 0;
  T_can_msg       can_msg;

  // Если флаг отладки не установлен - не отправляем ничего
  if (can_debug_send_digits == 0)
  {
    return;
  }

  // Проверяем, прошло ли 0.25 секунды с последней отправки
  // configTICK_RATE_HZ / 4 = количество тиков в 0.25 секунды
  if ((tick_counter - last_send_time) >= (configTICK_RATE_HZ / 4))
  {
    // Формируем CAN сообщение
    can_msg.format = EXTENDED_FORMAT;
    can_msg.type   = DATA_FRAME;
    can_msg.id     = PDISPLx_REQ | ((0) << 20);  // Отправляем соседнему узлу 0
    can_msg.len    = 8;

    // Очищаем данные
    memset(can_msg.data, 0, 8);

    // Заполняем данные для команды PDISPLx_SET_SYMBOL
    can_msg.data[0] = PDISPLx_SET_SYMBOL;  // Команда установки символа
    can_msg.data[1] = current_digit;       // Код символа (0-9)
    can_msg.data[2] = 1;                   // Цвет символа (1 = зеленый)

    // Отправляем сообщение
    CAN_send_or_post_msg(&can_msg, 10);

    // Обновляем время последней отправки
    last_send_time = tick_counter;

    // Увеличиваем номер цифры циклически от 0 до 9
    current_digit  = (current_digit + 1) % 10;
  }
}
