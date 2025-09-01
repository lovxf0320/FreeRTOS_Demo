#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include <stdio.h>
#include <string.h>

/* 任务优先级定义 */
#define BANK_TASK_PRIORITY_1    (tskIDLE_PRIORITY + 1)
#define BANK_TASK_PRIORITY_2    (tskIDLE_PRIORITY + 2)
#define MONITOR_TASK_PRIORITY   (tskIDLE_PRIORITY + 3)

/* 任务栈大小 */
#define TASK_STACK_SIZE         (configMINIMAL_STACK_SIZE * 2)

/* 银行账户结构体 */
typedef struct {
    uint32_t account_id;        //账户ID
    volatile int32_t balance;   //账户余额
    char owner_name[32];        //账户持有人姓名
    uint32_t transaction_count; //该账户的交易次数
} BankAccount_t;

/* 全局银行账户数组 */
BankAccount_t g_accounts[4] = {
    {1001, 10000, "张三", 0},
    {1002, 5000,  "李四", 0}, 
    {1003, 8000,  "王五", 0},
    {1004, 15000, "赵六", 0}
};

/* 全局统计信息 */
typedef struct {
    volatile uint32_t total_transactions;    //总交易次数
    volatile uint32_t successful_transfers;  //成功转账次数
    volatile uint32_t failed_transfers;      //失败转账次数
    volatile int32_t total_amount_moved;     //总转账金额
} BankStats_t;

BankStats_t g_bank_stats = {0};

/* 任务句柄 */
TaskHandle_t xBankTask1Handle = NULL;
TaskHandle_t xBankTask2Handle = NULL;
TaskHandle_t xMonitorTaskHandle = NULL;


/**
 * @brief 安全的转账函数 - 使用临界段保护
 * @param from_id 转出账户ID
 * @param to_id 转入账户ID  
 * @param amount 转账金额
 * @return 0:成功, -1:失败
 */
int safe_transfer_money(uint32_t from_id, uint32_t to_id, int32_t amount){
    BankAccount_t *from_account=NULL;
    BankAccount_t *to_account=NULL;
    int result;

    //参数检查
    if(amount <= 0 || from_id == to_id) {
        return -1;
    }

    //查找账户
    from_account = find_account(from_id);
    to_account = find_account(to_id);
    
    if(!from_account || !to_account) {
        printf("❌ 转账失败: 账户不存在 (从:%lu 到:%lu)\n", from_id, to_id);
        return -1;
    }

    /* ========== 进入临界段 - 保护转账操作的原子性 ========== */
    taskENTER_CRITICAL();
    {
        //检查余额是否充足
        if(from_account->balance>=amount){
            /* 执行转账操作 */
            from_account->balance -= amount;
            to_account->balance += amount;
            
            /* 更新账户交易计数 */
            from_account->transaction_count++;
            to_account->transaction_count++;
            
            /* 更新全局统计信息 */
            g_bank_stats.total_transactions++;
            g_bank_stats.successful_transfers++;
            g_bank_stats.total_amount_moved += amount;

            printf("✅ 转账成功: %s->%s, 金额:%ld, 交易#%lu\n",
                   from_account->owner_name, to_account->owner_name, 
                   amount, g_bank_stats.total_transactions);
            printf("   %s余额: %ld, %s余额: %ld\n",
                   from_account->owner_name, from_account->balance,
                   to_account->owner_name, to_account->balance);
            
            result = 0;
        }else{
            /* 余额不足 */
            g_bank_stats.total_transactions++;
            g_bank_stats.failed_transfers++;

            printf("❌ 转账失败: %s余额不足 (需要:%ld, 余额:%ld)\n", 
                   from_account->owner_name, amount, from_account->balance);
            
            result = -1;
        }
    }
    taskEXIT_CRITICAL();

    return result;
}


/**
 * @brief 查找账户
 * @param account_id 账户ID
 * @return 账户指针，未找到返回NULL
 */
BankAccount_t* find_account(uint32_t account_id){
    for(int i=0;i<4;i++){
        if(g_accounts[i].account_id==account_id){
            return &g_accounts[i];
        }

    }
    return NULL;
}

/**
 * @brief 获取账户余额（安全版本）
 * @param account_id 账户ID
 * @return 账户余额，-1表示账户不存在
 */
int32_t get_account_balance_safe(uint32_t account_id){
    BankAccount_t *account = find_account(account_id);
    int32_t balance = -1;

    if(account){
        taskENTER_CRITICAL();
        {
            balance=account->balance;
        }
        taskEXIT_CRITICAL();
    }

    return balance;
}


/**
 * @brief 获取银行统计信息快照
 * @param stats_snapshot 统计信息存储位置
 */
void get_bank_stats_snapshot(BankStats_t *stats_snapshot){
    taskENTER_CRITICAL();
    {
        *stats_snapshot=g_bank_stats;
    }
    taskEXIT_CRITICAL();
}



/**
 * @brief 银行任务1 - 执行转账操作
 */
