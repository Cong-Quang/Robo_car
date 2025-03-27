#ifndef OLED_H
#define OLED_H

#include "driver/i2c.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"
#include <stdarg.h>
#include "qrcode.h"

// Cấu hình I2C & OLED
#define I2C_MASTER_NUM           I2C_NUM_0
#define I2C_MASTER_SCL_IO        22
#define I2C_MASTER_SDA_IO        21
#define I2C_MASTER_FREQ_HZ       400000
#define OLED_ADDR                0x3C

#define OLED_WIDTH               128
#define OLED_HEIGHT              64
#define OLED_BUFFER_SIZE         (OLED_WIDTH * OLED_HEIGHT / 8)

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Khởi tạo giao tiếp I2C và OLED.
 *
 * @return esp_err_t kết quả khởi tạo.
 */
esp_err_t oled_init(void);

/**
 * @brief Xóa bộ đệm hiển thị.
 */
void oled_clear(void);

/**
 * @brief Cập nhật nội dung bộ đệm lên màn hình OLED.
 *
 * @return esp_err_t kết quả cập nhật.
 */
esp_err_t oled_display(void);

/**
 * @brief Vẽ một chuỗi ký tự tại vị trí (x, page) trên bộ đệm.
 *
 * @param x Vị trí cột bắt đầu (0 -> OLED_WIDTH-1).
 * @param page Vị trí trang (mỗi trang có 8 pixel cao).
 * @param str Chuỗi cần hiển thị.
 */
void oled_draw_string(uint8_t x, uint8_t page, const char *str);

/**
 * @brief Hàm in thông tin định dạng (printf-like) lên OLED.
 *
 * @param x Vị trí cột bắt đầu.
 * @param page Vị trí trang.
 * @param format Chuỗi định dạng.
 * @param ... Tham số cho chuỗi định dạng.
 */
void oled_print(uint8_t x, uint8_t page, const char *format, ...);

void generate_qr_code();
#ifdef __cplusplus
}
#endif

#endif // OLED_H
