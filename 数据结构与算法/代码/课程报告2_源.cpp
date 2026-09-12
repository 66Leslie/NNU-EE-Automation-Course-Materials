#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

#define INITIAL_STACK_SIZE 10000000  // 初始栈大小

// 栈结构体
typedef struct 
{
    char* data;
    int top;
    int capacity;
} Stack;

// 初始化栈
void initStack(Stack* stack, int initialCapacity) 
{
    stack->data = (char*)malloc(initialCapacity * sizeof(char));
    if (stack->data == NULL) 
    {
        printf("内存分配失败！\n");
        exit(1);  // 内存分配失败，直接退出
    }
    stack->top = -1;
    stack->capacity = initialCapacity;
}

// 判断栈是否为空
bool is_Empty(Stack* stack) 
{
    return stack->top == -1;
}

// 扩展栈容量
void resizeStack(Stack* stack) 
{
    stack->capacity *= 2;
    stack->data = (char*)realloc(stack->data, stack->capacity * sizeof(char));
    if (stack->data == NULL) 
    {
        printf("内存扩展失败！\n");
        exit(1);  // 内存分配失败，直接退出
    }
}

// 入栈
void push(Stack* stack, char c) 
{
    if (stack->top == stack->capacity - 1) 
    {
        resizeStack(stack);  // 扩展栈容量
    }
    stack->data[++stack->top] = c;
}

// 出栈
char pop(Stack* stack) 
{
    if (!is_Empty(stack)) 
    {
        return stack->data[stack->top--];
    }
    return '\0';  // 返回空字符
}

// 检查字符串中是否包含非法字符
bool has_Illegal_Characters(const char* s) 
{
    for (int i = 0; s[i] != '\0'; i++) 
    {
        char ch = s[i];
        if (ch != '(' && ch != ')' && ch != '{' && ch != '}' && ch != '[' && ch != ']') 
        {
            return true;  // 如果遇到非法字符
        }
    }
    return false;
}

// 判断括号是否匹配
bool is_Valid(const char* s) 
{
    Stack stack;
    initStack(&stack, INITIAL_STACK_SIZE);  // 初始化栈

    // 遍历字符串
    for (int i = 0; s[i] != '\0'; i++) 
    {
        char ch = s[i];

        // 如果是左括号，则入栈
        if (ch == '(' || ch == '{' || ch == '[') 
        {
            push(&stack, ch);
        }
        // 如果是右括号，检查栈顶是否有对应的左括号
        else if (ch == ')' || ch == '}' || ch == ']') 
        {
            if (is_Empty(&stack)) 
            {
                free(stack.data);  // 释放内存
                return false;  // 如果栈空了，说明没有左括号与之匹配
            }
            char top = pop(&stack);
            // 检查是否是匹配的括号
            if ((ch == ')' && top != '(') || (ch == '}' && top != '{') || (ch == ']' && top != '[')) 
            {
                free(stack.data);  // 释放内存
                return false;
            }
        }
    }

    // 最终栈是否为空，若为空则说明所有的括号都匹配了
    bool result = is_Empty(&stack);
    free(stack.data);  // 释放内存
    return result;
}

int main() 
{
    char s[1000000];  // 用于存储用户输入的字符串，支持较大的输入(已经是最大的了)

    // 提示用户输入字符串
    printf("请输入一个只包含英文括号的字符串（'('，')'，'{'，'}'，'['，']'）：\n");
    fgets(s, sizeof(s), stdin);  // 读取输入字符串

    // 去掉输入字符串的换行符
    s[strcspn(s, "\n")] = '\0';

    // 输入检查：确保字符串不为空
    if (strlen(s) == 0) 
    {
        printf("错误：输入为空！请输入包含括号的字符串！\n");
        return 1;
    }

    // 检查是否包含非法字符
    if (has_Illegal_Characters(s)) 
    {
        printf("错误：输入包含非法字符！请输入仅包含括号的字符串！\n");
        return 1;
    }

    // 判断括号是否匹配并输出结果
    if (is_Valid(s)) 
    {
        printf("该字符串的括号是匹配的。\n");
    }
    else 
    {
        printf("错误：该字符串的括号不匹配！\n");
    }

    return 0;
}
