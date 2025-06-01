#include "Application.h"

/* Static memory for CAN TX task */
StaticTask_t xCanTxTaskTCBBuffer;
StackType_t xCanTxTaskStack[CAN_TX_TASK_STACK_SIZE];

/* Static memory for CAN RX task */
StaticTask_t xCanRxTaskTCBBuffer;
StackType_t xCanRxTaskStack[CAN_RX_TASK_STACK_SIZE];

/* Static memory for CAN TX queue */
StaticQueue_t xCanTxQueueBuffer;
uint8_t ucCanTxQueueStorageArea[CAN_TX_QUEUE_LENGTH * sizeof(void*)];

/* Static memory for CAN RX queue */
StaticQueue_t xCanRxQueueBuffer;
uint8_t ucCanRxQueueStorageArea[CAN_RX_QUEUE_LENGTH * sizeof(void*)];

/* Task handles */
TaskHandle_t xCanTxTaskHandle = NULL;
TaskHandle_t xCanRxTaskHandle = NULL;

/* Queue handles - these are declared elsewhere, so we declare them as extern here */
/* They will be initialized in CAN_manager.c */

/* Semaphore handles - these are declared elsewhere, so we declare them as extern here */
/* They will be initialized in CAN_manager.c */
