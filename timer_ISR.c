#include <stdio.h>
#include "address_map_nios2.h"

extern volatile int *timer_ptr;
extern volatile int signal_10khz;
extern volatile int signal_1hz;
extern volatile int signal_0_dot_1hz;
extern volatile int *gpio1_ptr;

extern volatile int pwm[6]; //1000us부터 2000us까지 200us단위
extern volatile int sensor_servo[2], fan_servo[2];

void gpio_on2(int pin){
    *gpio1_ptr |= 1 << pin; //pin에 해당하는 레지스터에 1을 추가
}

void gpio_off2(int pin){
    *gpio1_ptr &= ~(1 << pin); //pin에 해당하는 레지스터만 0으로 하여 AND
}
void write_servo(int pin, int dgr){
    switch(dgr){
        case 0 :
            if(pwm[0]) gpio_on2(pin);
            else  gpio_off2(pin);
            break;
        case 1 :
            if(pwm[1]) gpio_on2(pin);
            else  gpio_off2(pin);
            break;
        case 2 :
            if(pwm[2]) gpio_on2(pin);
            else  gpio_off2(pin);
            break;
        case 3 :
            if(pwm[3]) gpio_on2(pin);
            else  gpio_off2(pin);
            break;
        case 4 :
            if(pwm[4]) gpio_on2(pin);
            else  gpio_off2(pin);
            break;
        case 5 :
            if(pwm[5]) gpio_on2(pin);
            else  gpio_off2(pin);
            break;
     
    }
}

void timer_ISR(){
    static int counter = 0;
    *timer_ptr = 0b11; //상태레지스터 초기화
    // 1회 호출될때마다 카운터는 1씩 증가
    counter++; //카운터는 20khz로 1씩 증가하게 됨

    //10khz 신호
    if(signal_10khz) signal_10khz = 0;
    else if (!signal_10khz) signal_10khz = 1;

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

    for(int i = 0 ; i < 6 ; i++){
        if( (counter % 100) < ((4 * i) + 20) ) pwm[i] = 1;    // 100으로 쪼갬 -> 0.05ms 해상도
        else pwm[i] = 0;                                      // 20이라면 1000us만 1이고 9ms는 0
    }                                                         // 24이라면 1200us 만 1이고 8.8ms 0
                                                              // 40이라면 2000us 1, 8ms 0....
    write_servo(sensor_servo[0], sensor_servo[1]);
    write_servo(fan_servo[0], fan_servo[1]);
    if(counter >= 2000000000) counter = 0;
}
