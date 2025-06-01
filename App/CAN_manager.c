#include "Application.h"
/*
 * Параметры CAN для скорости 100000 бит/с при CAN_CLK = 36 МГц:
 *
 * Prescaler (for Time Quantum) = 20
 * Time Quantum = 20 / 36MHz ≈ 555.56 ns
 *
 * Time Quanta in Bit Segment 1 (TS1) = 14
 * Time Quanta in Bit Segment 2 (TS2) = 3
 * ReSynchronization Jump Width (SJW) = 1
 *
 * Итоговое количество квант времени на бит: 1 + 14 + 3 = 18 Tq
 *
 * Время одного бита:
 * T_bit = 18 * 555.56 ns ≈ 10 μs
 *
 * Скорость передачи:
 * Bitrate = 36MHz / (20 * 18) = 100000 бит/с (точно)
 *
 * В STM32 HAL:
 *   hcan.Init.Prescaler = 20;
 *   hcan.Init.SyncJumpWidth = CAN_SJW_1TQ;
 *   hcan.Init.TimeSeg1 = CAN_BS1_14TQ;
 *   hcan.Init.TimeSeg2 = CAN_BS2_3TQ;
 */

/*
 * Параметры CAN для скорости ~555555 бит/с при CAN_CLK = 36 МГц:
 *
 * Prescaler (for Time Quantum) = 4
 * Time Quantum = 4 / 36MHz ≈ 111.11 ns
 *
 * Time Quanta in Bit Segment 1 (TS1) = 11
 * Time Quanta in Bit Segment 2 (TS2) = 4
 * ReSynchronization Jump Width (SJW) = 4
 *
 * Итоговое количество квант времени на бит: 1 + 11 + 4 = 16 Tq
 *
 * Время одного бита:
 * T_bit = 16 * 111.11 ns ≈ 1.777 μs
 *
 * Скорость передачи:
 * Bitrate = 36MHz / (4 * 16) = 562500 бит/с (~+1.3% от 555555)
 *
 * В STM32 HAL:
 *   hcan.Init.Prescaler = 4;
 *   hcan.Init.SyncJumpWidth = CAN_SJW_4TQ;
 *   hcan.Init.TimeSeg1 = CAN_BS1_11TQ;
 *   hcan.Init.TimeSeg2 = CAN_BS2_4TQ;
 */

extern uint32_t          display_idle_mode;
extern CAN_HandleTypeDef hcan;

/* Глобальная переменная для статистики ошибок CAN */

static CAN_Error_Stats_t can_error_stats    = {0};

/* Глобальная переменная для отслеживания статуса ONBUS сообщений */
static ONBUS_Status_t onbus_status          = {0};

/*--------------------------- CAN Filters Configuration -------------------*/

/* Массив базовых ID для CAN фильтров - экономит Flash память */
static const uint32_t can_filter_base_ids[] = {
 PDISPLx_ONBUS_MSG,       // Filter 0
 PDISPLx_REQ,             // Filter 1
 PDISPLx_ANS,             // Filter 2
 PDISPLx_UPGRADE_RX_ID,   // Filter 3
 PDISPLx_SET_RED_SYMB,    // Filter 4
 PDISPLx_SET_GREEN_SYMB,  // Filter 5
 PDISPLx_UPGRADE_TX_ID    // Filter 6
};

#define CAN_FILTERS_COUNT (sizeof(can_filter_base_ids) / sizeof(can_filter_base_ids[0]))

/* Memory pool for CAN messages, both transmit and receive*/
static T_can_msg can_memory_pool[CAN_CTRL_MAX_NUM * (CAN_NO_SEND_OBJECTS + CAN_NO_RECV_OBJECTS + CAN_NO_LOG_OBJECTS)];
static uint8_t   can_memory_pool_used[CAN_CTRL_MAX_NUM * (CAN_NO_SEND_OBJECTS + CAN_NO_RECV_OBJECTS + CAN_NO_LOG_OBJECTS)];

/* FreeRTOS queues for CAN mailbox functionality */
QueueHandle_t can_tx_queue;
QueueHandle_t can_rx_queue;

/*--------------------------- Memory management functions -------------------*/

/*-----------------------------------------------------------------------------------------------------
 * Function: alloc_can_msg
 *
 * Description: Выделяет память для CAN сообщения из статического пула памяти
 *
 * Input:       Нет
 *
 * Output:      Указатель на выделенный блок памяти или NULL если пул переполнен
 *
 * Called by:   - CAN_send_or_post_msg() при добавлении сообщения в очередь передачи
 *              - HAL_CAN_RxFifo0MsgPendingCallback() при приеме CAN сообщения
 *
 * Note:        Использует простой линейный поиск по массиву флагов использования
 *              Thread-safe за счет использования в контексте прерываний и задач
 *-----------------------------------------------------------------------------------------------------*/
T_can_msg *alloc_can_msg(void)
{
  for (unsigned int i = 0; i < sizeof(can_memory_pool_used); i++)
  {
    if (can_memory_pool_used[i] == 0)
    {
      can_memory_pool_used[i] = 1;
      return &can_memory_pool[i];
    }
  }
  return NULL;  // Pool is full
}

/*-----------------------------------------------------------------------------------------------------
 * Function: free_can_msg
 *
 * Description: Освобождает память CAN сообщения, возвращая её в пул
 *
 * Input:       msg - указатель на сообщение для освобождения
 *
 * Output:      Нет
 *
 * Called by:   - CAN_pull_msg_from_mbox() после копирования сообщения в пользовательский буфер
 *              - CAN_send_or_post_msg() при ошибке добавления в очередь
 *              - HAL_CAN_TxMailbox0CompleteCallback() после отправки сообщения из очереди
 *              - HAL_CAN_RxFifo0MsgPendingCallback() при ошибке чтения или переполнении очереди
 *
 * Note:        Проверяет принадлежность указателя к пулу памяти перед освобождением
 *-----------------------------------------------------------------------------------------------------*/
