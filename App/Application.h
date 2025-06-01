#ifndef __APPLICATION_H
#define __APPLICATION_H

/* Стандартные заголовочные файлы C */
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

/* Заголовочные файлы STM32 HAL */
#include "../Drivers/STM32F1xx_HAL_Driver/Inc/stm32f1xx_hal.h"
#include "../Core/Inc/stm32f1xx_hal_conf.h"
#include "../Core/Inc/stm32f1xx_it.h"
#include "../Core/Inc/gpio.h"
#include "../Core/Inc/can.h"
#include "../Core/Inc/spi.h"
#include "../Core/Inc/main.h"

/* Заголовочные файлы FreeRTOS */
#include "../Core/Inc/FreeRTOSConfig.h"
#include "../Middlewares/Third_Party/FreeRTOS/Source/include/FreeRTOS.h"
#include "../Middlewares/Third_Party/FreeRTOS/Source/include/task.h"
#include "../Middlewares/Third_Party/FreeRTOS/Source/include/queue.h"
#include "../Middlewares/Third_Party/FreeRTOS/Source/include/semphr.h"
#include "cmsis_os.h"

/* Заголовочные файлы приложения */
#include "BitMasks.h"
#include "CAN_IDs.h"
#include "CAN_manager.h"
#include "IO_funcs.h"
#include "LED_display.h"
#include "Symbols.h"
#include "FreeRTOS_static_memory.h"

#define ERROR        (-1)
#define SUCCESS      0
#define BUSY         (-2)

#define BIT(n)       (1u << n)
#define LSHIFT(v, n) (((unsigned int)(v) << n))

// Standard integer types are used from stdint.h instead of custom typedefs:
// uint8_t, int8_t, uint16_t, int16_t, uint32_t, int32_t, uint64_t
// bool from stdbool.h instead of BOOLEAN
// volatile types: volatile uint32_t, volatile uint16_t, volatile uint8_t
// float and double for floating point types

typedef struct
{
  uint32_t node_addr;
  uint32_t rotated;
  uint32_t req_temperature;

} T_app_vars;

enum T_led_states
{
  SIG_off    = 0,
  SIG_on     = 1,
  SIG_ret    = 2,
  SIG_repeat = 3,
  SIG_stop   = 4,
};

typedef struct
{
  enum T_led_states state;
  uint32_t          duration;
  uint32_t          value;
} T_sig_pattern;

typedef struct
{
  uint32_t from_sym;
  uint32_t to_sym;

} T_remap_sym;

extern T_app_vars  app_vars;
extern T_remap_sym remap_array[];

/* CAN Display Protocol Command Handlers */
void Handle_CAN_SetSymbol(const uint8_t *data);
void Handle_CAN_SetSymbolPattern1(const uint8_t *data);
void Handle_CAN_SetSymbolPattern2(const uint8_t *data);
void Handle_CAN_DynamicSymbolSet1(const uint8_t *data);
void Handle_CAN_DynamicSymbolSet2(const uint8_t *data);
void Handle_CAN_DynamicSymbolSet3(const uint8_t *data);
void Handle_CAN_DynamicSymbolSet4(const uint8_t *data);
void Handle_CAN_SetRedScreen(const uint8_t *data);
void Handle_CAN_SetGreenScreen(const uint8_t *data);

/* Dynamic symbol temporary storage */
extern T_din_symbol tmp_dsym;

extern void Main_cycle(void);

/* Флаг для отладки - отправка цифр по CAN */
extern volatile uint32_t can_debug_send_digits;

#endif
