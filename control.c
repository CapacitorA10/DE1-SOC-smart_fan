#include "address_map_nios2.h"

extern unsigned int hex_nums[];
extern volatile int *ledr_ptr;
extern volatile int *timer_ptr;
extern volatile int *gpio1_ptr;
extern volatile int *adc1_ptr;
extern volatile int *hex_5_4_ptr;
extern volatile int *hex_3_0_ptr;
extern volatile int *sliding_sw;
extern volatile int *key_ptr;

extern int signal_10khz;
extern int signal_10hz;
extern int signal_1hz;
extern int signal_0_dot_1hz;

extern int counter_1hz;
extern int pwm[6];
extern int sensor_servo[2], 
            fan_servo[2];


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
/*
void fan_dcmft(float discom, int pin2, int pin1, int clockwise){
    int pwm_power = (int)discom;
    if(clockwise == 1){
        fan_dc[0] = pin1;
        fan_dc[1] = pwm_power;
        gpio_off(pin2);
    }
    else{
        fan_dc[0] = pin2;
        fan_dc[1] = pwm_power;
        gpio_off(pin1);
    }
}
*/

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
    return returnVal;
}

int analog_read(int pin){
    *(adc1_ptr + pin) = 0; //읽기 위해 0을 써주어야 함
    return *(adc1_ptr + pin);
}

void led_dcmft(int dcmft){
    if( (dcmft > 50) && (dcmft < 80) ){
        gpio_on(9);
        gpio_off(11);
        gpio_off(13);
    }
    else if( (dcmft >= 80) && (dcmft < 90) ){
        gpio_on(9);
        gpio_on(11);
        gpio_off(13);
    }
    else if( (dcmft >= 90) ){
        gpio_on(9);
        gpio_on(11);
        gpio_on(13);
    }
    else{
        gpio_off(9);
        gpio_off(11);
        gpio_off(13);
    }
    
}

void power_check_ledr(int power){ //power에 따른 LEDR표기
    unsigned int k = 1023; //10칸으로 표기
    *ledr_ptr = k >> power; 
}

void fan_pwm(int pin1, int pin2, int power){
    static int i = 0;
    static int prev = 0;
    gpio_off(pin2);
    if (power < 0) power = 0; //0이하면 0으로 조정
    power = 10 - power;
    power_check_ledr(power);
    /* power는 0에서 9까지의 값을 가짐(이정도도 너무 쎄다) */
    /* 1이라면 9%의 PWM을 가지게 된다. (이건 거의 초미풍) */
    if( signal_10khz != prev ){
        prev = signal_10khz;
        i++;
        if( i > power ){ 
            gpio_on(pin1);
            i = 0;
        }
        else{
            gpio_off(pin1);
        }
    }
    
}