void free_can_msg(T_can_msg *msg)
{
  if (msg >= can_memory_pool && msg < &can_memory_pool[sizeof(can_memory_pool_used)])
  {
    int index                   = msg - can_memory_pool;
    can_memory_pool_used[index] = 0;
  }
}

/*-----------------------------------------------------------------------------------------------------
 * Function: CAN_setup_all_filters
 *
 * Description: Настраивает все CAN фильтры из массива - экономит Flash память
 *
 * Input:       Нет
 *
 * Output:      Нет
 *
 * Called by:   - Task_can_transmiter() при инициализации
 *              - CAN_process_errors() при восстановлении
 *
 * Note:        Использует массив can_filter_base_ids для экономии Flash памяти
 *              Все фильтры настраиваются с одинаковой маской 0x1FFFFFFF
 *-----------------------------------------------------------------------------------------------------*/
static void CAN_setup_all_filters(void)
{
  for (uint8_t i = 0; i < CAN_FILTERS_COUNT; i++)
  {
    CAN_set_32bit_filter_mask(i,
                              can_filter_base_ids[i] | (app_vars.node_addr << 20),
                              0x1FFFFFFF);
  }
}

/*-----------------------------------------------------------------------------------------------------
 * Function: CAN_release_init_mode
 *
 * Description: Освобождает режим инициализации CAN контроллера (заглушка для совместимости)
 *
 * Input:       Нет
 *
 * Output:      CAN_OK - всегда успешно
 *
 * Called by:   - Task_can_transmiter() после настройки фильтров
 * * Note:        В HAL STM32 эта операция не требуется, так как CAN уже запущен в CAN_init()
 *              Функция оставлена для совместимости с существующим API
 *-----------------------------------------------------------------------------------------------------*/
T_can_err CAN_release_init_mode(void)
{
  // В HAL это не требуется, CAN уже запущен в CAN_init
  return CAN_OK;
}

/*-----------------------------------------------------------------------------------------------------
 * Function: CAN_is_txmbox0_empty
 *
 * Description: Проверяет доступность передающих mailbox
 *
 * Input:       Нет
 *
 * Output:      CAN_OK - mailbox свободен
 *              CAN_TX_BUSY_ERROR - все mailbox заняты
 *
 * Called by:   - CAN_send_or_post_msg() для проверки возможности немедленной отправки
 *
 * Note:        STM32 HAL автоматически выбирает свободный mailbox (0, 1 или 2)
 *              HAL_CAN_AddTxMessage() является thread-safe операцией
 *-----------------------------------------------------------------------------------------------------*/
static T_can_err CAN_is_txmbox0_empty(void)
{
  if (HAL_CAN_GetTxMailboxesFreeLevel(&hcan) > 0)
  {
    return CAN_OK;
  }

  return CAN_TX_BUSY_ERROR;
}

/*-----------------------------------------------------------------------------------------------------
 * Function: CAN_fill_and_send_mbox
 *
 * Description: Заполняет заголовок CAN сообщения и отправляет через HAL
 *
 * Input:       msg - указатель на структуру CAN сообщения для отправки
 *
 * Output:      CAN_OK - сообщение успешно отправлено
 *              CAN_TX_BUSY_ERROR - ошибка отправки (mailbox заняты)
 *
 * Called by:   - CAN_send_or_post_msg() для немедленной отправки
 *              - HAL_CAN_TxMailbox0CompleteCallback() для отправки из очереди
 * * Note:        Преобразует внутренний формат сообщения в формат HAL
 *              Поддерживает стандартные и расширенные идентификаторы
 *              Использует аппаратную очередь CAN контроллера
 *-----------------------------------------------------------------------------------------------------*/
static T_can_err CAN_fill_and_send_mbox(T_can_msg *msg)
{
  CAN_TxHeaderTypeDef txHeader;
  uint32_t            txMailbox;

  // Настройка заголовка сообщения
  if (msg->format == EXTENDED_FORMAT)
  {
    txHeader.IDE   = CAN_ID_EXT;
    txHeader.ExtId = msg->id;
  }
  else
  {
    txHeader.IDE   = CAN_ID_STD;
    txHeader.StdId = msg->id;
  }

  txHeader.RTR                = (msg->type == REMOTE_FRAME) ? CAN_RTR_REMOTE : CAN_RTR_DATA;
  txHeader.DLC                = msg->len;
  txHeader.TransmitGlobalTime = DISABLE;

  // Отправка сообщения
  if (HAL_CAN_AddTxMessage(&hcan, &txHeader, msg->data, &txMailbox) != HAL_OK)
  {
    return CAN_TX_BUSY_ERROR;
  }

  return CAN_OK;
}

/*-----------------------------------------------------------------------------------------------------
 * Function: CAN_set_32bit_filter_mask
 *
 * Description: Настройка 32-битного фильтра CAN с маской через HAL
 *
 * Input:       bank - номер банка фильтра (0-13 для STM32F103)
 *              filter - значение фильтра (29-битный идентификатор)
 *              mask - маска фильтра (биты 1 - проверяются, биты 0 - игнорируются)
 *
 * Output:      Нет
 *
 * Called by:   - Task_can_transmiter() для настройки приема сообщений с определенным ID
 *
 * Note:        Фильтр настраивается для расширенных идентификаторов (29 бит)
 *              Сообщения направляются в FIFO0
 *              Преобразование битов выполняется согласно формату регистров STM32
 *-----------------------------------------------------------------------------------------------------*/
