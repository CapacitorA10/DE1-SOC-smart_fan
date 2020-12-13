
void gpio_on(int pin);
void gpio_off(int pin);
void divide_n_print(int input);
float dist_read(int pin);
void dc_mtr_on(int pin1, int pin2);
//void fan_dcmft(float discom, int pin2, int pin1, int clockwise);
void fan_servo_on(int pin, int loc);
void sensor_servo_on(int pin);
int search(int servo_pin, int dist_pin);
int analog_read(int pin);
void led_dcmft(int dcmft);
void power_check_ledr(int power);
void fan_pwm(int pin1, int pin2, int power);
