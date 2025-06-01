#ifndef FREERTOS_STATIC_MEMORY_H
#define FREERTOS_STATIC_MEMORY_H

/* Task stack sizes */
#define CAN_TX_TASK_STACK_SIZE   64
#define CAN_RX_TASK_STACK_SIZE   64

/* Queue sizes based on CAN.h constants */
#define CAN_TX_QUEUE_LENGTH      CAN_NO_SEND_OBJECTS   // 10
#define CAN_RX_QUEUE_LENGTH      CAN_NO_RECV_OBJECTS   // 10

/* Static memory declarations for tasks */
extern StaticTask_t xCanTxTaskTCBBuffer;
extern StaticTask_t xCanRxTaskTCBBuffer;
extern StackType_t xCanTxTaskStack[CAN_TX_TASK_STACK_SIZE];
extern StackType_t xCanRxTaskStack[CAN_RX_TASK_STACK_SIZE];

/* Static memory declarations for queues */
extern StaticQueue_t xCanTxQueueBuffer;
extern StaticQueue_t xCanRxQueueBuffer;
extern uint8_t ucCanTxQueueStorageArea[CAN_TX_QUEUE_LENGTH * sizeof(void*)];
extern uint8_t ucCanRxQueueStorageArea[CAN_RX_QUEUE_LENGTH * sizeof(void*)];

/* Task handles */
extern TaskHandle_t xCanTxTaskHandle;
extern TaskHandle_t xCanRxTaskHandle;

/* Queue handles */
extern QueueHandle_t can_tx_queue;
extern QueueHandle_t can_rx_queue;

#endif /* FREERTOS_STATIC_MEMORY_H */