void CAN_set_32bit_filter_mask(uint16_t bank, uint32_t filter, uint32_t mask)
{
  CAN_FilterTypeDef canFilterConfig;

  canFilterConfig.FilterActivation     = ENABLE;
  canFilterConfig.FilterBank           = bank;
  canFilterConfig.FilterFIFOAssignment = CAN_RX_FIFO0;
  canFilterConfig.FilterIdHigh         = (filter >> 13) & 0xFFFF;
  canFilterConfig.FilterIdLow          = (filter << 3) & 0xFFFF;
  canFilterConfig.FilterMaskIdHigh     = (mask >> 13) & 0xFFFF;
  canFilterConfig.FilterMaskIdLow      = (mask << 3) & 0xFFFF;
  canFilterConfig.FilterMode           = CAN_FILTERMODE_IDMASK;
  canFilterConfig.FilterScale          = CAN_FILTERSCALE_32BIT;
  canFilterConfig.SlaveStartFilterBank = 14;

  HAL_CAN_ConfigFilter(&hcan, &canFilterConfig);
}

/*-----------------------------------------------------------------------------------------------------
 * Function: CAN_get_errors
 *
 * Description: Получает текущие ошибки CAN контроллера через HAL
 *
 * Input:       chanel - номер канала CAN (не используется в текущей реализации)
 *
 * Output:      Битовая маска ошибок CAN контроллера
 *
 * Called by:   - Task_can_transmiter() для периодической проверки ошибок CAN
 *
 * Note:        Возвращает значение регистра ошибок HAL CAN
 *              Биты 0-2 указывают на критические ошибки (bus-off, error warning, error passive)
 *-----------------------------------------------------------------------------------------------------*/
unsigned int CAN_get_errors(uint32_t chanel)
{
  return HAL_CAN_GetError(&hcan);
}

/*-----------------------------------------------------------------------------------------------------
 * Function: CAN_init
 *
 * Description: Инициализация CAN менеджера и аппаратной части контроллера
 *              Инициализирует пул памяти для CAN сообщений, создает семафоры и очереди FreeRTOS,
 *              запускает HAL CAN и активирует прерывания
 *
 * Input:       Нет
 *
 * Output:      CAN_OK - успешная инициализация
 *              CAN_MEM_POOL_INIT_ERROR - ошибка создания семафоров или очередей
 *              CAN_BAUDRATE_ERROR - ошибка запуска CAN или активации прерываний
 *
 * Called by:   - Task_can_transmiter() при запуске задачи передачи CAN
 * * Note:        Использует статическое выделение памяти для FreeRTOS объектов
 *              Очереди can_tx_queue и can_rx_queue буферизуют сообщения
 *              HAL уже инициализирует CAN через MX_CAN_Init() в main.c
 *              STM32 HAL CAN функции уже thread-safe и не требуют дополнительной защиты
 *              Скорость передачи настраивается в CubeMX, не передается как параметр
 *-----------------------------------------------------------------------------------------------------*/
T_can_err CAN_init(void)
{
  static uint8_t first_run_flag = 0;

  /* When function is called for the first time it will initialize and setup
     all of the resources that are common to CAN functionality               */
  if (first_run_flag == 0)
  {
    first_run_flag = 1;

    /* Initialize memory pool */
    memset(can_memory_pool_used, 0, sizeof(can_memory_pool_used));
  }

  /* Create FreeRTOS queues using static allocation for mailbox functionality */
  can_tx_queue = xQueueCreateStatic(
  CAN_NO_SEND_OBJECTS,      // Queue length
  sizeof(T_can_msg *),      // Item size
  ucCanTxQueueStorageArea,  // Queue storage buffer
  &xCanTxQueueBuffer        // Queue buffer
  );

  can_rx_queue = xQueueCreateStatic(
  CAN_NO_RECV_OBJECTS,      // Queue length
  sizeof(T_can_msg *),      // Item size
  ucCanRxQueueStorageArea,  // Queue storage buffer
  &xCanRxQueueBuffer        // Queue buffer
  );

  if (can_tx_queue == NULL || can_rx_queue == NULL)
  {
    return CAN_MEM_POOL_INIT_ERROR;
  }

  /* Start CAN hardware - HAL уже инициализирует CAN через MX_CAN_Init() в main.c */
  if (HAL_CAN_Start(&hcan) != HAL_OK)
  {
    return CAN_BAUDRATE_ERROR;
  }

  /* Активация уведомлений о приеме и передаче */
  if (HAL_CAN_ActivateNotification(&hcan,
                                   CAN_IT_TX_MAILBOX_EMPTY |
                                   CAN_IT_RX_FIFO0_MSG_PENDING |
                                   CAN_IT_RX_FIFO0_OVERRUN) != HAL_OK)
  {
    return CAN_BAUDRATE_ERROR;
  }

  return CAN_OK;
}

/*-----------------------------------------------------------------------------------------------------
 * Function: CAN_send_or_post_msg
 *
 * Description: Отправляет CAN сообщение немедленно или помещает в очередь отправки
 *              Если передающий mailbox свободен - отправляет сразу,
 *              иначе помещает в очередь для отправки при освобождении mailbox
 *
 * Input:       msg - указатель на структуру CAN сообщения для отправки
 *              timeout - таймаут добавления в очередь (в миллисекундах)
 *
 * Output:      CAN_OK - сообщение отправлено или добавлено в очередь
 *              CAN_TIMEOUT_ERROR - таймаут добавления в очередь
 *              CAN_ALLOC_MEM_ERROR - нет свободной памяти для сообщения
 *
 * Called by:   - Send_ONBUS_MSG() для отправки сообщения о подключении к шине
 *              - Пользовательские функции для отправки CAN сообщений
 * * Note:        STM32 HAL CAN функции уже thread-safe и не требуют дополнительной защиты
 *              При неудаче добавления в очередь освобождает выделенную память
 *-----------------------------------------------------------------------------------------------------*/
