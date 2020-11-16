#include <stdio.h>
#include "address_map_nios2.h"
#include "nios2_ctrl_reg_macros.h"


volatile int *ledr_ptr = (int*) LEDR_BASE;
volatile int *timer_ptr = (int*) TIMER_BASE;
volatile int *gpio1_ptr = (int*) JP1_BASE;
volatile int *adc1_ptr = (int*) ADC_BASE;
volatile int *hex_5_4_ptr = (int*) HEX5_HEX4_BASE;
volatile int *hex_3_0_ptr = (int*) HEX3_HEX0_BASE;
volatile int *sliding_sw = (int*) SW_BASE;

unsigned int hex_nums[] = {0b00111111, 0b00000110, 0b01011011, 0b01001111, 0b01100110, 
                            0b01101101, 0b01111101, 0b00000111, 0b01111111, 0b01100111};


int run = 1;
int signal_10khz = 0;
int signal_1khz = 0;
int signal_500hz = 0;
int signal_50hz = 0;
int signal_0_dot_1hz = 0;
int signal_1hz = 0;

void gpio_on(int pin){
    *gpio1_ptr |= 1 << pin; //pin에 해당하는 레지스터에 1을 추가
}

void gpio_off(int pin){
    *gpio1_ptr &= ~(1 << pin); //pin에 해당하는 레지스터만 0으로 하여 AND
}

void divide_n_print(int input){
    unsigned int hex30 = 0, hex54 = 0;
 
    hex30 |= hex_nums[input % 10]; //hex0 (일의자릿수)
    hex30 |= hex_nums[(input % 100) / 10] << 8;
    hex30 |= hex_nums[(input % 1000) / 100] << 16;
    hex30 |= hex_nums[(input % 10000) / 1000] << 24; //hex3 (천의자릿수) 
    *hex_3_0_ptr = hex30;

    hex54 |= hex_nums[(input % 100000) / 10000]; //hex4 (만의자릿수)
    hex54 |= hex_nums[(input % 1000000) / 100000] << 8; //hex5 (십만 자릿수)
    *hex_5_4_ptr = hex54;
}

void pwm(int usec, int pin){
    static int prev_signal = 0, counter_50us = 0;
    int crnt_signal = signal_10khz;
    int period = usec / 50;

    if(prev_signal != crnt_signal) { //둘이 signal이 달라지는 시점 = edge
        prev_signal = crnt_signal; // 달라져 버리면 그 직후 prev에 현재 signal 저장
        counter_50us++; //10khz클럭이 변화(0->1 | 1->0) 할 때마다 counter가 증가
    }

    if(counter_50us < period) gpio_on(pin);
    else gpio_off(pin);

    if(counter_50us >= 400) counter_50us = 0; //20ms마다 초기화
}


void dc_mtr_on(int pin1, int pin2){ //해당 핀에 500hz로 입력을 넣음
    *gpio1_ptr |= (1 << pin1) & (signal_500hz * 0xffff);
    gpio_off(pin2);
}

int main(void)
{
    float temp;
    int prev_1hz = 0;
    // 초기화
    *ledr_ptr = 0;
    *hex_5_4_ptr = 0;
    *hex_3_0_ptr = 0;
    *gpio1_ptr = 0xffffffff;
    *gpio1_ptr = 0; //gpio 초기화

    // 인터럽트 설정
    *(timer_ptr + 3) = 0;
    *(timer_ptr + 2) = 5000; // 0.05ms초로 타이머 세팅 -> (1/0이 호출때마다 번갈아나옴)T = 100us -> 10kHz
    *(timer_ptr + 1) = 0b111; // 타이머 start/ITO
    NIOS2_WRITE_IENABLE(0b11);
    NIOS2_WRITE_STATUS(1);  // 인터럽트 활성화


    *(gpio1_ptr + 1) = 0xffffffff; //모두 output단자로 설정
    while(1){
        //첫번째 스위치는 led on/off
        if((*sliding_sw) & 0b1) gpio_on(0);
        else gpio_off(0);

        //두번째 스위치는 DC모터 on/off
        if((*sliding_sw) & 0b10) dc_mtr_on(2, 1);
        else gpio_off(2);

        //세번째 스위치는 서보모터 on/off
        if((*sliding_sw) & 0b100){
            if(signal_1hz) pwm(1000, 3);
            else pwm(2000, 3);
        }
        else gpio_off(3);

        //마지막으로 아날로그 온도계 읽기
        if(signal_1hz != prev_1hz) {
            prev_1hz = signal_1hz;
            *adc1_ptr = 0;
            temp = (((float) *adc1_ptr) / 37.24) * 100; //실제 온도*100로 변환
            divide_n_print((int)temp);
        }
    }
}