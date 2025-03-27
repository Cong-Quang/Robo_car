#include "oled.h"
#include "esp_log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "qrcode.h"

static const char *TAG = "OLED";

// Bộ đệm hiển thị
static uint8_t oled_buffer[OLED_BUFFER_SIZE];

/*=================== I2C Init ===================*/
static esp_err_t i2c_master_init(void)
{
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
    };
    esp_err_t err = i2c_param_config(I2C_MASTER_NUM, &conf);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "I2C param config failed");
        return err;
    }
    return i2c_driver_install(I2C_MASTER_NUM, conf.mode, 0, 0, 0);
}

/*=================== SSD1306 Driver ====================*/
// Gửi lệnh cho SSD1306 (prefix 0x00)
static esp_err_t ssd1306_send_command(uint8_t cmd)
{
    uint8_t data[2] = {0x00, cmd};
    return i2c_master_write_to_device(I2C_MASTER_NUM, OLED_ADDR, data, sizeof(data), 1000 / portTICK_PERIOD_MS);
}

// Gửi dữ liệu cho SSD1306 (prefix 0x40)
static esp_err_t ssd1306_send_data(uint8_t* data_ptr, size_t len)
{
    uint8_t *data = malloc(len + 1);
    if (!data) return ESP_ERR_NO_MEM;
    data[0] = 0x40; // Data mode
    memcpy(&data[1], data_ptr, len);
    esp_err_t err = i2c_master_write_to_device(I2C_MASTER_NUM, OLED_ADDR, data, len + 1, 1000 / portTICK_PERIOD_MS);
    free(data);
    return err;
}

// Hàm khởi tạo SSD1306 (theo datasheet)
static void ssd1306_init(void)
{
    vTaskDelay(100 / portTICK_PERIOD_MS);
    ssd1306_send_command(0xAE); // Display OFF
    ssd1306_send_command(0x20); // Set Memory Addressing Mode
    ssd1306_send_command(0x00); // Horizontal Addressing Mode
    ssd1306_send_command(0xB0); // Set Page Start Address cho Page Addressing Mode (0-7)
    ssd1306_send_command(0xC8); // COM Output Scan Direction (remapped)
    ssd1306_send_command(0x00); // Low Column Address
    ssd1306_send_command(0x10); // High Column Address
    ssd1306_send_command(0x40); // Start Line Address
    ssd1306_send_command(0x81); // Set Contrast Control
    ssd1306_send_command(0xFF);
    ssd1306_send_command(0xA1); // Segment Re-map
    ssd1306_send_command(0xA6); // Normal Display
    ssd1306_send_command(0xA8); // Set Multiplex Ratio
    ssd1306_send_command(0x3F);
    ssd1306_send_command(0xA4); // Output follows RAM content
    ssd1306_send_command(0xD3); // Set Display Offset
    ssd1306_send_command(0x00);
    ssd1306_send_command(0xD5); // Set Display Clock Divide Ratio/Oscillator Frequency
    ssd1306_send_command(0xF0);
    ssd1306_send_command(0xD9); // Set Pre-charge Period
    ssd1306_send_command(0x22);
    ssd1306_send_command(0xDA); // Set COM Pins Hardware Configuration
    ssd1306_send_command(0x12);
    ssd1306_send_command(0xDB); // Set VCOMH Deselect Level
    ssd1306_send_command(0x20);
    ssd1306_send_command(0x8D); // Charge Pump Setting
    ssd1306_send_command(0x14); // Enable Charge Pump
    ssd1306_send_command(0xAF); // Display ON
}