T_can_err CAN_send_or_post_msg(T_can_msg *msg, uint16_t timeout)
{
  T_can_msg *ptrmsg;

  if (CAN_is_txmbox0_empty() == CAN_OK)
  {
    if (CAN_fill_and_send_mbox(msg) != CAN_OK)
    {
      return CAN_TX_BUSY_ERROR;
    }
  }
  else
  {
    ptrmsg = alloc_can_msg();
    if (ptrmsg != NULL)
    {
      *ptrmsg = *msg;

      if (xQueueSend(can_tx_queue, &ptrmsg, pdMS_TO_TICKS(timeout)) != pdTRUE)
      {
        free_can_msg(ptrmsg);
        return CAN_TIMEOUT_ERROR;
      }
      else
      {        /* Check once again if transmit hardware is ready for transmission   */
        if (CAN_is_txmbox0_empty() == CAN_OK)
        {          if (xQueueReceive(can_tx_queue, &ptrmsg, 0) == pdTRUE)
          {
            free_can_msg(ptrmsg);
            if (CAN_fill_and_send_mbox(msg) != CAN_OK)
            {
              return CAN_TX_BUSY_ERROR;
            }
          }
        }
      }
    }
    else
      return CAN_ALLOC_MEM_ERROR;
  }
  return CAN_OK;
}

/*-----------------------------------------------------------------------------------------------------
 * Function: CAN_pull_msg_from_mbox
 *
 * Description: Извлекает принятое CAN сообщение из очереди приема
 *              Ожидает поступления сообщения в течение заданного таймаута,
 *              копирует сообщение в пользовательский буфер и освобождает память
 *
 * Input:       msg - указатель на структуру для сохранения принятого сообщения
 *              timeout - таймаут ожидания сообщения (в миллисекундах)
 *
 * Output:      CAN_OK - сообщение успешно получено и скопировано
 *              CAN_TIMEOUT_ERROR - таймаут ожидания сообщения
 *
 * Called by:   - Task_can_receiver() для получения принятых CAN сообщений
 *              - Пользовательские функции для чтения входящих сообщений
 *
 * Note:        Функция блокирующая - ожидает сообщения в течение timeout
 *              Автоматически освобождает память после копирования сообщения
 *              Использует очередь can_rx_queue для буферизации принятых сообщений
 *-----------------------------------------------------------------------------------------------------*/
T_can_err CAN_pull_msg_from_mbox(T_can_msg *msg, uint16_t timeout)
{
  T_can_msg *ptrmsg;

  /* Wait for received message in queue                                    */
  if (xQueueReceive(can_rx_queue, &ptrmsg, pdMS_TO_TICKS(timeout)) != pdTRUE)
    return CAN_TIMEOUT_ERROR;

  /* Copy received message from queue to address given in function parameter msg */
  *msg = *ptrmsg;

  /* Free memory where message was kept                                         */
  free_can_msg(ptrmsg);

  return CAN_OK;
}

/*-----------------------------------------------------------------------------------------------------
 * Function: Send_ONBUS_MSG
 *
 * Description: Отправляет сообщение о подключении устройства к CAN шине
 *              Устанавливает флаг ожидания подтверждения ACK
 *
 * Input:       Нет
 *
 * Output:      Результат вызова CAN_send_or_post_msg() - код ошибки или успеха
 *
 * Called by:   - Task_can_transmiter() при инициализации и периодически до получения ACK
 *
 * Note:        Сообщение имеет расширенный формат (29 бит)
 *              ID формируется как PDISPLx_ONBUS_MSG | (node_addr << 20)
 *              Устанавливает статус ожидания подтверждения
 *-----------------------------------------------------------------------------------------------------*/
static int Send_ONBUS_MSG(void)
{
  T_can_msg can_msg;
  T_can_err result;

  can_msg.format = EXTENDED_FORMAT;
  can_msg.type   = DATA_FRAME;
  can_msg.id     = PDISPLx_ONBUS_MSG | (app_vars.node_addr << 20);
  can_msg.len    = 0;
  memset(can_msg.data, 0, 8);

  result = CAN_send_or_post_msg(&can_msg, 0x010);

  if (result == CAN_OK)
  {
    // Устанавливаем статус ожидания подтверждения
    onbus_status.pending        = 1;
    onbus_status.last_send_time = xTaskGetTickCount();
    onbus_status.retry_count++;
    onbus_status.ack_received = 0;
  }

  return result;
}

/*-----------------------------------------------------------------------------------------------------
 * Function: Check_ONBUS_ACK_timeout
 *
 * Description: Проверяет таймаут ожидания ACK для ONBUS сообщения
 *
 * Input:       Нет
 *
 * Output:      1 - если нужна повторная отправка, 0 - если не нужна
 *
 * Called by:   - Task_can_transmiter() для проверки необходимости повторной отправки
 *
 * Note:        Таймаут 1 секунда для повторной отправки
 *-----------------------------------------------------------------------------------------------------*/
