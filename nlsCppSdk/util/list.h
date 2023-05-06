/*
 * Copyright 2015 Alibaba Group Holding Limited
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef NLS_SDK_LIST_H
#define NLS_SDK_LIST_H

namespace AlibabaNls {
namespace utility {

//双向链表节点
struct NlsListHead {
    struct NlsListHead *next, *prev;
};

//typedef struct NlsListHead NlsListHeadT;

//初始化节点: 设置name节点的前继节点和后继节点都是指向name本身
#define NLS_LIST_HEAD_INIT(name) { &(name), &(name) }

//定义表头(节点): 新建双向链表表头name，并设置name的前继节点和后继节点都是指向name本身
#define NLS_LIST_HEAD(name) struct NlsListHead name = NLS_LIST_HEAD_INIT(name)

//初始化节点: 将list节点的前继节点和后继节点都是指向list本身
static inline void nlsInitListHead(struct NlsListHead* list) {
    list->next = list;
    list->prev = list;
}

//添加节点: 将new插入到prev和next之间
static inline void _nlsListAdd(struct NlsListHead* node,
                               struct NlsListHead* prev,
                               struct NlsListHead* next){
    next->prev = node;
    node->next = next;
    node->prev = prev;
    prev->next = node;
 }

//添加new节点: 将new添加到head之后，是new称为head的后继节点
static inline void nlsListAdd(struct NlsListHead* node, struct NlsListHead* head) {
    _nlsListAdd(node, head, head->next);
}

//添加new节点: 将new添加到head之前，即将new添加到双链表的末尾
static inline void nlsListAddTail(struct NlsListHead* node, struct NlsListHead *head){
    _nlsListAdd(node, head->prev, head);
}

//从双链表中删除entry节点
static inline void _nlsListDelete(struct NlsListHead *prev, struct NlsListHead *next) {
    next->prev = prev;
    prev->next = next;
}

//从双链表中删除entry节点
static inline void nlsListDelete(struct NlsListHead *entry) {
    _nlsListDelete(entry->prev, entry->next);
}

//从双链表中删除entry节点
static inline void _nlsListDeleteEntry(struct NlsListHead *entry) {
    _nlsListDelete(entry->prev, entry->next);
}

//从双链表中删除entry节点, 并将entry节点的前继节点和后继节点都指向entry本身
static inline void nlsListDeleteInit(struct NlsListHead *entry) {
    _nlsListDeleteEntry(entry);
    nlsInitListHead(entry);
}

//用new节点取代old节点
static inline void nlsListReplace(struct NlsListHead *old, struct NlsListHead *node){
    node->next = old->next;
    node->next->prev = node;
    node->prev = old->prev;
    node->prev->next = node;
}

//双链表是否为空
static inline int nlsListEmpty(const struct NlsListHead *head) {
    return head->next == head;
}

//获取"MEMBER成员"在"结构体TYPE"中的位置偏移
#define nlsOffsetof(TYPE, MEMBER) ((size_t) ((char*)(&((TYPE *)1)->MEMBER) - 1))

//根据"结构体(type)变量"中的"域成员变量(member)的指针(ptr)"来获取指向整个结构体变量的指针
#define nlsContainerOf(ptr, type, member) ({  \
    const typeof( ((type *)0)->member ) *__mptr = (ptr);    \
    (type *)( (char *)__mptr - nlsOffsetof(type,member) );})

//遍历双向链表
#define NLS_LIST_FOR_EACH(pos, head)    \
    for (pos = (head)->next; pos != (head); pos = pos->next)

#define NLS_LIST_FOR_EACH_SAFE(pos, n, head) \
        for(pos=(head)->next, n=pos->next; pos!=(head); pos=n, n=pos->next)

#define NLS_LIST_ENTRY(ptr, type, member) nlsContainerOf(ptr, type, member)

}
}

#endif  //NLS_SDK_LIST_H

/*

struct person
{
    int age;
    char name[20];
    struct list_head list;
};

void main(int argc, char* argv[])
{
    struct person *pperson;
    struct person person_head;
    struct list_head *pos, *next;
    int i;

    // 初始化双链表的表头
    INIT_LIST_HEAD(&person_head.list);

    // 添加节点
    for (i=0; i<5; i++)
    {
        pperson = (struct person*)malloc(sizeof(struct person));
        pperson->age = (i+1)*10;
        sprintf(pperson->name, "%d", i+1);
        // 将节点链接到链表的末尾
        // 如果想把节点链接到链表的表头后面，则使用 list_add
        list_add_tail(&(pperson->list), &(person_head.list));
    }

    // 遍历链表
    printf("==== 1st iterator d-link ====\n");
    list_for_each(pos, &person_head.list)
    {
        pperson = list_entry(pos, struct person, list);
        printf("name:%-2s, age:%d\n", pperson->name, pperson->age);
    }

    // 删除节点age为20的节点
    printf("==== delete node(age:20) ====\n");
    list_for_each_safe(pos, next, &person_head.list) NLS_LIST_FOR_EACH_SAFE
    {
        pperson = list_entry(pos, struct person, list);
        if(pperson->age == 20)
        {
            list_del_init(pos);
            free(pperson);
        }
    }

    // 再次遍历链表
    printf("==== 2nd iterator d-link ====\n");
    list_for_each(pos, &person_head.list)
    {
        pperson = list_entry(pos, struct person, list);
        printf("name:%-2s, age:%d\n", pperson->name, pperson->age);
    }

    // 释放资源
    list_for_each_safe(pos, next, &person_head.list)
    {
        pperson = list_entry(pos, struct person, list);
        list_del_init(pos);
        free(pperson);
    }

}

 * */

