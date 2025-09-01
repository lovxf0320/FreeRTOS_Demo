/*
 * Demo3: 任务切换与优先级机制演示
 * 
 * 学习目标：
 * 1. 理解任务优先级的作用
 * 2. 观察高优先级任务如何抢占低优先级任务
 * 3. 理解vTaskSwitchContext()的工作原理
 * 4. 学习任务状态转换
 */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include <stdio.h>
#include <string.h>

//任务状态跟踪
typedef struct{
    char name[20];
    uint32_t run_count;
    uint32_t last_run_time;
    eTaskState state;
}TaskInfo_t;

TaskInfo_t task_info[4];
TaskHandle_t task_handles[4];

//获取当前时间（简化版）
uint32_t get_current_time(void){
    return xTaskGetTickCount();
}

//打印任务状态信息
void print_task_info(const char* task_name, int task_id, const char* action){
    task_info[task_id].run_count++;
    task_info[task_id].last_run_time = get_current_time();

    printf("[时间:%4u] %s %s (运行次数:%u)\n",
            task_info[task_id].last_run_time,
            task_name,
            action,
            task_info[task_id].run_count);
}

//高优先级任务（优先级4）
void HighPriorityTask(void *pvParameters){
    strcpy(task_info[0].name, "高优先级任务");

    while(1){
        print_task_info("高优先级任务", 0, ">>> 开始运行");

        //模拟工作
        printf("高优先级任务正在执行重要工作...\n");
        vTaskDelay(pdMS_TO_TICKS(100));

        print_task_info("高优先级任务", 0, "<<< 完成工作，进入延时");
        
        // 延时3秒，让其他任务有机会运行
        vTaskDelay(pdMS_TO_TICKS(3000));
    }
}

//中优先级任务（优先级2）
void MediumPriorityTask(void *pvParameters){
    strcpy(task_info[1].name, "中优先级任务");

    while(1){
        print_task_info("中优先级任务", 1, ">>> 开始运行");

        for(int i = 0; i < 5; i++){
            vTaskDelay(pdMS_TO_TICKS(500));

            if(i == 2){
                printf("中优先级任务：工作到一半，可能会被抢占\n");
            }
        }
        
        print_task_info("中优先级任务", 1, "<<< 完成所有工作");

        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

//低优先级任务（优先级1）
void LowPriorityTask(void *pvParameters){
    strcpy(task_info[2].name, "低优先级任务");

    while(1){
        print_task_info("低优先级任务", 2, ">>> 开始运行");

        // 模拟持续的后台工作
        for(int i = 0; i < 10; i++){
            printf("    低优先级任务后台工作... (%d/10)\n", i+1);
            vTaskDelay(pdMS_TO_TICKS(200)); // 每次工作200ms
        }
        
        print_task_info("低优先级任务", 2, "<<< 完成后台工作");
        
        // 短暂延时
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

//系统监控任务（最高优先级5）
void MonitorTask(void *pvParameters){
    strcpy(task_info[3].name, "监控任务");

    while(1){
        // 延时10秒后打印系统状态
        vTaskDelay(pdMS_TO_TICKS(10000));
        
        printf("\n");
        for(int j = 0; j < 50; j++) printf("=");
        printf("\n");
        printf("=== 系统任务状态监控报告 ===\n");
        printf("当前系统时间: %u ticks\n", get_current_time());

        //遍历所有任务，获取状态
        for(int i = 0; i < 3; i++){
            eTaskState state = eTaskGetState(task_handles[i]);
            const char* state_name;

            switch(state){
                case eRunning:   state_name = "运行中"; break;
                case eReady:     state_name = "就绪"; break;
                case eBlocked:   state_name = "阻塞"; break;
                case eSuspended: state_name = "挂起"; break;
                case eDeleted:   state_name = "已删除"; break;
                default:         state_name = "未知"; break;
            }

            printf("任务: %-12s | 状态: %-6s | 运行次数: %3u | 最后运行: %u\n",
                   task_info[i].name,
                   state_name,
                   task_info[i].run_count,
                   task_info[i].last_run_time);
        }

        for(int j = 0; j < 50; j++) printf("=");
        printf("\n\n");
    }
}

//临时超高优先级任务函数
void TempHighPriorityTask(void *pvParameters){
    printf("*** 超高优先级任务运行！抢占所有其他任务！***\n");
    vTaskDelay(pdMS_TO_TICKS(1000));
    printf("*** 超高优先级任务完成，自我删除 ***\n");
    vTaskDelete(NULL);
}

//演示任务抢占的特殊任务
void PreemptionDemoTask(void *pvParameters){
    //等待10秒后开始演示
    vTaskDelay(pdMS_TO_TICKS(10000));

    while(1){
        printf("\n!!! 抢占演示开始 !!!\n");
        printf("创建一个超高优先级任务来演示抢占...\n");
        
        // 创建一个临时的超高优先级任务
        TaskHandle_t temp_task;
        xTaskCreate(
            TempHighPriorityTask,
            "TempHighPrio",
            1000,
            NULL,
            6,
            &temp_task
        );

        //等待30秒后再次演示
        vTaskDelay(pdMS_TO_TICKS(30000));
    }
}

int main(void){
    printf("=== FreeRTOS 任务切换与优先级机制演示 ===\n\n");
    
    printf("任务优先级设置:\n");
    printf("- 监控任务:     优先级 5 (最高)\n");
    printf("- 高优先级任务: 优先级 4\n");
    printf("- 中优先级任务: 优先级 2\n");
    printf("- 低优先级任务: 优先级 1 (最低)\n\n");

    printf("观察要点:\n");
    printf("1. 高优先级任务会抢占正在运行的低优先级任务\n");
    printf("2. 任务在延时期间会让出CPU给其他任务\n");
    printf("3. 监控任务会定期报告所有任务的状态\n\n");

    //创建各个任务
    xTaskCreate(HighPriorityTask, "HighPrio", 1000, NULL, 4, &task_handles[0]);
    xTaskCreate(MediumPriorityTask, "MedPrio", 1000, NULL, 2, &task_handles[1]);
    xTaskCreate(LowPriorityTask, "LowPrio", 1000, NULL, 1, &task_handles[2]);
    xTaskCreate(MonitorTask, "Monitor", 1000, NULL, 5, &task_handles[3]);

    //可选：创建抢占演示任务
    //xTaskCreate(PreemptionDemoTask, "PreemptDemo", 1000, NULL, 3, NULL);

    printf("所有任务已创建，启动调度器...\n\n");

    //启动调度器
    vTaskStartScheduler();

    printf("调度器启动失败！\n");
    while(1);
}

/*
 * 预期现象：
 * 1. 低优先级任务开始运行后台工作
 * 2. 中优先级任务会抢占低优先级任务
 * 3. 高优先级任务会抢占中优先级任务（即使中优先级任务正在工作）
 * 4. 监控任务每10秒打印一次所有任务的状态
 * 5. 可以清楚看到任务切换的时机和优先级抢占的效果
 * 
 * 学习重点：
 * 1. 理解优先级抢占机制
 * 2. 观察任务状态变化（运行->就绪->阻塞）
 * 3. 理解为什么需要合理设计任务优先级
 */