#include "motor.h"
#include "driver/ledc.h"
#include "driver/gpio.h"
#include <math.h>
//---------------- Motor Functions ----------------

// Initialize PWM for motor and configure direction GPIOs
void pwm_init(void)
{
    // Configure timer for motor PWM on LEDC_TIMER_0 using low-speed mode
    ledc_timer_config_t ledc_timer = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .duty_resolution = LEDC_RES,
        .timer_num = LEDC_TIMER_0,
        .freq_hz = LEDC_FREQ,
        .clk_cfg = LEDC_AUTO_CLK};
    ledc_timer_config(&ledc_timer);

    // Configure PWM channel for motor on LEDC_CHANNEL_0
    ledc_channel_config_t ledc_channel = {
        .gpio_num = PWM_GPIO,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = LEDC_CHANNEL_0,
        .timer_sel = LEDC_TIMER_0,
        .duty = 0,
        .hpoint = 0};
    ledc_channel_config(&ledc_channel);

    // Set motor direction pins as outputs
    gpio_set_direction(RPWM_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_direction(LPWM_GPIO, GPIO_MODE_OUTPUT);
}

// Drive motor forward with specified PWM duty cycle
void motor_forward(uint32_t duty)
{
    gpio_set_level(RPWM_GPIO, 1);
    gpio_set_level(LPWM_GPIO, 0);
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, duty);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
}

// Drive motor backward with specified PWM duty cycle
void motor_backward(uint32_t duty)
{
    gpio_set_level(RPWM_GPIO, 0);
    gpio_set_level(LPWM_GPIO, 1);
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, duty);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
}

// Stop the motor
void motor_stop()
{
    gpio_set_level(RPWM_GPIO, 0);
    gpio_set_level(LPWM_GPIO, 0);
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 0);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
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

// Hàm tính tốc độ dựa trên giá trị j1y và tốc độ tối đa
float calculate_speed(int j1y, int speed_max)
{
    return (speed_max * abs(j1y)) / (MAX_AXIS_VALUE * 1.0);
}

// Hàm tính góc quay dựa trên giá trị j1x và j1y (giới hạn trong phạm vi MAX_ANGLE)
float calculate_angle(int j1x, int j1y)
{
    float angle = atan2(j1x, j1y) * (180 / 3.1415926535);
    angle = -angle; // Đảo góc quay
    if (angle >= MAX_ANGLE)
    {
        angle += MAX_ANGLE;
    }
    else if (angle <= -MAX_ANGLE)
    {
        angle -= MAX_ANGLE;
    }
    return angle;
}

// hàm chuẩn hoá lại góc quay
float normalize_angle(int angle)
{
    // Tính giá trị đã điều chỉnh, bắt đầu từ 0 sau khi loại bỏ 25 độ
    float adjusted = fmax(fabs(angle) - 25, 0);
    // Giới hạn giá trị tối đa là 40 độ
    adjusted = fmin(adjusted, 60);
    // Gán lại dấu theo góc ban đầu
    return copysign(adjusted, angle);
}

// Mảng hàm điều khiển motor với thứ tự: {lùi, dừng, tiến}
void (*motor_functions[3])(uint32_t) = {motor_backward, motor_stop, motor_forward};

// Hàm điều khiển motor dựa trên giá trị j1y và tính PWM tương ứng
void motor_control(int j1y, uint32_t duty)
{
    float speed = calculate_speed(j1y, SPEED_MAX);
    uint32_t pwm_duty = (int)(speed * 1023 / MAX_AXIS_VALUE);
    int direction = (j1y > 0) - (j1y < 0);
    motor_functions[direction + 1](pwm_duty);
}

// Set servo angle between 0° and 180°
// For a 50Hz signal, a 20ms period:
//   - 1ms pulse (≈ 5% duty) corresponds to 0°
//   - 2ms pulse (≈ 10% duty) corresponds to 180°
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