/*=================== Font 5x7 ====================*/
// Hàm trả về con trỏ đến mảng 5 byte định nghĩa một ký tự 5x7
static const uint8_t* get_font_data(char c)
{
    switch(c)
    {
        // ----------- Ký tự ASCII cơ bản -----------
        // Ký tự khoảng trắng
        case ' ': { static const uint8_t data[5] = {0x00,0x00,0x00,0x00,0x00}; return data; }
        // Chữ thường
        case 'a': { static const uint8_t data[5] = {0x20,0x54,0x54,0x54,0x78}; return data; }
        case 'b': { static const uint8_t data[5] = {0x7F,0x48,0x44,0x44,0x38}; return data; }
        case 'c': { static const uint8_t data[5] = {0x38,0x44,0x44,0x44,0x20}; return data; }
        case 'd': { static const uint8_t data[5] = {0x38,0x44,0x44,0x48,0x7F}; return data; }
        case 'e': { static const uint8_t data[5] = {0x38,0x54,0x54,0x54,0x18}; return data; }
        case 'f': { static const uint8_t data[5] = {0x08,0x7E,0x09,0x01,0x02}; return data; }
        case 'g': { static const uint8_t data[5] = {0x0C,0x52,0x52,0x52,0x3E}; return data; }
        case 'h': { static const uint8_t data[5] = {0x7F,0x08,0x04,0x04,0x78}; return data; }
        case 'i': { static const uint8_t data[5] = {0x00,0x44,0x7D,0x40,0x00}; return data; }
        case 'j': { static const uint8_t data[5] = {0x20,0x40,0x44,0x3D,0x00}; return data; }
        case 'k': { static const uint8_t data[5] = {0x7F,0x10,0x28,0x44,0x00}; return data; }
        case 'l': { static const uint8_t data[5] = {0x00,0x41,0x7F,0x40,0x00}; return data; }
        case 'm': { static const uint8_t data[5] = {0x7C,0x04,0x18,0x04,0x78}; return data; }
        case 'n': { static const uint8_t data[5] = {0x7C,0x08,0x04,0x04,0x78}; return data; }
        case 'o': { static const uint8_t data[5] = {0x38,0x44,0x44,0x44,0x38}; return data; }
        case 'p': { static const uint8_t data[5] = {0x7C,0x14,0x14,0x14,0x08}; return data; }
        case 'q': { static const uint8_t data[5] = {0x08,0x14,0x14,0x18,0x7C}; return data; }
        case 'r': { static const uint8_t data[5] = {0x7C,0x08,0x04,0x04,0x08}; return data; }
        case 's': { static const uint8_t data[5] = {0x48,0x54,0x54,0x54,0x20}; return data; }
        case 't': { static const uint8_t data[5] = {0x04,0x3F,0x44,0x40,0x20}; return data; }
        case 'u': { static const uint8_t data[5] = {0x3C,0x40,0x40,0x20,0x7C}; return data; }
        case 'v': { static const uint8_t data[5] = {0x1C,0x20,0x40,0x20,0x1C}; return data; }
        case 'w': { static const uint8_t data[5] = {0x3C,0x40,0x30,0x40,0x3C}; return data; }
        case 'x': { static const uint8_t data[5] = {0x44,0x28,0x10,0x28,0x44}; return data; }
        case 'y': { static const uint8_t data[5] = {0x0C,0x50,0x50,0x50,0x3C}; return data; }
        case 'z': { static const uint8_t data[5] = {0x44,0x64,0x54,0x4C,0x44}; return data; }
        // Chữ in hoa
        case 'A': { static const uint8_t data[5] = {0x7E,0x11,0x11,0x11,0x7E}; return data; }
        case 'B': { static const uint8_t data[5] = {0x7F,0x49,0x49,0x49,0x36}; return data; }
        case 'C': { static const uint8_t data[5] = {0x3E,0x41,0x41,0x41,0x22}; return data; }
        case 'D': { static const uint8_t data[5] = {0x7F,0x41,0x41,0x22,0x1C}; return data; }
        case 'E': { static const uint8_t data[5] = {0x7F,0x49,0x49,0x49,0x41}; return data; }
        case 'F': { static const uint8_t data[5] = {0x7F,0x09,0x09,0x09,0x01}; return data; }
        case 'G': { static const uint8_t data[5] = {0x3E,0x41,0x49,0x49,0x7A}; return data; }
        case 'H': { static const uint8_t data[5] = {0x7F,0x08,0x08,0x08,0x7F}; return data; }
        case 'I': { static const uint8_t data[5] = {0x00,0x41,0x7F,0x41,0x00}; return data; }
        case 'J': { static const uint8_t data[5] = {0x20,0x40,0x41,0x3F,0x01}; return data; }
        case 'K': { static const uint8_t data[5] = {0x7F,0x08,0x14,0x22,0x41}; return data; }
        case 'L': { static const uint8_t data[5] = {0x7F,0x40,0x40,0x40,0x40}; return data; }
        case 'M': { static const uint8_t data[5] = {0x7F,0x02,0x0C,0x02,0x7F}; return data; }
        case 'N': { static const uint8_t data[5] = {0x7F,0x04,0x08,0x10,0x7F}; return data; }
        case 'O': { static const uint8_t data[5] = {0x3E,0x41,0x41,0x41,0x3E}; return data; }
        case 'P': { static const uint8_t data[5] = {0x7F,0x09,0x09,0x09,0x06}; return data; }
        case 'Q': { static const uint8_t data[5] = {0x3E,0x41,0x51,0x21,0x5E}; return data; }
        case 'R': { static const uint8_t data[5] = {0x7F,0x09,0x19,0x29,0x46}; return data; }
        case 'S': { static const uint8_t data[5] = {0x46,0x49,0x49,0x49,0x31}; return data; }
        case 'T': { static const uint8_t data[5] = {0x01,0x01,0x7F,0x01,0x01}; return data; }
        case 'U': { static const uint8_t data[5] = {0x3F,0x40,0x40,0x40,0x3F}; return data; }
        case 'V': { static const uint8_t data[5] = {0x1F,0x20,0x40,0x20,0x1F}; return data; }
        case 'W': { static const uint8_t data[5] = {0x7F,0x20,0x18,0x20,0x7F}; return data; }
        case 'X': { static const uint8_t data[5] = {0x63,0x14,0x08,0x14,0x63}; return data; }
        case 'Y': { static const uint8_t data[5] = {0x03,0x04,0x78,0x04,0x03}; return data; }
        case 'Z': { static const uint8_t data[5] = {0x61,0x51,0x49,0x45,0x43}; return data; }
        // Chữ số
        case '0': { static const uint8_t data[5] = {0x3E,0x51,0x49,0x45,0x3E}; return data; }
        case '1': { static const uint8_t data[5] = {0x00,0x42,0x7F,0x40,0x00}; return data; }
        case '2': { static const uint8_t data[5] = {0x42,0x61,0x51,0x49,0x46}; return data; }
        case '3': { static const uint8_t data[5] = {0x21,0x41,0x45,0x4B,0x31}; return data; }
        case '4': { static const uint8_t data[5] = {0x18,0x14,0x12,0x7F,0x10}; return data; }
        case '5': { static const uint8_t data[5] = {0x27,0x45,0x45,0x45,0x39}; return data; }
        case '6': { static const uint8_t data[5] = {0x3C,0x4A,0x49,0x49,0x30}; return data; }
        case '7': { static const uint8_t data[5] = {0x01,0x71,0x09,0x05,0x03}; return data; }
        case '8': { static const uint8_t data[5] = {0x36,0x49,0x49,0x49,0x36}; return data; }
        case '9': { static const uint8_t data[5] = {0x06,0x49,0x49,0x29,0x1E}; return data; }

        // Dấu chấm câu
        case ':': { static const uint8_t data[5] = {0x00,0x36,0x36,0x00,0x00}; return data; }
        case '.': { static const uint8_t data[5] = {0x00,0x40,0x60,0x00,0x00}; return data; }

        default:  { static const uint8_t data[5] = {0x00,0x00,0x00,0x00,0x00}; return data; }
    }
}

