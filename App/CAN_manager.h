#ifndef __CAN_MANAGER_H
#define __CAN_MANAGER_H

typedef enum
{
  CAN_OK = 0,                 // No error
  CAN_NOT_IMPLEMENTED_ERROR,  // Function has not been implemented
  CAN_MEM_POOL_INIT_ERROR,    // Memory pool initialization error
  CAN_BAUDRATE_ERROR,         // Baudrate was not set
  CAN_TX_BUSY_ERROR,          // Transmitting hardware busy
  CAN_OBJECTS_FULL_ERROR,     // No more rx or tx objects available
  CAN_ALLOC_MEM_ERROR,        // Unable to allocate memory from pool
  CAN_DEALLOC_MEM_ERROR,      // Unable to deallocate memory
  CAN_TIMEOUT_ERROR,          // Timeout expired
  CAN_UNEXIST_CTRL_ERROR,     // Controller does not exist
  CAN_UNEXIST_CH_ERROR,       // Channel does not exist
} T_can_err;

/*--------------------------- CAN Buffer Configuration -------------------*/
/*
 * ОБЩЕЕ ИСПОЛЬЗОВАНИЕ ПАМЯТИ CAN:
 *
 * Статический пул: CAN_CTRL_MAX_NUM * (SEND + RECV + LOG) = 1 * (2 + 8 + 2) = 12 сообщений
 * Размер сообщения: sizeof(T_can_msg) = 16 байт
 * Общий расход RAM: 12 * 16 = 192 байта для пула сообщений
 *                 + 2 * 4 = 8 байт для TX очереди указателей
 *                 + 8 * 4 = 32 байта для RX очереди указателей
 *                 + размер статических буферов FreeRTOS
 * ИТОГО: ~240 байт RAM для CAN подсистемы
 */

/**
 * @brief Размер очереди передачи CAN сообщений
 *
 * Определяет максимальное количество сообщений, которые могут ожидать
 * отправки в программном буфере. Небольшое значение (2) обусловлено тем,
 * что LED дисплей редко отправляет сообщения (в основном ONBUS).
 *
 * Используется для:
 * - Создания FreeRTOS очереди can_tx_queue
 * - Расчета размера общего пула памяти для CAN сообщений
 */
#define CAN_NO_SEND_OBJECTS 2

/**
 * @brief Размер очереди приема CAN сообщений
 *
 * Определяет максимальное количество принятых сообщений, которые могут
 * храниться в программном буфере до их обработки пользовательским кодом.
 * Значение 8 обеспечивает буферизацию команд управления дисплеем при
 * пиковых нагрузках CAN шины.
 *
 * Используется для:
 * - Создания FreeRTOS очереди can_rx_queue
 * - Расчета размера общего пула памяти для CAN сообщений
 */
#define CAN_NO_RECV_OBJECTS 8

/**
 * @brief Резерв для будущего функционала логирования CAN сообщений
 *
 * В текущей реализации активно НЕ используется, но резервирует память
 * в общем пуле для возможного добавления функций логирования/диагностики.
 * Можно установить в 0 для экономии RAM, если логирование не планируется.
 *
 * Используется только для:
 * - Расчета размера общего пула памяти для CAN сообщений
 */
#define CAN_NO_LOG_OBJECTS  2

/*--------------------------- CAN Controller Configuration ---------------*/

/**
 * @brief Номер CAN канала для данного контроллера
 *
 * В STM32F1xx используется CAN1 (единственный доступный CAN контроллер).
 * Значение используется в функциях диагностики и может быть расширено
 * для поддержки многоканальных систем в будущем.
 */
#define CAN_CHANL           1

/**
 * @brief Максимальное количество CAN контроллеров в системе
 *
 * Используется для расчета общего размера пула памяти для всех
 * CAN контроллеров. В текущей системе используется только один
 * контроллер (CAN1), поэтому значение = 1.
 *
 * Общий размер пула = CAN_CTRL_MAX_NUM * (SEND + RECV + LOG) = 1 * 12 = 12 сообщений
 */
#define CAN_CTRL_MAX_NUM 1

// CAN message object structure
typedef struct
{
  uint32_t id;       // 29 bit identifier
  uint8_t  data[8];  // Data field
  uint8_t  len;      // Length of data field in bytes
  uint8_t  format;   // 0 - STANDARD, 1- EXTENDED IDENTIFIER
  uint8_t  type;     // 0 - DATA FRAME, 1 - REMOTE FRAME
} T_can_msg;

// Symbolic names for formats of CAN message
typedef enum
{
  STANDARD_FORMAT = 0,
  EXTENDED_FORMAT
} T_can_id_type;

// Symbolic names for frame types
typedef enum
{
  DATA_FRAME = 0,
  REMOTE_FRAME
} T_can_frame_type;

/* Статистика ошибок CAN */
typedef struct {
  uint32_t bus_off_count;        // Счетчик Bus-Off состояний
  uint32_t error_warning_count;  // Счетчик Error Warning
  uint32_t error_passive_count;  // Счетчик Error Passive
  uint32_t ack_error_count;      // Счетчик ACK ошибок
  uint32_t stuff_error_count;    // Счетчик Stuff ошибок
  uint32_t form_error_count;     // Счетчик Form ошибок
  uint32_t crc_error_count;      // Счетчик CRC ошибок
  uint32_t tx_terr0_count;       // Счетчик ошибок передачи MailBox 0
  uint32_t tx_terr1_count;       // Счетчик ошибок передачи MailBox 1
  uint32_t tx_terr2_count;       // Счетчик ошибок передачи MailBox 2
  uint32_t recovery_attempts;    // Счетчик попыток восстановления
  uint32_t last_error_time;      // Время последней ошибки (в тиках)
  uint8_t  consecutive_errors;   // Счетчик последовательных ошибок
  uint8_t  recovery_in_progress; // Флаг процесса восстановления
} CAN_Error_Stats_t;

/*--------------------------- ONBUS Status Tracking -------------------*/

/**
 * @brief Структура для отслеживания статуса ONBUS сообщений
 *
 * Используется для мониторинга получения ACK битов на отправленные
 * ONBUS сообщения и определения активности CAN шины.
 */
typedef struct {
  uint8_t pending;              // Ожидает подтверждения (ACK)
  uint32_t last_send_time;      // Время последней отправки (в тиках)
  uint32_t retry_count;         // Количество попыток отправки
  uint8_t ack_received;         // Получено подтверждение (ACK) от шины
} ONBUS_Status_t;

T_can_err    CAN_init(void);
T_can_err    CAN_release_init_mode(void);
T_can_err    CAN_send_or_post_msg(T_can_msg *msg, uint16_t timeout);
T_can_err    CAN_pull_msg_from_mbox(T_can_msg *msg, uint16_t timeout);
unsigned int CAN_get_errors(uint32_t chanel);
void         CAN_set_32bit_filter_mask(uint16_t bank, uint32_t filter, uint32_t mask);

/* FreeRTOS task functions - take void pointer parameter */
void Task_can_transmiter(void *pvParameters);
void Task_can_receiver(void *pvParameters);

/* Memory management functions */
T_can_msg *alloc_can_msg(void);
void       free_can_msg(T_can_msg *msg);

/* Error statistics functions */
const CAN_Error_Stats_t* CAN_get_error_stats(void);
void                     CAN_reset_error_stats(void);

/* ONBUS status functions */
uint8_t Check_ONBUS_ACK_timeout(void);
uint8_t Is_CAN_bus_active(uint32_t can_err);
ONBUS_Status_t* Get_ONBUS_status(void);

#endif