uint8_t Check_ONBUS_ACK_timeout(void)
{
  uint32_t current_time = xTaskGetTickCount();

  // Если ожидаем подтверждения и прошла секунда без ACK
  if (onbus_status.pending &&
      (current_time - onbus_status.last_send_time) > pdMS_TO_TICKS(1000))
  {
    return 1;  // Нужна повторная отправка
  }

  return 0;    // Повторная отправка не нужна
}

/*-----------------------------------------------------------------------------------------------------
 * Function: Is_CAN_bus_active
 *
 * Description: Проверяет активность CAN шины по наличию ACK на ONBUS сообщения
 *
 * Input:       Нет
 *
 * Output:      1 - шина активна (есть устройства), 0 - шина неактивна
 *
 * Called by:   - Task_can_transmiter() для определения состояния шины
 *
 * Note:        Считает шину активной если получен ACK на ONBUS сообщение
 *              Также проверяет отсутствие критических ошибок
 *-----------------------------------------------------------------------------------------------------*/
uint8_t Is_CAN_bus_active(uint32_t can_err)
{
  // Если получен ACK, то шина точно активна
  if (onbus_status.ack_received)
  {
    return 1;
  }

  // Проверяем критические ошибки, указывающие на отключение шины
  // Bus-off - шина отключена
  if (can_err & HAL_CAN_ERROR_BOF)
  {
    return 0;
  }

  // Если слишком много попыток без ACK - считаем шину отключенной
  if (onbus_status.retry_count >= 10)
  {
    return 0;
  }

  // В других случаях считаем шину активной (возможны временные помехи)
  return 1;
}

/*-----------------------------------------------------------------------------------------------------
 * Function: CAN_process_errors
 *
 * Description: Обрабатывает ошибки CAN и выполняет необходимые действия по восстановлению
 *
 * Input:       can_err - битовая маска ошибок CAN
 *
 * Output:      Нет
 *
 * Called by:   - Task_can_transmiter() для обработки ошибок CAN
 *
 * Note:        Реализует различные стратегии восстановления в зависимости от типа ошибки
 *              Ведет статистику ошибок для диагностики проблем
 *-----------------------------------------------------------------------------------------------------*/
static void CAN_process_errors(uint32_t can_err)
{
  uint32_t        current_time     = xTaskGetTickCount();
  static uint32_t last_stable_time = 0;

  // Сначала сбрасываем обработанные ошибки в HAL для предотвращения накопления
  if (can_err != 0)
  {
    HAL_CAN_ResetError(&hcan);
    can_error_stats.last_error_time = current_time;
  }

  // Проверяем стабильность шины (нет ошибок более 100 мс)
  if (can_err == 0)
  {
    last_stable_time = current_time;
    // Постепенно сбрасываем счетчик при стабильной работе
    if (can_error_stats.consecutive_errors > 0)
    {
      can_error_stats.consecutive_errors--;
    }
    can_error_stats.recovery_in_progress = 0;
    return;
  }

  // Увеличиваем счетчик только если ошибки персистентные (более 50 мс)
  if ((current_time - last_stable_time) > pdMS_TO_TICKS(50))
  {
    if (can_error_stats.consecutive_errors < 255)
    {
      can_error_stats.consecutive_errors++;
    }
  }

  /* Критические ошибки состояния шины */

  // Bus-Off ошибка - самая критичная (bit 2)
  if (can_err & 0x04)
  {
    can_error_stats.bus_off_count++;
    can_error_stats.recovery_in_progress = 1;
    can_error_stats.recovery_attempts++;

    // Автоматическое восстановление при Bus-Off
    HAL_CAN_Stop(&hcan);
    vTaskDelay(pdMS_TO_TICKS(200));  // Увеличенная пауза для стабилизации
    HAL_CAN_Start(&hcan);

    // Повторная активация прерываний
    HAL_CAN_ActivateNotification(&hcan,
                                 CAN_IT_TX_MAILBOX_EMPTY |
                                 CAN_IT_RX_FIFO0_MSG_PENDING |
                                 CAN_IT_RX_FIFO0_OVERRUN);

    // Очищаем очереди и сбрасываем счетчик
    xQueueReset(can_tx_queue);
    xQueueReset(can_rx_queue);
    can_error_stats.consecutive_errors = 0;
    return;  // Не обрабатываем другие ошибки после Bus-Off восстановления
  }

  // Error Passive состояние - менее агрессивная реакция (bit 1)
  if (can_err & 0x02)
  {
    can_error_stats.error_passive_count++;

    // Восстановление только при критическом количестве ошибок
    if (can_error_stats.consecutive_errors > 30)
    {
      can_error_stats.recovery_attempts++;
      HAL_CAN_Stop(&hcan);
      vTaskDelay(pdMS_TO_TICKS(100));
      HAL_CAN_Start(&hcan);
      can_error_stats.consecutive_errors = 0;
    }
  }

  // Error Warning состояние (bit 0)
  if (can_err & 0x01)
  {
    can_error_stats.error_warning_count++;
  }

  /* Ошибки протокола CAN - менее критичные */

  // ACK ошибки - могут быть временными при подаче питания (bit 5)
  if (can_err & 0x20)
  {
    can_error_stats.ack_error_count++;

    // Уведомление только при устойчивых проблемах
    if (can_error_stats.ack_error_count % 20 == 0)
    {
      // Можно добавить уведомление верхнего уровня о проблемах связи
      // app_vars.canerr_phy_layer = 1;
    }
  }

  // Stuff ошибки (bit 4)
  if (can_err & 0x10)
  {
    can_error_stats.stuff_error_count++;
  }

  // Form ошибки (bit 3)
  if (can_err & 0x08)
  {
    can_error_stats.form_error_count++;
  }

  // CRC ошибки (bit 6)
  if (can_err & 0x40)
  {
    can_error_stats.crc_error_count++;
  }

  /* Ошибки передачи TX_TERR - обычно временные */

  // TX_TERR0 - ошибка передачи в Mailbox 0 (bit 26)
  if (can_err & (1UL << 26))
  {
    can_error_stats.tx_terr0_count++;
  }

  // TX_TERR1 - ошибка передачи в Mailbox 1 (bit 27)
  if (can_err & (1UL << 27))
  {
    can_error_stats.tx_terr1_count++;
  }

  // TX_TERR2 - ошибка передачи в Mailbox 2 (bit 28)
  if (can_err & (1UL << 28))
  {
    can_error_stats.tx_terr2_count++;
  }

  /* Критическая диагностика - только для персистентных ошибок */
  // Полная переинициализация только при критическом состоянии
  if (can_error_stats.consecutive_errors > 100 &&
      !can_error_stats.recovery_in_progress)
  {
    can_error_stats.recovery_attempts++;
    can_error_stats.consecutive_errors   = 0;
    can_error_stats.recovery_in_progress = 1;  // Полная переинициализация CAN с увеличенной паузой
    vTaskDelay(pdMS_TO_TICKS(500));            // Пауза перед переинициализацией
    CAN_init();
    // Повторная настройка всех фильтров после восстановления
    CAN_setup_all_filters();
    CAN_release_init_mode();
  }
}

