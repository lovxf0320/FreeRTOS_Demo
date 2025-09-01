// 题目2：链表项的删除和重新插入
// 任务描述：创建一个链表，插入3个任务项。
// 然后删除中间的一个项，修改它的优先级后重新插入，观察链表的变化。

// 重新插入的意义：
// 模拟任务优先级动态变化
// 理解链表的动态特性
// 掌握FreeRTOS中任务状态切换的基础

#include <stdio.h>
#include "list.h"
#include "string.h"

// 模拟任务控制块结构
typedef struct {
    char taskName[20];  // 任务名称
    int taskID;         // 任务ID
    int currentPriority; // 当前优先级
    int originalPriority; // 原始优先级
} TaskControlBlock_t;

// 全局变量声明
struct xLIST TaskReadyList;     // 就绪任务队列
struct xLIST_ITEM List_Item[3]; // 链表项数组
TaskControlBlock_t Tasks[3];    // 任务控制块数组

void initializeTasks();
void printListContents(struct xLIST *pxList, const char *message);
void printListStatus(struct xLIST *pxList, char *ListName);
void demostrateRemoveAndReinsert();

int main() {
    // 1. 初始化链表
    vListInitialise(&TaskReadyList);

    // 2. 初始化任务控制块
    initializeTasks();

    // 3. 初始化链表项
    initializelistItem();

    // 4. 演示删除和重新插入
    demostrateRemoveAndReinsert();

    for (;;) {
        // 在实际应用中，这里会是任务调度器
        // 为了演示，我们直接退出
        break;
    }
}

void initializeTasks() {
    sprintf(Tasks[0].taskName, "Task1");
    Tasks[0].taskID = 1;
    Tasks[0].currentPriority = 2;
    Tasks[0].originalPriority = 2;

    sprintf(Tasks[1].taskName, "Task2");
    Tasks[1].taskID = 2;
    Tasks[1].currentPriority = 5;
    Tasks[1].originalPriority = 5;

    sprintf(Tasks[2].taskName, "Task3");
    Tasks[2].taskID = 3;
    Tasks[2].currentPriority = 8;
    Tasks[2].originalPriority = 8;
}

void initializelistItem() {
    for (int i = 0; i < 3; i++) {
        vListInitialiseItem(&List_Item[i]); // 初始化列表项
        listSET_LIST_ITEM_VALUE(&List_Item[i], Tasks[i].currentPriority); // 设置列表项的值为当前优先级
        listSET_LIST_ITEM_OWNER(&List_Item[i], &Tasks[i]); // 设置任务项的所有者
    }
}

void demostrateRemoveAndReinsert() {
    printf("开始演示删除和重新插入...\n");

    // 显示初始状态
    printListContents(&TaskReadyList, "初始状态");

    // 第一步：删除中间的任务 (DisplayTask)
    // 找到要删除的任务项（DisplayTask）
    struct xLIST_ITEM *pxItemToRemove = &List_Item[1];
    TaskControlBlock_t *pxTaskToModify = &Tasks[1];

    printf("准备删除任务: %s (当前优先级: %d)\n",
           pxTaskToModify->taskName,
           listGET_LIST_ITEM_VALUE(pxItemToRemove));

    // 执行删除操作
    UBaseType_t remainingItems = uxListRemove(pxItemToRemove);

    printf("删除操作完成\n");
    printf("剩余任务量：%d\n", remainingItems);

    // 打印链表的状态
    printListContents(&TaskReadyList, "删除后状态");

    if (pxItemToRemove->pxContainer == NULL) {
        printf("任务 %s 已成功从链表中删除。\n", pxTaskToModify->taskName);
    } else {
        printf("任务 %s 删除失败，仍在链表中。\n", pxTaskToModify->taskName);
    }

    // 第二步：修改优先级
    printf("步骤2：修改任务优先级\n");

    int oldPriority = pxTaskToModify->currentPriority;
    int newPriority = 1; // 新优先级

    printf("任务 %s 的优先级从 %d 修改为 %d\n",
           pxTaskToModify->taskName,
           oldPriority,
           newPriority);

    // 更新任务控制块中的优先级
    pxTaskToModify->currentPriority = newPriority;

    // 更新列表项的值
    listSET_LIST_ITEM_VALUE(pxItemToRemove, newPriority);

    printf("优先级修改完成\n");
    printf("任务控制块优先级: %d\n", pxTaskToModify->currentPriority);
    printf("链表项值: %d\n", listGET_LIST_ITEM_VALUE(pxItemToRemove));

    // 第三步：重新插入到链表
    printf("步骤3：重新插入任务到链表\n");
    printf("将 %s 重新插入到链表中...\n", pxTaskToModify->taskName);
    vListInsert(&TaskReadyList, pxItemToRemove);

    printf("重新插入完成\n");
    printf("链表长度: %d\n", listCURRENT_LIST_LENGTH(&TaskReadyList));

    // 验证插入结果
    if (pxItemToRemove->pxContainer == &TaskReadyList) {
        printf("项目容器已重新设置\n");
    }

    printListContents(&TaskReadyList, "重新插入后状态");

    // 第四步：再次演示删除和插入（模拟优先级恢复）
    printf("步骤4：演示优先级恢复\n:");
    printf("模拟任务完成紧急工作，恢复原始优先级\n");

    uxListRemove(pxItemToRemove); // 再次删除
    printf("任务 %s 已从链表中删除\n", pxTaskToModify->taskName);

    // 恢复原始优先级
    pxTaskToModify->currentPriority = pxTaskToModify->originalPriority;
    listSET_LIST_ITEM_VALUE(pxItemToRemove, pxTaskToModify->currentPriority);

    // 重新插入
    vListInsert(&TaskReadyList, pxItemToRemove);
    printf("任务 %s 已恢复原始优先级并重新插入链表\n", 
            pxTaskToModify->taskName);

    printListContents(&TaskReadyList, "恢复原始优先级后状态");
}

void printListContents(struct xLIST *pxList, const char *message) {
    printf("%s\n", message);

    if (LIST_IS_EMPTY(pxList)) {
        printf("链表为空\n");
        return;
    }

    struct xLIST_ITEM *pxIterator = listGET_HEAD_ENTRY(pxList);
    struct TaskControlBlock_t *pxTask;
    int index = 1;

    printf("║ 序号 │      任务名      │ 任务ID │ 优先级 │      地址      ║\n");
    printf("╠═══════════════════════════════════════════════════════════╣\n");

    do {
        pxTask = (struct TaskControlBlock_t *)listGET_LIST_ITEM_OWNER(pxIterator);

        printf("║  %2d  │ %-15s │   %2d   │   %2d   │ %p ║\n",
               index,
               pxTask->taskName,
               pxTask->taskID,
               listGET_LIST_ITEM_VALUE(pxIterator),
               (void*)pxTask);

        pxIterator = listGET_NEXT(pxIterator);
    } while (pxIterator != listGET_HEAD_ENTRY(pxList));

    printf("╚═══════════════════════════════════════════════════════════╝\n");
    printf("链表长度: %d | 最高优先级: %d\n",
           listCURRENT_LIST_LENGTH(pxList),
           listGET_LIST_ITEM_VALUE(listGET_HEAD_ENTRY(pxList)));
}
