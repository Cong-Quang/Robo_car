    #ifndef MOTOR_H
    #define MOTOR_H

    #include <stdint.h>

    #ifdef __cplusplus
    extern "C" {
    #endif

    // Motor configuration
    #define PWM_GPIO 5
    #define RPWM_GPIO 18
    #define LPWM_GPIO 19

    #define LEDC_FREQ 1000
    #define LEDC_RES LEDC_TIMER_10_BIT

    // Servo configuration
    #define SERVO_GPIO 13
    #define SERVO_LEDC_FREQ 50
    #define SERVO_LEDC_TIMER LEDC_TIMER_1
    #define SERVO_LEDC_CHANNEL LEDC_CHANNEL_1

    #define MAX_AXIS_VALUE 200
    #define SPEED_MAX 256
    #define MAX_ANGLE 90
    #define MAX_ANGLE_REAL 40

    // Function prototypes
    void pwm_init(void);
    void motor_forward(uint32_t duty);
    void motor_backward(uint32_t duty);
    void motor_stop();

    void servo_init(void);
    void servo_set_angle(uint32_t angle);
    void motor_control(int j1y, uint32_t duty);
    float calculate_speed(int j1y, int speed_max);
    float calculate_angle(int j1x, int j1y);
    float normalize_angle(int angle);

    #ifdef __cplusplus
    }
    #endif

    #endif // MOTOR_H
