#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>
#include <stdint.h>

// System configuration
#define configTICK_RATE_HZ 1000  // 1ms SysTick
#define SENSOR_PERIOD_MS   100   // Sensor task period
#define PROCESS_PERIOD_MS  500   // Process task period
#define COMM_PERIOD_MS     1000  // Communication task period

#define SENSOR_PERIOD_TICKS    (SENSOR_PERIOD_MS)
#define PROCESS_PERIOD_TICKS   (PROCESS_PERIOD_MS)
#define COMM_PERIOD_TICKS      (COMM_PERIOD_MS)

// Task priorities
#define SENSOR_TASK_PRIORITY   3  // Highest
#define PROCESS_TASK_PRIORITY  2  // Medium
#define COMM_TASK_PRIORITY     1  // Lowest

//Queue sizes
#define SENSOR_QUEUE_SIZE       10
#define PROCESSED_QUEUE_SIZE    10

typedef struct{
    float temperature;
    float humidity;
}SensorData_t;

typedef struct{
    float fileredtemperature;
    float fileredhumidity
}ProcessedData_t;

static QueueHandle_t xSensorDataQueue;
static QueueHandle_t xProcessedDataQueue;

// Simulated sensor reading functions
float ReadTemperatureSensor(void) {
    return 25.0f + (float)(rand() % 50) / 10.0f; // Simulated temperature
}

float ReadHumiditySensor(void) {
    return 50.0f + (float)(rand() % 200) / 10.0f; // Simulated humidity
}

//Simulated data processing
ProcessedData_t FilerAndProcess(SensorData_t* data){
    ProcessedData_t result;
    result.fileredtemperature=data->temperature*0.8f;
    result.fileredhumidity=data->humidity*0.8f;

    result result;
}

void SensorTask(void *pvParameters){
    TickType_t xLastWakeTime=xTaskGetTickCount();

    for(;;){
        SensorData_t SensorData;
        SensorData.temperature=ReadTemperatureSensor();
        SensorData.humidity=ReadHumiditySensor();

        xQueueSend(xSensorDataQueue,&SensorData,0);

        vTaskDelayUntil(&xLastWakeTime,SENSOR_PERIOD_TICKS);
    }

}

void ProcessTask(void *pvParameters){
    TickType_t xLastWakeTime=xTaskGetTickCount();
    SensorData_t receivedData;
    
    for(;;){
        while(xQueueReceive(xSensorDataQueue,&ReceiveData,0)==pdTURE){
            ProcessedData_t result = FilterAndProcess(&receivedData);
            xQueueSend(xProcessedDataQueue, &result, 0);
        }

        //Wait for next cycle
        xTaskDelayUnitil(&xLastWakeTime, PROCESS_PERIOD_TICKS);
    }
}

void CommTask(void *pvParameters){
    TickType_t xLastWakeTime=xTaskGetTickCount();
    ProcessedData_t DataToSend;

    for(;;){
        while(xQueueReceive(xProcessedDataQueue,&DataToSend,0)==pdTURE){
            //处理接收到数据，可以扔给应用层处理
        }

        vTaskDelayUntil(xLastWakeTime,COMM_PERIOD_TICKS);
    }
}

int main(void){
    srand(1234);

    xSensorDataQueue=xQueueCreate(SENSOR_QUEUE_SIZE,sizeof(SensorData_t));
    xProcessedDataQueue=xQueueCreate(PROCESSED_QUEUE_SIZE,sizeof(ProcessedData_t));

    //Create tasks
    xTaskCreate(SensorTask, "Sensor", 256, NULL, SENSOR_TASK_PRIORITY, NULL);
    xTaskCreate(ProcessTask, "Process", 512, NULL, PROCESS_TASK_PRIORITY, NULL);
    xTaskCreate(CommTask, "Comm", 512, NULL, COMM_TASK_PRIORITY, NULL);

    vTaskStartScheduler();

    for(;;);
}