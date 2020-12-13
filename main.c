#include <stdio.h>
#include "address_map_nios2.h"
#include "nios2_ctrl_reg_macros.h"
#include "control.h"


volatile int *ledr_ptr = (int*) LEDR_BASE;
volatile int *timer_ptr = (int*) TIMER_BASE;
volatile int *gpio1_ptr = (int*) JP1_BASE;
volatile int *adc1_ptr = (int*) ADC_BASE;
volatile int *hex_5_4_ptr = (int*) HEX5_HEX4_BASE;
volatile int *hex_3_0_ptr = (int*) HEX3_HEX0_BASE;
volatile int *sliding_sw = (int*) SW_BASE;
volatile int *key_ptr = (int*) KEY_BASE;

unsigned int hex_nums[] = {0b00111111, 0b00000110, 0b01011011, 0b01001111, 0b01100110, 
                            0b01101101, 0b01111101, 0b00000111, 0b01111111, 0b01100111};


int signal_10khz = 0;
int signal_10hz = 0;
int signal_1hz = 0;
int signal_0_dot_1hz = 0;

int counter_1hz = 0;
int pwm[6] = {0,};
int sensor_servo[2] = {0,}, 
    fan_servo[2] = {0,};

int priority_counter = 0;


int main(void)
{
    float temperature=0, humidity=0, discomfort = 0, discomfort_mean=0;
    int prev_1hz = 0, prev_10hz = 0;
    int counter_prev_1hz = 0;
    int nearist_loc = 0;
    int timer = 0, hour, min, sec, time_print;
    int accum_var = 0;
    int grow_up = 0;
    
    // 초기화
    *ledr_ptr = 0;
    *hex_5_4_ptr = 0;
    *hex_3_0_ptr = 0;
    *gpio1_ptr = 0xffffffff; //모두 output으로
    *gpio1_ptr = 0; //gpio 초기화

    // 인터럽트 설정
    *(key_ptr + 2) = 0b1000; //4번째 키를 인터럽트로 설정
    *(timer_ptr + 3) = 0;
    *(timer_ptr + 2) = 5000; // 0.5ms초로 타이머 세팅 -> (1/0이 호출때마다 번갈아나옴)T = 1000us -> 1kHz
    *(timer_ptr + 1) = 0b111; // 타이머 start/ITO
    NIOS2_WRITE_IENABLE(0b11); //타이머,키 IRQ 활성화
    NIOS2_WRITE_STATUS(1);  // 인터럽트 활성화
    *(gpio1_ptr + 1) = 0xffffffff; //모두 output단자로 설정
    while(1){
        // 1초마다 1씩 증가하는 카운터 구현
        if(counter_prev_1hz != signal_1hz){
            counter_prev_1hz = signal_1hz;
            if(signal_1hz) {
                counter_1hz++; 
                if(priority_counter > 0) priority_counter--; //priority counter를 1초마다 1씩 감소시켜, 0이 되면 다시 타이머 혹은 사라지게
            }
        } if(counter_1hz > 100000) counter_1hz = 0; //overflow방지


        ///////////////////////////////////////////////////////////////
        //////////////////////////////동작부///////////////////////////
        //////////////////////////////////////////////////////////////
        /* -------스위치가 내려가 있으면 무조건 시행, 스위치가 올라가 있으면 타이머가 1보다 커야지만 실행------- */

        if( !((*sliding_sw) & 1) || ( ((*sliding_sw) & 1) && (timer > 0) )) { 

            // 아날로그 읽기
            temperature = ((float) analog_read(2)) * 180 / 4096; //실제 온도로 변환(섭씨)
            humidity = ((float) analog_read(4) / 4096) * 150 / 100; //실제 습도로 변환(1~0)

            // 불쾌지수 계산
            discomfort += (1.8 * temperature) - 0.55*(1 - humidity) * (1.8 * temperature - 26) + 32;
            accum_var += 1;
            // 불쾌지수를 100번 누적하여 평균 낸 것으로 계산하여 출력
            if(accum_var == 1000) {
                discomfort_mean = discomfort / 1000;
                led_dcmft(discomfort_mean); //9,11,13
                //divide_n_print((int)discomfort_mean);
                accum_var = 0;
                discomfort = 0;
            }
            // 두번째 스위치 - DC모터 on/off
            if((*sliding_sw) & 0b10) {
                fan_pwm(2,3, (int)((((discomfort_mean - 66) * 3.3))/10) ); //66 이하일 때에는 파워가 0, 70이상부터 파워가 리니어하게 10까지 올라감.
            }
            else {
                gpio_off(2); gpio_off(3); 
                *ledr_ptr = 0;
                } 

            // 서보모터 on/off 및 최근접 점 탐색하여 조준
            if((*sliding_sw) & 0b100) {
                nearist_loc = search(5, 0); //검색용 서보모터는 5번 gpio, 센서는 0번
                fan_servo_on(7, nearist_loc); //선풍기용 서보모터는 7번
            }

            
          
        }
        else { // 타이머가 다 됐으므로 필요없는거 끈다
            
            *ledr_ptr = 0;
        }



        //////////////////////////////////////////////////////////////
        ///////////////// 자동 꺼짐 타이머 동작부 /////////////////////
        /////////////////////////////////////////////////////////////

        if( (*sliding_sw) & 1 ) {
            if(*key_ptr & 1){
                if(signal_10hz != prev_10hz){
                    prev_10hz = signal_10hz;
                    // 0번 key를 누르고 있는 동안 타이머를 증가하는데, 오래 누를수록 그 양이 증가
                    timer += grow_up;
                    grow_up++;
                }
                
            }
            else if(*key_ptr & 0b10) timer = 0; // 1번 키를 누르면 리셋


            else{ // key를 안누르면 타이머를 초당 1씩 감소
                if(signal_1hz != prev_1hz){
                    prev_1hz = signal_1hz;
                    if(signal_1hz)
                        if(timer > 0) timer--; 
                }
                grow_up = 0;
            }
            hour = timer / 3600;
            min = (timer % 3600) / 60;
            sec = (timer % 60);
            time_print = (10000 * hour) + (100 * min) + sec;

            
            // priority_counter가 선점하고 있지 않을 때에만 타이머 표시
            if(!priority_counter) divide_n_print(time_print);
            else 
            {
                divide_n_print(  (int)(discomfort_mean * 100) );
            }
        }
        else{ //정상 작동중이면 HEX를 끈다.

            // 여기도 priority_counter가 선점하고 있을 때에는 불쾌지수 표기
            if(priority_counter) divide_n_print( (int)(discomfort_mean * 100) );
            else 
            {
                *hex_5_4_ptr = 0;
                *hex_3_0_ptr = 0;
            }
            timer = 0;
            *ledr_ptr = 0;

        }
    }
}
