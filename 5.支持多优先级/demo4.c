/*
 * Demo 4: 延时列表和就绪列表管理
 * 演示任务状态转换和列表管理机制
 * 
 * 学习要点:
 * 任务状态转换：Ready ↔ Blocked ↔ Suspended
 * 列表管理：任务在不同列表间的移动
 * 系统监控：实时观察任务状态
 * 动态控制：挂起和恢复任务
 * 
 * 
 * 任务状态转换：sensor_task、data_processing_task 等中的 vTaskDelay()，以及 suspend_resume_demo_task 中的 vTaskSuspend() 和 vTaskResume()。
 * 列表管理：通过 vTaskDelay()、vTaskSuspend() 和 vTaskResume() 间接触发 FreeRTOS 内核的列表操作。
 * 系统监控：task_state_monitor 中的 uxTaskGetSystemState() 和状态统计代码。
 * 动态控制：suspend_resume_demo_task 中的挂起/恢复逻辑。
 */
#include "FreeRTOS.h"
#include "task.h"

//任务状态统计
typedef struct{
    char task_name[16];
    eTaskState current_state;
    UBaseType_t priority;
    TickType_t creation_time;
    TickType_t total_runtime;
    uint32_t delay_count;
    uint32_t ready_count;
}task_status_t;

#define MAX_TASKS 10
static task_status_t task_status_array[MAX_TASKS];
static uint32_t task_count=0;


void task_state_monitor(void *pvParameters){
    TaskStatus_t *task_status_list;
    UBaseType_t task_array_size;
    uint32_t total_runtime;

    for(;;){
        //获取系统中所有任务的状态
        task_array_size=uxTaskGetNumberOfTasks();
        task_status_list=pvPortMalloc(task_array_size * sizeof(TaskStatus_t));

        if(task_status_list!=NULL){
            task_array_size=uxTaskGetSystemState(task_status_list,
                                                task_array_size,
                                                &total_runtime);

            for(uint32_t i=0;i<task_array_size&&i<MAX_TASKS;i++){
                strncpy(task_status_array[i].task_name,
                        task_status_list[i].pcTaskName,15);
                task_status_array[i].current_state=task_status_list[i].eCurrentState;
                task_status_array[i].priority = task_status_list[i].uxCurrentPriority;
                task_status_array[i].total_runtime = task_status_list[i].ulRunTimeCounter;

                // 统计状态变化
                switch(task_status_list[i].eCurrentState)
                {
                    case eReady:
                    case eRunning:
                        task_status_array[i].ready_count++;
                        break;
                    case eBlocked:
                    case eSuspended:
                        task_status_array[i].delay_count++;
                        break;
                    default:
                        break;
                }
            }

            task_count=task_array_size;
            vPortFree(task_status_list);
        }
        
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}


// 模拟传感器读取任务
void sensor_task(void *pvParameters){
    uint32_t sensor_data=0;
    for(;;){
        sensor_data++;
        for(volatile int i = 0; i < 1000; i++);
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}


// 模拟数据处理任务
void data_processing_task(void *pvParameters){
    for(;;){
        for(volatile int i = 0; i < 10000; i++);
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}


// 模拟通信任务
void communication_task(void *pvParameters){
    for(;;){
        for(volatile int i = 0; i < 5000; i++);
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}


// 模拟用户界面任务
void ui_task(void *pvParameters){
    for(;;){
        for(volatile int i = 0; i < 3000; i++);
        vTaskDelay(pdMS_TO_TICKS(33));
    }
}


// 演示任务挂起和恢复
void suspend_resume_demo_task(void *pvParameters){
    static uint32_t demo_cycle=0;

    for(;;){
        demo_cycle++;

        switch(demo_cycle%4){
            case 0:
                //挂起传感器任务2秒
                vTaskSuspend(sensor_handle);
                vTaskDelay(pdMS_TO_TICKS(2000));
                vTaskResume(sensor_handle);
                break;

            case 1:
                // 挂起数据处理任务1秒
                vTaskSuspend(data_processing_handle);
                vTaskDelay(pdMS_TO_TICKS(1000));
                vTaskResume(data_processing_handle);
                break;

            case 2:
                // 挂起通信任务3秒
                vTaskSuspend(communication_handle);
                vTaskDelay(pdMS_TO_TICKS(3000));
                vTaskResume(communication_handle);
                break;

            case 3:
                // 所有任务正常运行5秒
                vTaskDelay(pdMS_TO_TICKS(5000));
                break;
        }
    }
}


//任务句柄
static TaskHandle_t monitor_handle;
static TaskHandle_t sensor_handle;
static TaskHandle_t data_processing_handle;
static TaskHandle_t communication_handle;
static TaskHandle_t ui_handle;
static TaskHandle_t demo_handle;

//任务栈和TCB
static StackType_t task_stacks[6][256];
static TCB_t task_tcbs[6];

int main(void){
    //创建任务状态监控器（最高优先级）
    monitor_handle=xTaskCreateStatic(
        task_state_monitor,
        "Monitor",
        256,
        NULL,
        6,              //最高优先级
        task_stacks[0],
        &task_tcbs[0]
    );
    
    // 创建传感器任务
    sensor_handle = xTaskCreateStatic(
        sensor_task,
        "Sensor",
        256,
        NULL,
        4,  // 高优先级
        task_stacks[1],
        &task_tcbs[1]
    );

    // 创建数据处理任务
    data_processing_handle = xTaskCreateStatic(
        data_processing_task,
        "DataProc",
        256,
        NULL,
        3,  // 中等优先级
        task_stacks[2],
        &task_tcbs[2]
    );

    // 创建通信任务
    communication_handle = xTaskCreateStatic(
        communication_task,
        "Comm",
        256,
        NULL,
        2,  // 中等优先级
        task_stacks[3],
        &task_tcbs[3]
    );

    // 创建用户界面任务
    ui_handle = xTaskCreateStatic(
        ui_task,
        "UI",
        256,
        NULL,
        2,  // 中等优先级
        task_stacks[4],
        &task_tcbs[4]
    );

    // 创建挂起/恢复演示任务
    demo_handle = xTaskCreateStatic(
        suspend_resume_demo_task,
        "Demo",
        256,
        NULL,
        5,  // 高优先级
        task_stacks[5],
        &task_tcbs[5]
    );

    vTaskStartScheduler();

    for(;;);
}

static StackType_t idle_task_stack[configMINIMAL_STACK_SIZE];
static TCB_t idle_task_tcb;

void vApplicationGetIdleTaskMemory(TCB_t **ppxIdleTaskTCBBuffer,
                                StackType_t **ppxIdleTaskStackBuffer,
                                uint32_t *pulIdleTaskStackSize){
    *ppxIdleTaskTCBBuffer=&idle_task_tcb;
    *ppxIdleTaskStackBuffer=idle_task_stack;
    *pulIdleTaskStackSize=configMINIMAL_STACK_SIZE;
}