void vBankTask1(void *pvParameters){
    TickType_t xLastWakeTime;
    const TickType_t xFrequency=pdMS_TO_TICKS(1500);

    //初始化上次唤醒时间
    xLastWakeTime=xTaskGetTickCount();

    printf("银行任务1启动 - 执行常规转账\n");

    while(1){
        //张三向李四转账
        safe_transfer_money(1001,1002,200);
        vTaskDelayUntil(&xLastWakeTime,xFrequency);

        /* 王五向赵六转账 */
        safe_transfer_money(1003, 1004, 300);
        vTaskDelayUntil(&xLastWakeTime, xFrequency);
        
        /* 赵六向张三转账 */
        safe_transfer_money(1004, 1001, 150);
        vTaskDelayUntil(&xLastWakeTime, xFrequency);
    }
}


/**
 * @brief 银行任务2 - 执行转账操作
 */
void vBankTask2(void *pvParameters){
    TickType_t xLastWakeTime;
    const TickType_t xFrequency pdMS_TO_TICKS(2000);

    //初始化上次唤醒时间
    xLastWakeTime=xTaskGetTickCount();

    printf("银行任务2启动 - 执行大额转账\n");

    while(1){
        /* 李四向王五转账 */
        safe_transfer_money(1002, 1003, 500);
        vTaskDelayUntil(&xLastWakeTime, xFrequency);
        
        /* 赵六向李四转账 */
        safe_transfer_money(1004, 1002, 800);
        vTaskDelayUntil(&xLastWakeTime, xFrequency);
        
        /* 张三向赵六转账（可能会失败） */
        safe_transfer_money(1001, 1004, 2000);
        vTaskDelayUntil(&xLastWakeTime, xFrequency);
    }
}


/**
 * @brief 监控任务 - 定期显示银行状态
 */
void vMonitorTask(void *pvParameters){
    BankStats_t stats;
    TickType_t xLastWakeTime;
    const TickType_t xFrequency = pdMS_TO_TICKS(5000); // 5秒显示一次

    //初始化上次唤醒时间
    xLastWaskeTime=xTaskGetTickCount();

    printf("监控任务启动 - 定期显示银行状态\n");

    while(1){
        vTaskDelayUntil(&xLastWakeTime, xFrequency);
        
        /* 获取统计信息快照 */
        get_bank_stats_snapshot(&stats);

        printf("\n========== 银行状态报告 ==========\n");
        printf("总交易次数: %lu\n", stats.total_transactions);
        printf("成功转账: %lu, 失败转账: %lu\n", 
               stats.successful_transfers, stats.failed_transfers);
        
        printf("总转账金额: %ld\n", stats.total_amount_moved);
        printf("成功率: %.1f%%\n", 
               stats.total_transactions > 0 ? 
               (float)stats.successful_transfers * 100.0f / stats.total_transactions : 0.0f);
        
        printf("/n账户余额情况/n");
        for(int i=0;i<4;i++){
            int32_t balance=get_account_balance_safe(g_accounts[i].account_id);
            printf("  %s(ID:%lu): %ld元, 交易次数:%lu\n",
                   g_accounts[i].owner_name, g_accounts[i].account_id,
                   balance, g_accounts[i].transaction_count);
        }
        printf("================================\n\n");

    }
}


void create_bank_demo_tasks(){
    BaseType_t xReturn;

    //创建银行任务1
    xReturn=xTaskCreate(vBankTask1,
                    "BankTask1",
                    TASK_STACK_SIZE,
                    NULL,
                    BANK_TANK_PRIORITY_1,
                    &xBankTask1Handle
                    );
    if(xReturn!=pdPASS){
        printf("银行任务1创建失败!\n");
        return;
    }

    //创建银行任务1
    xReturn=xTaskCreate(vBankTask2,
                    "BankTask2",
                    TASK_STACK_SIZE,
                    NULL,
                    BANK_TANK_PRIORITY_2,
                    &xBankTask2Handle
                    );
    if(xReturn!=pdPASS){
        printf("银行任务2创建失败!\n");
        return;
    }

    //创建银行任务1
    xReturn=xTaskCreate(vMonitorTask1,
                    "Monitor",
                    TASK_STACK_SIZE,
                    NULL,
                    BANK_TANK_PRIORITY,
                    &xMonitorHandle
                    );
    if(xReturn!=pdPASS){
        printf("监控任务创建失败!\n");
        return;
    }

    printf("🚀 银行转账系统Demo启动成功!\n");
    printf("💡 观察多任务环境下临界段如何保护转账操作的原子性\n\n");

}


int main(void){
    printf("=== FreeRTOS 临界段保护 Demo 1: 银行转账系统 ===\n\n");

    /*显示初始账户状态*/
    printf("初始账户状态:\n");
    for(int i=0;i<4;i++){
        printf("  %s(ID:%lu): %ld元\n",
            g_accounts[i].owner_name, g_accounts[i].account_id, g_accounts[i].balance);
    }

    //创建demo任务（把创建任务从main中分离了）
    create_bank_demo_tasks();

    //启动调度器
    vTaskStartScheduler();

    for(;;);


    return 0;
}


