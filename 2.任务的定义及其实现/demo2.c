// Demo 2: 带参数传递的任务
// 重点：展示如何向任务传递参数和数据共享
// 特色：数据处理任务通过结构体接收不同配置参数
// 学习要点：任务参数传递、结构体使用、全局数据访问

#include "FreeRTOS.h"
#include "task.h"
#include <string.h>

//任务相关参数结构体
typedef struct {
    uint32_t task_id;           // 任务ID，用于唯一标识任务
    uint32_t process_interval;  // 任务处理周期
    char task_name[16];         // 任务名称，存储任务的名称（最多16个字符）
}TaskParams_t;

//任务句柄
TaskHandle DataProcess1_Handle;
TaskHandle DataProcess2_Handle;
TaskHandle Monitor_Handle;

//任务堆栈大小
#define TASK_STACK_SIZE 256

//任务堆栈
StackType_t DataProcess1Stack[TASK_STACK_SIZE];
StackType_t DataProcess2Stack[TASK_STACK_SIZE];
StackType_t MonitorStack[TASK_STACK_SIZE];

//任务控制块
StaticTask_t DataProcess1TCB;
StaticTask_t DataProcess2TCB;
StaticTask_t MonitorTCB;

//任务参数
// 修改1. static 确保数据在程序的整个生命周期中有效。
// const 确保数据在初始化后无法被修改。
// 这两者结合起来，不仅保证了数据的持久性，还提高了代码的安全性和稳定性。
static const TaskParams_t task1_params={1,300,"Datapro1"};
static const TaskParams_t task2_params={2,600,"Datapro2"};
// 修改2.动态分配（如果使用heap）
TaskParams_t *Monitor_params=pvPorMalloc(sizeof(TaskParams_t));
*Monitor_params=(TaskParams_t){3,1000,"Monitor"};

//全局变量缓冲区
volatile uint32_t data_buffer[2]    =   {0,0};
volatile uint32_t process_count[2]  =   {0,0};

//简单的数据处理函数
uint32_t process_data(uint32_t input,uint32_t task_id){
    return (input+task_id+100)%100;
}

//延时函数
void delay_ms(uint32_t ms){
    // for(uint32_t i=0;i<ms*1000;i++);
    vTaskDelay(pdMS_TO_TICKS(ms));
}

void DataProcess_Task(void *pvParameters){
    TaskParams_t *params=(TaskParams_t *)pvParameters;
    uint32_t local_counter=0;

    for(;;){
        //处理数据
        local_counter++;
        uint32_t process_data=process_data(local_counter,params->task_id);

        //存储到对应的缓冲区
        data_buffer[params->task_id-1]=process_data;
        process_count[params->task_id-1]++;

        //延时
        delay_ms(params->process_interval);

        //让出CPU
        taskYIELD();
    }
}

void Monitor_Task(void *pvParameters){
    TaskParams_t *params=(TaskParams_t *)pvParameters;

    for(;;){
        //在实际应用中，这里可以输出到串口或显示器
        //这里只是简单地更新一些状态变量

        //模拟监控器输出
        volatile uint32_t total_processes=process_count[0]+process_count[1];
        volatile uint32_t data_sum=data_buffer[0]+data_buffer[1];

        //防止编译器优化
        (void)total_processes;
        (void)data_sum;
        
        delay_ms(params->process_interval);
        taskYIELD();
    }
}

int main(){
    //初始化任务相关的列表(FreeRTOS内核会处理，不需要显示写出)
    //prvInitialiseTaskLists();

    //创建数据处理函数1
    DataProcess1_Handle=xTaskCreateStatic(
        DataProcess_Task,       // 任务函数
        "DataProcess1",         // 任务名称
        TASK_STACK_SIZE,        // 任务堆栈大小
        &task1_params,          // 任务参数 ← 这里就是 pvParameters
        DataProcess1Stack,      // 任务堆栈
        &DataProcess1TCB        // 任务控制块
    );

    //创建数据处理函数1
    DataProcess2_Handle=xTaskCreateStatic(
        DataProcess_Task,       // 任务函数
        "DataProcess2",         // 任务名称
        TASK_STACK_SIZE,        // 任务堆栈大小
        &task2_params,          // 任务参数 ← 这里就是 pvParameters
        DataProcess2Stack,      // 任务堆栈
        &DataProcess2TCB        // 任务控制块
    );
    
    //创建监控任务
    Monitor_Handle=xTaskCreateStatic(
        Monitor_Task,           // 任务函数
        "Monitor",              // 任务名称
        TASK_STACK_SIZE,        // 任务堆栈大小
        &Monitor_params,        // 任务参数 ← 这里就是 pvParameters
        MonitorStack,           // 任务堆栈
        &MonitorTCB             // 任务控制块
    );

    //将任务添加到就绪队列（不同优先级）（FreeRTOS内核会处理，不需要显示写出）
    // vListInsertEnd(&(pxReadyTaskLists[1]),&(DataProcess1TCB.xStateListItem));
    // vListInsertEnd(&(pxReadyTaskLists[2]),&(DataProcess2TCB.xStateListItem));
    // vListInsertEnd(&(pxReadyTaskLists[3]),&(MonitorTCB.xStateListItem));

    //启动调度器
    vTaskStartScheduler();

    for(;;);

    return 0;

}