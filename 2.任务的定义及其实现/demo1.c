// Demo 1: 基础任务创建与运行（修正版）
// 重点:展示最基本的任务创建和运行
// 特色:3个不同闪烁频率的LED任务,演示并发执行
// 学习要点:任务函数结构、堆栈分配、任务控制块的使用

#include "FreeRTOS.h"
#include "task.h"

// 任务句柄定义（修正拼写错误）
TaskHandle_t RedLED_Handle;
TaskHandle_t GreenLED_Handle;
TaskHandle_t BlueLED_Handle;

// 任务堆栈大小定义（去掉分号）
#define LED_STACK_SIZE 128

// 任务堆栈
StackType_t RedLEDStack[LED_STACK_SIZE];
StackType_t GreenLEDStack[LED_STACK_SIZE];
StackType_t BlueLEDStack[LED_STACK_SIZE];

// 任务控制块
StaticTask_t RedLEDTCB;   // 使用StaticTask_t而不是StackType_t
StaticTask_t GreenLEDTCB;
StaticTask_t BlueLEDTCB;

// 模拟LED状态的全局变量
volatile uint8_t red_led_state = 0;
volatile uint8_t green_led_state = 0;
volatile uint8_t blue_led_state = 0;

// 延时函数
void delay_ms(uint32_t ms) {
    // 简单的延时实现（实际项目中应使用vTaskDelay）
    // for(uint32_t i = 0; i < ms * 1000; i++);
    vTaskDelay(pdMS_TO_TICKS(ms));
}

// 红色LED任务——快速闪烁（200ms间隔）
void RedLED_Task(void *pvParameters) {
    for(;;) {
        red_led_state = 1;    // 点亮红色LED
        delay_ms(200);
        red_led_state = 0;    // 熄灭红色LED（修正错误）
        delay_ms(200);
        
        taskYIELD();          // 主动让出CPU
    }
}

// 绿色LED任务——中速闪烁（500ms间隔）
void GreenLED_Task(void *pvParameters) {
    for(;;) {
        green_led_state = 1;   // 点亮绿色LED
        delay_ms(500);
        green_led_state = 0;   // 熄灭绿色LED（修正错误）
        delay_ms(500);
        
        taskYIELD();           // 主动让出CPU
    }
}

// 蓝色LED任务——慢速闪烁（1000ms间隔）
void BlueLED_Task(void *pvParameters) {
    for(;;) {
        blue_led_state = 1;    // 点亮蓝色LED
        delay_ms(1000);
        blue_led_state = 0;    // 熄灭蓝色LED（修正错误）
        delay_ms(1000);
        
        taskYIELD();           // 主动让出CPU
    }
}

int main(void) {
    // 创建红色LED任务
    RedLED_Handle = xTaskCreateStatic(
        RedLED_Task,        // 任务函数
        "RedLED",          // 任务名称
        LED_STACK_SIZE,    // 堆栈大小
        NULL,              // 传递给任务的参数
        1,                 // 任务优先级
        RedLEDStack,       // 堆栈数组
        &RedLEDTCB         // 任务控制块
    );

    // 创建绿色LED任务
    GreenLED_Handle = xTaskCreateStatic(
        GreenLED_Task,
        "GreenLED",
        LED_STACK_SIZE,
        NULL,
        2,                 // 优先级2
        GreenLEDStack,
        &GreenLEDTCB
    );

    // 创建蓝色LED任务（修正句柄名称拼写）
    BlueLED_Handle = xTaskCreateStatic(
        BlueLED_Task,
        "BlueLED",
        LED_STACK_SIZE,
        NULL,
        2,                 // 优先级2
        BlueLEDStack,
        &BlueLEDTCB
    );

    // 启动调度器
    vTaskStartScheduler();

    // 正常情况下不会到达这里
    for(;;);

    return 0;
}

/* 
执行流程说明：
1. main()函数创建三个静态任务
2. vTaskStartScheduler()启动调度器
3. 调度器开始管理任务执行：
   - 红色LED以200ms间隔闪烁
   - 绿色LED以500ms间隔闪烁  
   - 蓝色LED以1000ms间隔闪烁
4. 任务通过taskYIELD()主动让出CPU，实现协作式调度
5. 系统在三个任务间切换，实现并发执行的效果
*/