/*=================== Vẽ ký tự và chuỗi ====================*/
// Vẽ một ký tự tại vị trí (x, page). Mỗi ký tự có kích thước 5x7 pixel + 1 pixel khoảng cách.
static void ssd1306_draw_char(uint8_t x, uint8_t page, char c)
{
    const uint8_t *bitmap = get_font_data(c);
    for (int col = 0; col < 5; col++) {
        uint8_t line = bitmap[col];
        for (int row = 0; row < 7; row++) {
            if (line & (1 << row)) {
                uint16_t index = (page * OLED_WIDTH) + x + col;
                if (index < OLED_BUFFER_SIZE)
                    oled_buffer[index] |= (1 << row);
            }
        }
    }
    // Thêm 1 cột trắng làm khoảng cách sau ký tự
    for (int row = 0; row < 7; row++) {
        uint16_t index = (page * OLED_WIDTH) + x + 5;
        if (index < OLED_BUFFER_SIZE)
            oled_buffer[index] &= ~(1 << row);
    }
}

// Vẽ chuỗi ký tự bắt đầu từ vị trí (x, page)
void oled_draw_string(uint8_t x, uint8_t page, const char *str)
{
    while (*str) {
        ssd1306_draw_char(x, page, *str);
        x += 6; // 5 pixel cho ký tự + 1 pixel khoảng cách
        str++;
    }
}

/*=================== API Thư Viện ====================*/
esp_err_t oled_init(void)
{
    esp_err_t err = i2c_master_init();
    if(err != ESP_OK) {
        ESP_LOGE(TAG, "I2C init failed");
        return err;
    }
    ssd1306_init();
    oled_clear();
    return ESP_OK;
}

void oled_clear(void)
{
    memset(oled_buffer, 0, OLED_BUFFER_SIZE);
}

esp_err_t oled_display(void)
{
    for(uint8_t page = 0; page < 8; page++){
        ssd1306_send_command(0xB0 + page); // Set page address
        ssd1306_send_command(0x00);        // Set lower column start address
        ssd1306_send_command(0x10);        // Set higher column start address
        // Gửi 128 byte dữ liệu cho mỗi trang
        esp_err_t ret = ssd1306_send_data(&oled_buffer[OLED_WIDTH * page], OLED_WIDTH);
        if(ret != ESP_OK)
            return ret;
    }
    return ESP_OK;
}

// Hàm in thông tin dạng printf-like lên OLED
void oled_print(uint8_t x, uint8_t page, const char *format, ...)
{
    char buffer[64];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    oled_draw_string(x, page, buffer);
}
