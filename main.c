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


int signal_10khz = 0;
int signal_1hz = 0;
int signal_0_dot_1hz = 0;

int counter_1hz = 0;
int pwm[6] = {0,};
int sensor_servo[2] = {0,}, 
    fan_servo[2] = {0,};

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

float dist_read(int pin){
    *(adc1_ptr + pin) = 0;
    float volt = (((float) *(adc1_ptr + pin)) * 0.0048828125) * 0.25;
    return (65 * (1/volt));
}

void dc_mtr_on(int pin1, int pin2){ //해당 핀에 500hz로 입력을 넣음
    *gpio1_ptr |= (1 << pin1) & (signal_10khz * 0xffff);
    gpio_off(pin2);
}

void fan_servo_on(int pin, int loc){
    fan_servo[0] = pin;
    fan_servo[1] = loc;
}

void sensor_servo_on(int pin){
    sensor_servo[0] = pin;
    sensor_servo[1] = counter_1hz % 6;
}

int search(int servo_pin, int dist_pin){
    static int returnVal = 0, prev_0_dot_1hz = 0;
    static float distances[6] = {0,};
    int curr_loc = counter_1hz % 6;
    
    sensor_servo_on(servo_pin);
    // ---현재 위치에서 거리 측정하여 배열에 항상 저장---
    distances[curr_loc] = dist_read(dist_pin);

    //저장된 배열 중 최근접 점만 빼냄
    //해당 건은 5초에 한번만 시행됨
    int min = 99999999;
    if(prev_0_dot_1hz != signal_0_dot_1hz){
        prev_0_dot_1hz = signal_0_dot_1hz;
        for(int i = 0 ; i < 6; i++){
            if(min > distances[i]){
                min = distances[i];
                returnVal = i;
            }
        }
    }
    *(ledr_ptr) = prev_0_dot_1hz;
    return returnVal;
}



int main(void)
{
    float temp, temp2;
    int prev_1hz = 0;
    int counter_prev_1hz = 0;
    int nearist_loc = 0;
    // 초기화
    *ledr_ptr = 0;
    *hex_5_4_ptr = 0;
    *hex_3_0_ptr = 0;
    *gpio1_ptr = 0xffffffff;
    *gpio1_ptr = 0; //gpio 초기화

    // 인터럽트 설정
    *(timer_ptr + 3) = 0;
    *(timer_ptr + 2) = 5000; // 0.5ms초로 타이머 세팅 -> (1/0이 호출때마다 번갈아나옴)T = 1000us -> 1kHz
    *(timer_ptr + 1) = 0b111; // 타이머 start/ITO
    NIOS2_WRITE_IENABLE(0x01); //타이머 IRQ 활성화
    NIOS2_WRITE_STATUS(1);  // 인터럽트 활성화


    *(gpio1_ptr + 1) = 0xffffffff; //모두 output단자로 설정
    while(1){
        //1초마다 1씩 증가하는 카운터 구현
        if(counter_prev_1hz != signal_1hz){
            counter_prev_1hz = signal_1hz;
            if(signal_1hz) counter_1hz++; 
        } if(counter_1hz > 100000) counter_1hz = 0; //overflow방지

        //첫번째 스위치는 led on/off
        if((*sliding_sw) & 0b1) gpio_on(0);
        else gpio_off(0);

        //두번째 스위치는 DC모터 on/off
        if((*sliding_sw) & 0b10) dc_mtr_on(2, 3);
        else { gpio_off(2); gpio_off(3);} //모터는 방향제어로 인한 핀 2개 소모

        //적외선거리센서의 서보모터 on/off 및 최근접 점 탐색
        if((*sliding_sw) & 0b100) nearist_loc = search(5, 0); //서보모터는 5번 gpio, 센서는 0번

        //모터의 서보모터 on/off 및 최근접 점 탐색
        if((*sliding_sw) & 0b1000) fan_servo_on(7, nearist_loc); // 서보모터는 7번 gpio

        //마지막으로 아날로그 읽기
        if((*sliding_sw) & 0b10000){
            if(signal_1hz != prev_1hz) {
                prev_1hz = signal_1hz;
                *adc1_ptr = 0;
                temp = (((float) *adc1_ptr) * 0.0048828125) * 0.25; //실제 온도*100로 변환
                temp2 = 65 * (1/temp);
                divide_n_print((int)temp2);
            }
        }
        else{
            if(signal_1hz != prev_1hz) {
                prev_1hz = signal_1hz;
                *(adc1_ptr+1) = 0;
                temp = ((float) *(adc1_ptr+1) / 4096) * 10000; //실제 습도*100로 변환
                divide_n_print((int)temp);
            }

        }

    }
}
