#include <stdio.h>
#include "address_map_nios2.h"

extern volatile int *gpio1_ptr;//= (int*) JP1_BASE;
extern volatile int *timer_ptr;
extern volatile int signal_10khz;
extern volatile int signal_1khz;
extern volatile int signal_500hz;
extern volatile int signal_50hz;
extern volatile int signal_1hz;
extern volatile int signal_0_dot_1hz;
/*
void delayMicroseconds(int t){
    t *= 100; // 100Mhz에 맞게, 100을 곱하면 1us달성
    unsigned int t_high = t >> 16;
    unsigned int t_low = t & 0xffff;
    *(timer_ptr) = *timer_ptr; // 타이머 초기화
    *(timer_ptr + 5) = t_high;
    *(timer_ptr + 4) = t_low;
    *(timer_ptr + 1) = 0b100; // 타이머 start
}
*/
void timer_ISR(){
    static int counter = 0;
    *timer_ptr = 0b11; //상태레지스터 초기화
    // 1회 호출될때마다 카운터는 1씩 증가
    counter += 1;
    //10khz 신호
    if(signal_10khz) signal_10khz = 0;
    else if (!signal_10khz) signal_10khz = 1;

    //1khz 신호
    if((counter % 10)==0){
        if(signal_1khz) signal_1khz = 0;
        else if (!signal_1khz) signal_1khz = 1;
    }

    //500hz 신호
    if((counter % 20)==0){
        if(signal_500hz) signal_500hz = 0;
        else if (!signal_500hz) signal_500hz = 1;
    }

    //50hz 신호
    if((counter % 200)==0){
        if(signal_50hz) signal_50hz = 0;
        else if (!signal_50hz) signal_50hz = 1;
    }

    //1hz 신호
    if((counter % 10000)==0){
        if(signal_1hz) signal_1hz = 0;
        else if (!signal_1hz) signal_1hz = 1;
    }

    //0.1hz 신호
    if((counter % 100000)==0){
        if(signal_0_dot_1hz) signal_0_dot_1hz = 0;
        else if (!signal_0_dot_1hz) signal_0_dot_1hz = 1;
    }
    if(counter >= 2000000000) counter = 0;
}