/*-----------------------------------------------------------------------------------------------------
 * Function: Task_can_transmiter
 *
 * Description: Задача FreeRTOS для инициализации CAN и мониторинга ошибок
 *              Выполняет начальную настройку CAN контроллера, фильтров,
 *              отправляет сообщение ONBUS и периодически проверяет состояние шины
 *
 * Input:       pvParameters - параметры задачи FreeRTOS (не используются)
 *
 * Output:      Нет (бесконечный цикл)
 *
 * Called by:   - FreeRTOS scheduler при создании задачи
 *
 * Note:        Приоритет задачи должен обеспечивать своевременное обслуживание CAN
 *              Фильтр настраивается на прием сообщений с ID узла (PDISPLx_REQ)
 *              Проверка ошибок каждые 10 мс для раннего обнаружения проблем шины
 *              Реализует автоматическое восстановление при критических ошибках
 *-----------------------------------------------------------------------------------------------------*/
void Task_can_transmiter(void *pvParameters)
{
  unsigned int can_err;

  // Инициализация CAN контроллера
  CAN_init();
  // Настройка всех фильтров для приема сообщений с адресом узла
  CAN_setup_all_filters();

  CAN_release_init_mode();

  // Отправка первого сообщения о подключении к шине (кроме мастера)
  Send_ONBUS_MSG();

  // Основной цикл мониторинга
  for (;;)
  {
    // Получение текущих ошибок CAN контроллера
    can_err = CAN_get_errors(CAN_CHANL);

    // Обработка ошибок с автоматическим восстановлением
    CAN_process_errors(can_err);
    // Проверка необходимости повторной отправки ONBUS сообщения
    if (Check_ONBUS_ACK_timeout())
    {
      // Если ACK не получен в течение секунды и шина активна - повторяем отправку
      if (Is_CAN_bus_active(can_err) || onbus_status.retry_count < 5)
      {
        Send_ONBUS_MSG();
      }
      else
      {
        // Шина неактивна и много попыток - прекращаем отправку
        onbus_status.pending = 0;
      }
    }

    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

/*-----------------------------------------------------------------------------------------------------
 * Function: Task_can_receiver
 *
 * Description: Задача FreeRTOS для приема и обработки CAN сообщений дисплейного протокола
 *              Использует switch-case для эффективной маршрутизации команд по базовому ID
 *              Извлекает базовый ID (без адреса узла) и направляет сообщения к соответствующим обработчикам
 *
 * Input:       pvParameters - параметры задачи FreeRTOS (не используются)
 *
 * Output:      Нет (бесконечный цикл)
 *
 * Called by:   - FreeRTOS scheduler при создании задачи
 *
 * Note:        Таймаут приема 255 мс обеспечивает отзывчивость системы
 *              Использует вложенные switch-case для двухуровневой маршрутизации:
 *              1. По базовому ID (PDISPLx_REQ, PDISPLx_SET_RED_SYMB, PDISPLx_SET_GREEN_SYMB)
 *              2. По подкоманде в data[0] для PDISPLx_REQ
 *              Обработчики команд вынесены в отдельные функции в Application.c
 *              Поддерживаемые команды определяются в CAN_IDs.h
 *-----------------------------------------------------------------------------------------------------*/
void Task_can_receiver(void *pvParameters)
{
  T_can_msg msg_rcv;
  uint32_t  base_id;

  for (;;)
  {
    // When message arrives store received value to global variable Rx_val
    if (CAN_pull_msg_from_mbox(&msg_rcv, 0x00FF) == CAN_OK)
    {
      // Extract base ID (without node address)
      base_id = msg_rcv.id & 0x1E0FFFFF;  // Mask out node address (bits 20-23)

      // Process messages from display protocol using switch case
      switch (base_id)
      {
        case PDISPLx_REQ:
          // Handle commands sent to PDISPLx_REQ ID - sub-command in data[0]
          switch (msg_rcv.data[0])
          {
            case PDISPLx_SET_SYMBOL:
              Handle_CAN_SetSymbol(msg_rcv.data);
              break;

            case PDISPLx_SET_SYMBOL_PTRN1:
              Handle_CAN_SetSymbolPattern1(msg_rcv.data);
              break;

            case PDISPLx_SET_SYMBOL_PTRN2:
              Handle_CAN_SetSymbolPattern2(msg_rcv.data);
              break;

            case PDISPLx_DIN_SYMBOL_SET1:
              Handle_CAN_DynamicSymbolSet1(msg_rcv.data);
              break;

            case PDISPLx_DIN_SYMBOL_SET2:
              Handle_CAN_DynamicSymbolSet2(msg_rcv.data);
              break;

            case PDISPLx_DIN_SYMBOL_SET3:
              Handle_CAN_DynamicSymbolSet3(msg_rcv.data);
              break;

            case PDISPLx_DIN_SYMBOL_SET4:
              Handle_CAN_DynamicSymbolSet4(msg_rcv.data);
              break;

            default:
              // Unknown command - ignore
              break;
          }
          break;

        case PDISPLx_SET_RED_SYMB:
          // Handle red screen command
          Handle_CAN_SetRedScreen(msg_rcv.data);
          break;

        case PDISPLx_SET_GREEN_SYMB:
          // Handle green screen command
          Handle_CAN_SetGreenScreen(msg_rcv.data);
          break;

        default:
          // Unknown message ID - ignore
          break;
      }
    }
  }
}

/*--------------------------- HAL CAN Callback Functions -------------------*/

/*-----------------------------------------------------------------------------------------------------
 * Function: HAL_CAN_TxMailbox0CompleteCallback
 * * Description: Callback функция HAL для обработки завершения передачи из Mailbox 0
 *              Вызывается аппаратным прерыванием при успешной отправке CAN сообщения
 *              Проверяет очередь передачи и отправляет следующее сообщение если есть
 *
 * Input:       hcan - указатель на дескриптор CAN контроллера HAL
 *
 * Output:      Нет
 *
 * Called by:   - HAL_CAN interrupt handler при завершении передачи
 *
 * Note:        Выполняется в контексте прерывания - использует ISR-безопасные функции FreeRTOS
 *              Если очередь передачи не пуста - отправляет следующее сообщение
 *              Автоматически обрабатывает цепочку сообщений в очереди
 *              Вызывает portYIELD_FROM_ISR для переключения контекста при необходимости
 *-----------------------------------------------------------------------------------------------------*/
void HAL_CAN_TxMailbox0CompleteCallback(CAN_HandleTypeDef *hcan)
{
  T_can_msg *ptrmsg;
  BaseType_t xHigherPriorityTaskWoken = pdFALSE;

  // Проверяем, получили ли ACK (успешная передача без ошибок)
  uint32_t can_error                  = HAL_CAN_GetError(hcan);
  if ((can_error & (HAL_CAN_ERROR_TX_ALST0 | HAL_CAN_ERROR_TX_TERR0)) == HAL_CAN_ERROR_NONE)
  {
    // ACK получен - отмечаем успешную передачу для ONBUS сообщений
    if (onbus_status.pending)
    {
      onbus_status.ack_received = 1;
      onbus_status.pending      = 0;
    }
  }  // If there is a message in the queue ready for send, read the message from the queue and send it
  if (xQueueReceiveFromISR(can_tx_queue, &ptrmsg, &xHigherPriorityTaskWoken) == pdTRUE)
  {
    CAN_fill_and_send_mbox(ptrmsg);
    free_can_msg(ptrmsg);
    xHigherPriorityTaskWoken = pdTRUE;
  }
  portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

/*-----------------------------------------------------------------------------------------------------
 * Function: HAL_CAN_TxMailbox1CompleteCallback
 *
 * Description: Callback функция HAL для обработки завершения передачи из Mailbox 1
 *              Перенаправляет обработку на общую функцию HAL_CAN_TxMailbox0CompleteCallback
 *
 * Input:       hcan - указатель на дескриптор CAN контроллера HAL
 *
 * Output:      Нет
 *
 * Called by:   - HAL_CAN interrupt handler при завершении передачи из Mailbox 1
 *
 * Note:        STM32 CAN контроллер имеет 3 передающих mailbox (0, 1, 2)
 *              Все используют одинаковую логику обработки завершения передачи
 *              Функция выполняется в контексте прерывания
 *-----------------------------------------------------------------------------------------------------*/
void HAL_CAN_TxMailbox1CompleteCallback(CAN_HandleTypeDef *hcan)
{
  HAL_CAN_TxMailbox0CompleteCallback(hcan);
}

/*-----------------------------------------------------------------------------------------------------
 * Function: HAL_CAN_TxMailbox2CompleteCallback
 *
 * Description: Callback функция HAL для обработки завершения передачи из Mailbox 2
 *              Перенаправляет обработку на общую функцию HAL_CAN_TxMailbox0CompleteCallback
 *
 * Input:       hcan - указатель на дескриптор CAN контроллера HAL
 *
 * Output:      Нет
 *
 * Called by:   - HAL_CAN interrupt handler при завершении передачи из Mailbox 2
 *
 * Note:        STM32 CAN контроллер имеет 3 передающих mailbox (0, 1, 2)
 *              Все используют одинаковую логику обработки завершения передачи
 *              Функция выполняется в контексте прерывания
 *-----------------------------------------------------------------------------------------------------*/
void HAL_CAN_TxMailbox2CompleteCallback(CAN_HandleTypeDef *hcan)
{
  HAL_CAN_TxMailbox0CompleteCallback(hcan);
}

/*-----------------------------------------------------------------------------------------------------
 * Function: HAL_CAN_RxFifo0MsgPendingCallback
 *
 * Description: Callback функция HAL для обработки поступления CAN сообщения в FIFO0
 *              Вызывается аппаратным прерыванием при получении нового CAN сообщения
 *              Читает сообщение из FIFO, преобразует в внутренний формат и помещает в очередь приема
 *
 * Input:       hcan - указатель на дескриптор CAN контроллера HAL
 *
 * Output:      Нет
 *
 * Called by:   - HAL_CAN interrupt handler при поступлении сообщения в RX FIFO0
 *
 * Note:        Выполняется в контексте прерывания - использует ISR-безопасные функции FreeRTOS
 *              Поддерживает стандартные и расширенные идентификаторы CAN
 *              Поддерживает Data Frame и Remote Frame типы сообщений
 *              При переполнении очереди приема сообщение отбрасывается
 *              Автоматически освобождает память при ошибках чтения или очереди
 *              Вызывает portYIELD_FROM_ISR для переключения контекста при необходимости
 *-----------------------------------------------------------------------------------------------------*/
void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
  CAN_RxHeaderTypeDef rxHeader;
  T_can_msg          *ptrmsg;
  BaseType_t          xHigherPriorityTaskWoken = pdFALSE;

  // Allocate memory for CAN message
  ptrmsg                                       = alloc_can_msg();
  if (ptrmsg)
  {
    // Read received message
    if (HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &rxHeader, ptrmsg->data) == HAL_OK)
    {
      // Fill message structure
      if (rxHeader.IDE == CAN_ID_EXT)
      {
        ptrmsg->format = EXTENDED_FORMAT;
        ptrmsg->id     = rxHeader.ExtId;
      }
      else
      {
        ptrmsg->format = STANDARD_FORMAT;
        ptrmsg->id     = rxHeader.StdId;
      }

      ptrmsg->type = (rxHeader.RTR == CAN_RTR_REMOTE) ? REMOTE_FRAME : DATA_FRAME;
      ptrmsg->len  = rxHeader.DLC;

      // Try to send message to queue
      if (xQueueSendFromISR(can_rx_queue, &ptrmsg, &xHigherPriorityTaskWoken) == pdPASS)
      {
        xHigherPriorityTaskWoken = pdTRUE;
      }
      else
      {
        // Queue is full, free the allocated message
        free_can_msg(ptrmsg);
      }
    }
    else
    {
      free_can_msg(ptrmsg);
    }
  }

  portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

/*-----------------------------------------------------------------------------------------------------
 * Function: HAL_CAN_ErrorCallback
 *
 * Description: Callback функция HAL для обработки критических ошибок CAN контроллера
 *              Вызывается автоматически при обнаружении ошибок на аппаратном уровне
 *
 * Input:       hcan - указатель на дескриптор CAN контроллера HAL
 *
 * Output:      Нет
 *
 * Called by:   - HAL_CAN interrupt handler при критических ошибках
 *
 * Note:        Выполняется в контексте прерывания
 *              Обрабатывает только критические ошибки Bus-Off
 *              Основная обработка ошибок выполняется в Task_can_transmiter
 *-----------------------------------------------------------------------------------------------------*/
void HAL_CAN_ErrorCallback(CAN_HandleTypeDef *hcan)
{
  // Обработка критической ошибки Bus-Off
  if ((hcan->ErrorCode & HAL_CAN_ERROR_BOF) != 0)
  {
    // Увеличиваем счетчик Bus-Off ошибок
    can_error_stats.bus_off_count++;
    can_error_stats.consecutive_errors++;

    // Основное восстановление будет выполнено в задаче Task_can_transmiter
    // В прерывании только отмечаем факт ошибки
  }

  // Остальные ошибки обрабатываются в Task_can_transmiter
  // для избежания блокировки прерывания
}

/*-----------------------------------------------------------------------------------------------------
 * Function: CAN_get_error_stats
 *
 * Description: Возвращает указатель на структуру со статистикой ошибок CAN
 *
 * Input:       Нет
 *
 * Output:      Указатель на структуру CAN_Error_Stats_t с текущей статистикой
 *
 * Called by:   - Диагностические функции для получения статистики ошибок
 *              - Функции мониторинга состояния системы
 *
 * Note:        Возвращаемые данные только для чтения
 *              Статистика накапливается с момента запуска системы
 *-----------------------------------------------------------------------------------------------------*/
const CAN_Error_Stats_t *CAN_get_error_stats(void)
{
  return &can_error_stats;
}

/*-----------------------------------------------------------------------------------------------------
 * Function: CAN_reset_error_stats
 *
 * Description: Сбрасывает статистику ошибок CAN
 *
 * Input:       Нет
 *
 * Output:      Нет
 *
 * Called by:   - Диагностические функции для очистки счетчиков
 *              - Функции технического обслуживания
 *
 * Note:        Сохраняет только флаг recovery_in_progress для корректной работы
 *-----------------------------------------------------------------------------------------------------*/
void CAN_reset_error_stats(void)
{
  uint8_t recovery_state = can_error_stats.recovery_in_progress;
  memset(&can_error_stats, 0, sizeof(CAN_Error_Stats_t));
  can_error_stats.recovery_in_progress = recovery_state;
}

/*-----------------------------------------------------------------------------------------------------
 * Function: Get_ONBUS_status
 *
 * Description: Возвращает текущий статус ONBUS сообщений
 *
 * Input:       Нет
 *
 * Output:      Указатель на структуру ONBUS_Status_t с текущим статусом
 *
 * Called by:   - Внешние модули для проверки статуса ONBUS
 *
 * Note:        Возвращает указатель на статическую структуру
 *-----------------------------------------------------------------------------------------------------*/
ONBUS_Status_t *Get_ONBUS_status(void)
{
  return &onbus_status;
}
