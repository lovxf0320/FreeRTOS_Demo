/*
 * Demo2: 阻塞延时 vs 软件延时对比演示
 * 
 * 学习目标：
 * 1. 直观理解阻塞延时和软件延时的区别
 * 2. 观察CPU利用率的差异
 * 3. 理解为什么RTOS要使用阻塞延时
 */

#include "FreeRTOS.h"
#include "task.h"


//计数器，用于观察系统效率
volatile uint32_t system_tick_counter=0;
volatile uint32_t background_work_counter=0;


//软件延时函数（浪费CPU的延时）
void software_delay(uint32_t delay_ms){
    //这是一个粗略的软件延时，实际延时时间取决于CPU频率
    uint32_t i,j;
    for(i=0;i<delay_ms;i++){

        for(j=0;j<delay_ms;j++){

            __asm("nop");       //空操作，消耗CPU时间

        }

    }
}


//演示任务1：使用软件延时
void Task_SoftwareDelay(void *pvParameters){
    while(1){
        printf("[软件延时任务] 开始工作\n");

        //使用软件延时1秒 - CPU被完全占用！
        printf("[软件延时任务] 开始软件延时1秒...\n");
        software_delay(1000);
        printf("[软件延时任务] 软件延时结束\n");
        
        printf("[软件延时任务] 完成工作，准备下次循环\n\n");
    }
}


//演示任务2：使用阻塞延时
void Task_BlockingDelay(void *pvParameters){
    while(1){
        printf("[阻塞延时任务] 开始工作\n");
        
        //使用阻塞延时1秒 - CPU可以去做其他事情！
        printf("[阻塞延时任务] 开始阻塞延时1秒...\n");
        vTaskDelay(pdMS_TO_TICKS(1000));
        printf("[阻塞延时任务] 阻塞延时结束\n");

        printf("[阻塞延时任务] 完成工作，准备下次循环\n\n");
    }

}


//后台任务：模拟其他重要工作
void Task_Background(void *pvParameters){
    while(1){
        background_work_counter++;

        //每1000次打印一次，避免刷屏
        if(background_work_counter%1000==0){
            printf("    [后台任务] 完成了 %u 次后台工作\n", 
                   background_work_counter);
        }

        // 小延时，让其他任务有机会运行
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}


//系统监控任务
void Task_Monitor(void *pvParameters){
    uint32_t last_background_count=0;

    while(1){
        vTaskDelay(pdMS_TO_TICKS(5000));  // 每5秒监控一次

        uint32_t work_done=background_work_counter-last_background_count;
        last_background_count=background_work_counter;

        printf("\n=== 系统性能监控 ===\n");
        printf("过去5秒内后台任务完成工作: %u 次\n", work_done);
        printf("系统节拍计数: %u\n", system_tick_counter);

        if(work_done>1000){
            printf("系统效率高 - 使用了阻塞延时\n");
        }else{
            printf("系统效率低 - 可能使用了软件延时\n");
        }

        printf("==================\n\n");
    }
}


int main(void){
    printf("=== 阻塞延时 vs 软件延时对比演示 ===\n\n");
    printf("实验说明：\n");
    printf("1. 首先运行使用软件延时的任务，观察系统效率\n");
    printf("2. 然后切换到阻塞延时的任务，对比系统效率\n\n");

    //创建后台任务（始终运行）
    xTaskCreate(Task_Background,"Background",1000,NULL,1,NULL);

    //创建监控任务
    xTaskCreate(Task_Monitor,"Monitor",1000,NULL,3,NULL);

    //方案1：创建软件延时任务（注释掉方案2）
    printf("当前演示：软件延时任务\n");
    printf("预期：后台任务几乎无法运行，系统效率极低\n\n");
    xTaskCreate(Task_SoftwareDelay, "SoftDelay", 1000, NULL, 2, NULL);

    // 方案2：创建阻塞延时任务（注释掉方案1来测试）
    // printf("当前演示：阻塞延时任务\n");
    // printf("预期：后台任务可以正常运行，系统效率高\n\n");
    // xTaskCreate(Task_BlockingDelay, "BlockDelay", 1000, NULL, 2, NULL);

    //启动调度器
    vTaskStartScheduler();

    printf("调度器启动失败！\n");
    while(1);
}
/*
 * 实验步骤：
 * 1. 先运行软件延时版本，观察后台任务几乎无法运行
 * 2. 注释掉软件延时任务，启用阻塞延时任务
 * 3. 重新编译运行，观察后台任务可以正常工作
 * 
 * 关键观察点：
 * 1. 软件延时时：后台工作计数几乎不增长
 * 2. 阻塞延时时：后台工作计数快速增长
 * 3. 系统监控会显示效率差异
 */