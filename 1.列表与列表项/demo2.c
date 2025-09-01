#include "list.h"
#include <stdio.h>

#define TASK_PRIORITY_1 1
#define TASK_PRIORITY_2 2
#define TASK_PRIORITY_3 3
#define TASK_PRIORITY_4 4
#define TASK_PRIORITY_5 5

typedef struct{
    char taskName[20];
    int taskID;
} TaskControlBlock_t;

struct xLIST List_Test;
struct xLIST_ITEM List_Item[5];
TaskControlBlock_t Tasks[5];

void main(void) {

    // 1. Initialize the test list
    vListInitialise(&List_Test);

    // 2. Initialize the list items
    sprintf(Tasks[0].taskName, "Task1");
    Tasks[0].taskID = 5;

    sprintf(Tasks[1].taskName, "Task2");
    Tasks[1].taskID = 4;

    sprintf(Tasks[2].taskName, "Task3");
    Tasks[2].taskID = 3;

    sprintf(Tasks[3].taskName, "Task4");
    Tasks[3].taskID = 2;

    sprintf(Tasks[4].taskName, "Task5");
    Tasks[4].taskID = 1;

    // 3. Initialize and set values for list items
    vListInitialiseItem(&List_Item[0]);                         // 初始化列表项
    listSET_LIST_ITEM_VALUE(&List_Item[0], TASK_PRIORITY_1);    // 设置列表项的值
    listSET_LIST_ITEM_OWNER(&List_Item[0], &Tasks[0]);          // 设置任务项的所有者

    vListInitialiseItem(&List_Item[1]);                         // 初始化列表项
    listSET_LIST_ITEM_VALUE(&List_Item[1], TASK_PRIORITY_2);    // 设置列表项的值
    listSET_LIST_ITEM_OWNER(&List_Item[1], &Tasks[1]);          // 设置任务项的所有者

    vListInitialiseItem(&List_Item[2]);                         // 初始化列表项
    listSET_LIST_ITEM_VALUE(&List_Item[2], TASK_PRIORITY_3);    // 设置列表项的值
    listSET_LIST_ITEM_OWNER(&List_Item[2], &Tasks[2]);          // 设置任务项的所有者

    vListInitialiseItem(&List_Item[3]);                         // 初始化列表项
    listSET_LIST_ITEM_VALUE(&List_Item[3], TASK_PRIORITY_4);    // 设置列表项的值
    listSET_LIST_ITEM_OWNER(&List_Item[3], &Tasks[3]);          // 设置任务项的所有者

    vListInitialiseItem(&List_Item[4]);                         // 初始化列表项
    listSET_LIST_ITEM_VALUE(&List_Item[4], TASK_PRIORITY_5);    // 设置列表项的值
    listSET_LIST_ITEM_OWNER(&List_Item[4], &Tasks[4]);          // 设置任务项的所有者

    // 4. 按顺序插入任务到链表（注意：会自动排序）
    vListInsert(&List_Test, &List_Item[0]);
    vListInsert(&List_Test, &List_Item[1]);
    vListInsert(&List_Test, &List_Item[2]);
    vListInsert(&List_Test, &List_Item[3]);
    vListInsert(&List_Test, &List_Item[4]);

    // 5. 显示链表的状态
    printf("链表的状态为：\n");
    printf("列表中项目的个数为：%d\n", listCURRENT_LIST_LENGTH(&List_Test));
    printf("链表是否为空：%s\n", listLIST_IS_EMPTY(&List_Test) ? "是" : "否");

    // 6. 遍历链表并打印所有任务（按优先级排序）
    if (!listLIST_IS_EMPTY(&List_Test)) {
        ListItem_t *pxIterator;         // 链表项指针，指向当前项
        TaskControlBlock_t *pxTask;     // 任务控制块指针，指向当前任务
        int index = 1;

        // 获取链表头部项
        pxIterator = listGET_HEAD_ENTRY(&List_Test);

        // 遍历链表
        do {
            pxTask = (TaskControlBlock_t *)listGET_LIST_ITEM_OWNER(pxIterator); // 获取当前项的所有者
            printf("任务%d: 名称=%s, ID=%d, 优先级=%d\n", 
                            index, 
                            pxTask->taskName, 
                            pxTask->taskID, 
                            listGET_LIST_ITEM_VALUE(pxIterator));

            pxIterator = listGET_NEXT(pxIterator); // 获取下一个项
        } while (pxIterator != listGET_HEAD_ENTRY(&List_Test));
    }

    // 7. 查找最高优先级任务（最小值）
    ListItem_t *pxHighestPriorityItem;
    TaskControlBlock_t *pxHighestPriorityTask;

    if (!listLIST_IS_EMPTY(&List_Test)) {
        pxHighestPriorityItem = listGET_HEAD_ENTRY(&List_Test);
        pxHighestPriorityTask = (TaskControlBlock_t *)listGET_LIST_ITEM_OWNER(pxHighestPriorityItem);

        printf("最高优先级任务(用列表项的值模拟优先级)：名称=%s, ID=%d, 优先级=%d\n", 
                pxHighestPriorityTask->taskName, 
                pxHighestPriorityTask->taskID, 
                listGET_LIST_ITEM_VALUE(pxHighestPriorityItem));
    } else {
        printf("链表为空，无法找到最高优先级任务。\n");
    }

    // 8. 统计信息
    printf("链表统计信息：\n");
    printf("总任务数：%d\n", listCURRENT_LIST_LENGTH(&List_Test));
    printf("最高优先级(用列表项的值模拟优先级)的值为：%d\n", 
            listGET_LIST_ITEM_VALUE(pxHighestPriorityItem));

    // 9. 演示链表的循环特性
    printf("从头部开始，连续获取3个项目：\n");
    ListItem_t *pxCurrent = listGET_HEAD_ENTRY(&List_Test);
    for (int i = 0; i < 3; i++) {
        TaskControlBlock_t *pxTask = (TaskControlBlock_t *)listGET_LIST_ITEM_OWNER(pxCurrent);
        printf("第%d个：%s（优先级为%d）\n",
                    i + 1,
                    pxTask->taskName,
                    listGET_LIST_ITEM_VALUE(pxCurrent));

        pxCurrent = listGET_NEXT(pxCurrent);
    }

    // 主循环
    for (;;) {
        // 在实际应用中，这里会是任务调度器
        // 为了演示，我们直接退出
        break;
    }

    return 0;
}
