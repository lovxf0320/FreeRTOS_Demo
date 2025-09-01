// Demo 4: 任务状态管理和控制（修正版）
// 重点：展示任务的动态控制和状态管理
// 特色：控制器任务动态管理其他任务的运行状态
// 学习要点：任务暂停/恢复、状态监控、动态任务管理

#include "FreeRTOS.h"
#include "task.h"

TaskHandle_t Controller_Handle;
TaskHandle_t Work1_Handle;
TaskHandle_t Work2_Handle;
TaskHandle_t Work3_Handle;

#define CONTROLLER_STACK_SIZE 512
#define WORK_STACK_SIZE 256

StackType_t ControllerStack[CONTROLLER_STACK_SIZE];  // 修正：StackType_t
StackType_t Work1Stack[WORK_STACK_SIZE];
StackType_t Work2Stack[WORK_STACK_SIZE];
StackType_t Work3Stack[WORK_STACK_SIZE];

StaticTask_t ControllerTCB;  // 修正：StaticTask_t
StaticTask_t Work1TCB;
StaticTask_t Work2TCB;
StaticTask_t Work3TCB;

// 任务状态枚举
typedef enum{
    TASK_STATE_RUNNING,
    TASK_STATE_SUSPENDED,    // 修正：拼写错误
    TASK_STATE_DELETED       // 修正：拼写错误
}TaskState_t;

// 任务控制结构
typedef struct{
    TaskHandle_t *handle;
    TaskState_t state;
    uint32_t run_count;
    char name[16];
}TaskControl_t;

// 任务控制数组
TaskControl_t task_controls[3]={
    {&Work1_Handle, TASK_STATE_RUNNING, 0, "Work1"},
    {&Work2_Handle, TASK_STATE_RUNNING, 0, "Work2"},
    {&Work3_Handle, TASK_STATE_RUNNING, 0, "Work3"}
};

// 系统状态
volatile uint32_t system_tick = 0;
volatile uint32_t control_mode = 0;

// 延时函数
void delay_ms(uint32_t ms){
    // for(uint32_t i = 0; i < 1000 * ms; i++);
    vTaskDelay(pdMS_TO_TICKS(ms));
}

// 暂停任务
void suspend_task(uint8_t task_index){
    if(task_index < 3 && task_controls[task_index].state == TASK_STATE_RUNNING){
        // 使用FreeRTOS API挂起任务
        vTaskSuspend(*task_controls[task_index].handle);
        // 更新状态
        task_controls[task_index].state = TASK_STATE_SUSPENDED;  // 修正：使用赋值而非比较
    }
}

// 恢复任务
void resume_task(uint8_t task_index){
    if(task_index < 3 && task_controls[task_index].state == TASK_STATE_SUSPENDED){
        // 使用FreeRTOS API恢复任务
        vTaskResume(*task_controls[task_index].handle);
        // 更新状态
        task_controls[task_index].state = TASK_STATE_RUNNING;  // 修正：使用赋值而非比较
    }
}

// 控制器任务—管理其他任务的生命周期
void Controller_Task(void* pvParameters){
    uint32_t control_cycle = 0;

    for(;;){
        system_tick++;
        control_cycle++;

        // 每10个周期改变一次控制模式
        if(control_cycle % 10 == 0){
            control_mode = (control_mode + 1) % 3;

            switch(control_mode){
                case 0:     // 正常运行模式 - 所有任务运行
                    resume_task(0);
                    resume_task(1);
                    resume_task(2);
                    break;
                
                case 1:     // 部分暂停模式 - 暂停Worker2
                    resume_task(0);
                    suspend_task(1);
                    resume_task(2);
                    break;

                case 2:     // 轮流运行模式 - 只运行一个任务
                    {
                        uint8_t active_task = (control_cycle / 10) % 3;
                        for(uint8_t i = 0; i < 3; i++){
                            if(i == active_task){
                                resume_task(i);
                            } else {
                                suspend_task(i);
                            }
                        }
                    }
                    break;
            }
        }
        
        // 统计总工作量
        volatile uint32_t total_work = 0;
        for(uint8_t i = 0; i < 3; i++){
            total_work += task_controls[i].run_count;
        }

        // 防止编译器优化
        (void)total_work;

        delay_ms(1000);
        taskYIELD();
    }
}

// 工作任务1
void Work1_Task(void* pvParameters){
    for(;;){
        task_controls[0].run_count++;
        
        // 模拟工作负载
        delay_ms(300);
        
        taskYIELD();  // 修正：taskYIELD而非task_YIELD
    }
}

// 工作任务2
void Work2_Task(void* pvParameters){  // 修正：函数名
    for(;;){
        task_controls[1].run_count++;
        
        // 模拟工作负载
        delay_ms(500);
        
        taskYIELD();
    }
}

// 工作任务3
void Work3_Task(void* pvParameters){  // 修正：函数名
    for(;;){
        task_controls[2].run_count++;
        
        // 模拟工作负载
        delay_ms(700);
        
        taskYIELD();
    }
}

int main(){
    // 创建控制器任务
    Controller_Handle = xTaskCreateStatic(
        Controller_Task,        // 任务函数
        "Controller",           // 任务名称
        CONTROLLER_STACK_SIZE,  // 任务堆栈大小
        NULL,                   // 任务参数
        4,                      // 任务优先级
        ControllerStack,        // 任务堆栈
        &ControllerTCB          // 任务控制块
    );

    // 创建工作任务1
    Work1_Handle = xTaskCreateStatic(  // 修正：变量名
        Work1_Task,             // 任务函数
        "Work1",                // 任务名称
        WORK_STACK_SIZE,        // 任务堆栈大小
        NULL,                   // 任务参数
        2,                      // 任务优先级
        Work1Stack,             // 任务堆栈
        &Work1TCB               // 任务控制块
    );

    // 创建工作任务2
    Work2_Handle = xTaskCreateStatic(  // 修正：变量名
        Work2_Task,             // 任务函数
        "Work2",                // 任务名称
        WORK_STACK_SIZE,        // 任务堆栈大小
        NULL,                   // 任务参数
        2,                      // 任务优先级
        Work2Stack,             // 任务堆栈
        &Work2TCB               // 任务控制块
    );

    // 创建工作任务3
    Work3_Handle = xTaskCreateStatic(  // 修正：变量名
        Work3_Task,             // 任务函数
        "Work3",                // 任务名称
        WORK_STACK_SIZE,        // 任务堆栈大小
        NULL,                   // 任务参数
        2,                      // 任务优先级
        Work3Stack,             // 任务堆栈
        &Work3TCB               // 任务控制块
    );

    // 启动调度器
    vTaskStartScheduler();  // 修正：函数名

    // 如果调度器启动失败，会执行到这里
    for(;;);

    return 0;  // 修正：拼写错误
}