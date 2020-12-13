#include <stdio.h>

extern volatile int *key_ptr;
extern volatile int priority_counter;
void pushbutton_ISR()
{
    int press = *(key_ptr + 3); // 엣지캡쳐
    *(key_ptr + 3) = press; // 엣지캡쳐 초기화
    priority_counter = 5; // 5초 카운터 시작
    printf("\nISR detected\n");
}