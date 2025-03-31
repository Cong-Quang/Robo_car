#include "motor.h"
#include "driver/ledc.h"
#include "driver/gpio.h"
#include <math.h>
//---------------- Motor Functions ----------------

// Initialize PWM for motor and configure direction GPIOs
void pwm_init(void)
{
    ledc_timer_config_t ledc_timer = {
        .speed_mode = LEDC_HIGH_SPEED_MODE,
        .duty_resolution = LEDC_RES,
        .timer_num = LEDC_TIMER_0,
        .freq_hz = LEDC_FREQ,
        .clk_cfg = LEDC_AUTO_CLK};
    ledc_timer_config(&ledc_timer);

    ledc_channel_config_t ledc_channel = {
        .gpio_num = PWM_GPIO,
        .speed_mode = LEDC_HIGH_SPEED_MODE,
        .channel = LEDC_CHANNEL_0,
        .timer_sel = LEDC_TIMER_0,
        .duty = 0,
        .hpoint = 0};
    ledc_channel_config(&ledc_channel);

    gpio_set_direction(RPWM_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_direction(LPWM_GPIO, GPIO_MODE_OUTPUT);
}
//---------------- Servo Functions ----------------

// Initialize PWM for servo using SERVO_GPIO and the defined servo timer/channel
void servo_init(void)
{
    // Configure timer for servo PWM on SERVO_LEDC_TIMER using low-speed mode
    ledc_timer_config_t ledc_timer = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .duty_resolution = LEDC_RES,
        .timer_num = SERVO_LEDC_TIMER,
        .freq_hz = SERVO_LEDC_FREQ,
        .clk_cfg = LEDC_AUTO_CLK};
    ledc_timer_config(&ledc_timer);

    // Configure PWM channel for servo on SERVO_LEDC_CHANNEL and SERVO_GPIO
    ledc_channel_config_t ledc_channel = {
        .gpio_num = SERVO_GPIO,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = SERVO_LEDC_CHANNEL,
        .timer_sel = SERVO_LEDC_TIMER,
        .duty = 0,
        .hpoint = 0};
    ledc_channel_config(&ledc_channel);
}
// Hàm điều khiển xe chạy tiến
void motor_forward(uint32_t duty) {
    gpio_set_level(RPWM_GPIO, 1);
    gpio_set_level(LPWM_GPIO, 0);
    ledc_set_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0, duty);
    ledc_update_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0);
}

// Hàm điều khiển xe chạy lùi
void motor_backward(uint32_t duty) {
    gpio_set_level(RPWM_GPIO, 0);
    gpio_set_level(LPWM_GPIO, 1);
    ledc_set_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0, duty);
    ledc_update_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0);
}

// Hàm dừng xe
void motor_stop() {
    gpio_set_level(RPWM_GPIO, 0);
    gpio_set_level(LPWM_GPIO, 0);
    ledc_set_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0, 0);
    ledc_update_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0);
}

// Hàm tính tốc độ dựa trên tọa độ y
// float calculate_speed(int j1y, int speed_max)
// {
//     return (speed_max * abs(j1y)) / (MAX_AXIS_VALUE * 1.0);
// }

// Hàm tính góc quay dựa trên giá trị j1x và j1y (giới hạn trong phạm vi MAX_ANGLE)
float calculate_angle(int j1x, int j1y)
{
    // Tính góc quay từ tọa độ x và y
    float angle = atan2(j1x, j1y) * (180 / 3.1415926535);
    angle = -angle;
    if (angle >= MAX_ANGLE)
    {
        angle -= MAX_ANGLE;
    }
    else if (angle <= -MAX_ANGLE)
    {
        angle += MAX_ANGLE;
    }
    return angle;
}


//hàm chuẩn hoá lại góc quay

float normalize_angle(int angle) {
    // Tính giá trị đã điều chỉnh, bắt đầu từ 0 sau khi loại bỏ 25 độ
    float adjusted = fmax(fabs(angle) - 20, 0);
    // Giới hạn giá trị tối đa là 60 độ
    adjusted = fmin(adjusted, MAX_ANGLE_REAL);
    // Gán lại dấu theo góc ban đầu
    return copysign(adjusted, angle);
}

// Bảng hàm điều khiển động cơ
void (*motor_functions[3])(uint32_t) = {motor_backward, motor_stop, motor_forward};

// Hàm điều khiển motor theo tốc độ và góc quay mà không sử dụng if-else
void motor_control(int j1y, uint32_t duty)
{
    uint32_t pwm_duty = (int)(abs(j1y) * SPEED_MAX / MAX_AXIS_VALUE);

    int direction = (j1y > 0) - (j1y < 0);

    motor_functions[direction + 1](pwm_duty);
}  

void servo_set_angle(uint32_t angle)
{
    // Clamp angle to maximum 180°
    if (angle > 180)
        angle = 180;

    // Calculate duty values based on LEDC_RES (e.g., 10-bit: max 1023)
    uint32_t duty_min = 51;  // Approximate duty for 1ms pulse (5% of 1023)
    uint32_t duty_max = 102; // Approximate duty for 2ms pulse (10% of 1023)
    uint32_t duty = duty_min + ((duty_max - duty_min) * angle) / 180;

    ledc_set_duty(LEDC_LOW_SPEED_MODE, SERVO_LEDC_CHANNEL, duty);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, SERVO_LEDC_CHANNEL);
}
