#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/event_groups.h>
#include <esp_log.h>
#include <esp_wifi.h>
#include <esp_event.h>
#include <nvs_flash.h>
#include <wifi_provisioning/manager.h>
#include <wifi_provisioning/scheme_ble.h>
#include "qrcode.h"
#include <lwip/sockets.h>
#include "motor.h" // Thư viện điều khiển motor riêng
#include "oled.h"  // oled

// Constants and definitions
static const char *TAG = "app";
#define EXAMPLE_PROV_SEC2_USERNAME "wifiprov"
#define EXAMPLE_PROV_SEC2_PWD "28090208"
#define PROV_QR_VERSION "v1"
#define PROV_TRANSPORT_BLE "ble"
#define QRCODE_BASE_URL "https://espressif.github.io/esp-jumpstart/qrcode.html"
#define UDP_PORT 65000

uint8_t buffer[6];
// Hard coded salt và verifier (Security 2)
static const char sec2_salt[] = {
    0x03, 0x6e, 0xe0, 0xc7, 0xbc, 0xb9, 0xed, 0xa8,
    0x4c, 0x9e, 0xac, 0x97, 0xd9, 0x3d, 0xec, 0xf4};

static const char sec2_verifier[] = {
    0x7c, 0x7c, 0x85, 0x47, 0x65, 0x08, 0x94, 0x6d, 0xd6, 0x36, 0xaf, 0x37, 0xd7, 0xe8, 0x91, 0x43,
    0x78, 0xcf, 0xfd, 0x61, 0x6c, 0x59, 0xd2, 0xf8, 0x39, 0x08, 0x12, 0x72, 0x38, 0xde, 0x9e, 0x24,
    0xa4, 0x70, 0x26, 0x1c, 0xdf, 0xa9, 0x03, 0xc2, 0xb2, 0x70, 0xe7, 0xb1, 0x32, 0x24, 0xda, 0x11,
    0x1d, 0x97, 0x18, 0xdc, 0x60, 0x72, 0x08, 0xcc, 0x9a, 0xc9, 0x0c, 0x48, 0x27, 0xe2, 0xae, 0x89,
    0xaa, 0x16, 0x25, 0xb8, 0x04, 0xd2, 0x1a, 0x9b, 0x3a, 0x8f, 0x37, 0xf6, 0xe4, 0x3a, 0x71, 0x2e,
    0xe1, 0x27, 0x86, 0x6e, 0xad, 0xce, 0x28, 0xff, 0x54, 0x46, 0x60, 0x1f, 0xb9, 0x96, 0x87, 0xdc,
    0x57, 0x40, 0xa7, 0xd4, 0x6c, 0xc9, 0x77, 0x54, 0xdc, 0x16, 0x82, 0xf0, 0xed, 0x35, 0x6a, 0xc4,
    0x70, 0xad, 0x3d, 0x90, 0xb5, 0x81, 0x94, 0x70, 0xd7, 0xbc, 0x65, 0xb2, 0xd5, 0x18, 0xe0, 0x2e,
    0xc3, 0xa5, 0xf9, 0x68, 0xdd, 0x64, 0x7b, 0xb8, 0xb7, 0x3c, 0x9c, 0xfc, 0x00, 0xd8, 0x71, 0x7e,
    0xb7, 0x9a, 0x7c, 0xb1, 0xb7, 0xc2, 0xc3, 0x18, 0x34, 0x29, 0x32, 0x43, 0x3e, 0x00, 0x99, 0xe9,
    0x82, 0x94, 0xe3, 0xd8, 0x2a, 0xb0, 0x96, 0x29, 0xb7, 0xdf, 0x0e, 0x5f, 0x08, 0x33, 0x40, 0x76,
    0x52, 0x91, 0x32, 0x00, 0x9f, 0x97, 0x2c, 0x89, 0x6c, 0x39, 0x1e, 0xc8, 0x28, 0x05, 0x44, 0x17,
    0x3f, 0x68, 0x02, 0x8a, 0x9f, 0x44, 0x61, 0xd1, 0xf5, 0xa1, 0x7e, 0x5a, 0x70, 0xd2, 0xc7, 0x23,
    0x81, 0xcb, 0x38, 0x68, 0xe4, 0x2c, 0x20, 0xbc, 0x40, 0x57, 0x76, 0x17, 0xbd, 0x08, 0xb8, 0x96,
    0xbc, 0x26, 0xeb, 0x32, 0x46, 0x69, 0x35, 0x05, 0x8c, 0x15, 0x70, 0xd9, 0x1b, 0xe9, 0xbe, 0xcc,
    0xa9, 0x38, 0xa6, 0x67, 0xf0, 0xad, 0x50, 0x13, 0x19, 0x72, 0x64, 0xbf, 0x52, 0xc2, 0x34, 0xe2,
    0x1b, 0x11, 0x79, 0x74, 0x72, 0xbd, 0x34, 0x5b, 0xb1, 0xe2, 0xfd, 0x66, 0x73, 0xfe, 0x71, 0x64,
    0x74, 0xd0, 0x4e, 0xbc, 0x51, 0x24, 0x19, 0x40, 0x87, 0x0e, 0x92, 0x40, 0xe6, 0x21, 0xe7, 0x2d,
    0x4e, 0x37, 0x76, 0x2f, 0x2e, 0xe2, 0x68, 0xc7, 0x89, 0xe8, 0x32, 0x13, 0x42, 0x06, 0x84, 0x84,
    0x53, 0x4a, 0xb3, 0x0c, 0x1b, 0x4c, 0x8d, 0x1c, 0x51, 0x97, 0x19, 0xab, 0xae, 0x77, 0xff, 0xdb,
    0xec, 0xf0, 0x10, 0x95, 0x34, 0x33, 0x6b, 0xcb, 0x3e, 0x84, 0x0f, 0xb9, 0xd8, 0x5f, 0xb8, 0xa0,
    0xb8, 0x55, 0x53, 0x3e, 0x70, 0xf7, 0x18, 0xf5, 0xce, 0x7b, 0x4e, 0xbf, 0x27, 0xce, 0xce, 0xa8,
    0xb3, 0xbe, 0x40, 0xc5, 0xc5, 0x32, 0x29, 0x3e, 0x71, 0x64, 0x9e, 0xde, 0x8c, 0xf6, 0x75, 0xa1,
    0xe6, 0xf6, 0x53, 0xc8, 0x31, 0xa8, 0x78, 0xde, 0x50, 0x40, 0xf7, 0x62, 0xde, 0x36, 0xb2, 0xba};

// Global variables for Wi-Fi connection and task control
const int WIFI_CONNECTED_EVENT = BIT0;
static EventGroupHandle_t wifi_event_group;
TaskHandle_t udp_task_handle = NULL;
static volatile bool udp_running = false;

/*---------------------------------------------------------------
 * UDP listener task:
 * Nhận dữ liệu UDP dạng binary (6 byte):
 *   - 2 byte: j1X (int16_t, little endian)
 *   - 2 byte: j1Y (int16_t, little endian)
 *   - 2 byte: speed (int16_t, little endian)
 * Điều khiển motor theo giá trị j1X (dương: quay thuận, âm: quay nghịch)
 *--------------------------------------------------------------*/

void udp_listener_task(void *pvParameters)
{
    int sock = -1;
    struct sockaddr_in server_addr;

    // Chờ WiFi kết nối thành công
    xEventGroupWaitBits(wifi_event_group, WIFI_CONNECTED_EVENT, pdFALSE, pdTRUE, portMAX_DELAY);

    sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    if (sock < 0)
    {
        ESP_LOGE(TAG, "Không thể tạo socket");
        vTaskDelete(NULL);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(UDP_PORT);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        ESP_LOGE(TAG, "Không thể bind socket: %d", errno);
        close(sock);
        vTaskDelete(NULL);
    }

    udp_running = true;
    ESP_LOGI(TAG, "Bắt đầu UDP listener trên cổng %d", UDP_PORT);

    char buffer[128];
    struct sockaddr_in source_addr;
    socklen_t socklen = sizeof(source_addr);

    while (udp_running)
    {
        int len = recvfrom(sock, buffer, sizeof(buffer), 0,
                           (struct sockaddr *)&source_addr, &socklen);
        if (len == 6)
        {
            int16_t j1X, j1Y, speed_max;
            memcpy(&j1X, buffer, 2);
            memcpy(&j1Y, buffer + 2, 2);
            memcpy(&speed_max, buffer + 4, 2);

            float raw_angle = calculate_angle(j1X, j1Y);
            float angle = normalize_angle(raw_angle);
            ESP_LOGI(TAG, "Nhận dữ liệu: j1X=%d, j1Y=%d, angle=%.2f", j1X, j1Y, angle);
            // xe chạy motor quang ngân
            servo_set_angle(90 + angle);
            motor_control(j1Y, 1024);

            oled_clear();
            oled_print(0, 0, "x = %d", j1X);
            oled_print(0, 1, "y = %d", j1Y);
            oled_print(0, 3, "angle = %.2f", angle);
            oled_display();

            vTaskDelay(pdMS_TO_TICKS(10));
        }
        else if (len < 0 && udp_running)
        {
            ESP_LOGE(TAG, "Lỗi nhận UDP: %d", errno);
        }
        else
        {
            ESP_LOGW(TAG, "Dữ liệu không hợp lệ: %d bytes", len);
        }
    }

    close(sock);
    ESP_LOGI(TAG, "Đóng socket UDP");
    oled_clear();
    oled_print(0, 5, "close Socket UDP");
    oled_display();
    vTaskDelete(NULL);
}

/*---------------------------------------------------------------
 * Khởi tạo và bắt đầu task UDP listener
 *--------------------------------------------------------------*/
void start_udp_task(void)
{
    if (udp_task_handle == NULL && !udp_running)
    {
        xTaskCreate(udp_listener_task, "udp_listener", 4096, NULL, 5, &udp_task_handle);
        ESP_LOGI(TAG, "UDP task started");
    }
}

/*---------------------------------------------------------------
 * Dừng task UDP listener
 *--------------------------------------------------------------*/
void stop_udp_task(void)
{
    if (udp_task_handle != NULL && udp_running)
    {
        udp_running = false;
        vTaskDelay(100 / portTICK_PERIOD_MS);
        if (udp_task_handle != NULL)
        {
            vTaskDelete(udp_task_handle);
            udp_task_handle = NULL;
            ESP_LOGI(TAG, "UDP task stopped");
        }
    }
}

/*---------------------------------------------------------------
 * Các hàm hỗ trợ provisioning và xử lý sự kiện
 *--------------------------------------------------------------*/
static esp_err_t example_get_sec2_salt(const char **salt, uint16_t *salt_len)
{
    ESP_LOGI(TAG, "Development mode: using hard coded salt");
    *salt = sec2_salt;
    *salt_len = sizeof(sec2_salt);
    return ESP_OK;
}

static esp_err_t example_get_sec2_verifier(const char **verifier, uint16_t *verifier_len)
{
    ESP_LOGI(TAG, "Development mode: using hard coded verifier");
    *verifier = sec2_verifier;
    *verifier_len = sizeof(sec2_verifier);
    return ESP_OK;
}

static void event_handler(void *arg, esp_event_base_t event_base,
                          int32_t event_id, void *event_data)
{
    static int retries = 0;

    if (event_base == WIFI_PROV_EVENT)
    {
        switch (event_id)
        {
        case WIFI_PROV_START:
            ESP_LOGI(TAG, "Provisioning started");
            stop_udp_task();
            break;
        case WIFI_PROV_CRED_RECV:
        {
            wifi_sta_config_t *wifi_sta_cfg = (wifi_sta_config_t *)event_data;
            ESP_LOGI(TAG, "Nhận thông tin Wi-Fi\n\tSSID : %s\n\tPassword : %s",
                     (const char *)wifi_sta_cfg->ssid, (const char *)wifi_sta_cfg->password);
            oled_clear();
            oled_print(10, 0, "Connected wifi");
            oled_print(0, 1, "SSID %s", (const char *)wifi_sta_cfg->ssid);
            oled_print(0, 2, "Password %s", (const char *)wifi_sta_cfg->password);
            oled_display();
            break;
        }
        case WIFI_PROV_CRED_FAIL:
        {
            wifi_prov_sta_fail_reason_t *reason = (wifi_prov_sta_fail_reason_t *)event_data;
            ESP_LOGE(TAG, "Provisioning thất bại! Lý do: %s",
                     (*reason == WIFI_PROV_STA_AUTH_ERROR) ? "Lỗi xác thực" : "Không tìm thấy AP");
            retries++;
            if (retries >= CONFIG_EXAMPLE_PROV_MGR_MAX_RETRY_CNT)
            {
                ESP_LOGI(TAG, "Không kết nối được, reset thông tin provisioned");
                wifi_prov_mgr_reset_sm_state_on_failure();
                retries = 0;
            }
            break;
        }
        case WIFI_PROV_CRED_SUCCESS:
            ESP_LOGI(TAG, "Provisioning thành công");
            retries = 0;
            break;
        case WIFI_PROV_END:
            wifi_prov_mgr_deinit();
            break;
        default:
            break;
        }
    }
    else if (event_base == WIFI_EVENT)
    {
        switch (event_id)
        {
        case WIFI_EVENT_STA_START:
            esp_wifi_connect();
            break;
        case WIFI_EVENT_STA_DISCONNECTED:
            ESP_LOGI(TAG, "Mất kết nối. Đang kết nối lại...");
            oled_clear();
            oled_print(5, 3, "Disconected........Try again");
            oled_display();
            esp_wifi_connect();
            stop_udp_task();
            break;
        default:
            break;
        }
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG, "Kết nối thành công với IP: " IPSTR, IP2STR(&event->ip_info.ip));

        oled_clear();
        oled_print(10, 0, "IP Config");
        oled_print(0, 2, "ip: " IPSTR, IP2STR(&event->ip_info.ip));
        oled_print(0, 1, "port: %d", UDP_PORT);
        oled_display();

        xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_EVENT);
        start_udp_task();
    }
}

static void wifi_init_sta(void)
{
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());
}

static void get_device_service_name(char *service_name, size_t max)
{
    uint8_t eth_mac[6];
    const char *ssid_prefix = "PROV_";
    esp_wifi_get_mac(WIFI_IF_STA, eth_mac);
    snprintf(service_name, max, "%s%02X%02X%02X", ssid_prefix, eth_mac[3], eth_mac[4], eth_mac[5]);
}

esp_err_t custom_prov_data_handler(uint32_t session_id, const uint8_t *inbuf, ssize_t inlen,
                                   uint8_t **outbuf, ssize_t *outlen, void *priv_data)
{
    if (inbuf)
    {
        ESP_LOGI(TAG, "Nhận dữ liệu: %.*s", inlen, (char *)inbuf);
    }
    char response[] = "SUCCESS";
    *outbuf = (uint8_t *)strdup(response);
    if (*outbuf == NULL)
    {
        ESP_LOGE(TAG, "Hết bộ nhớ");
        return ESP_ERR_NO_MEM;
    }
    *outlen = strlen(response) + 1;
    return ESP_OK;
}

static void wifi_prov_print_qr(const char *name, const char *username,
                               const char *pop, const char *transport)
{
    char payload[150] = {0};
    snprintf(payload, sizeof(payload),
             "{\"ver\":\"%s\",\"name\":\"%s\",\"username\":\"%s\",\"pop\":\"%s\",\"transport\":\"%s\"}",
             PROV_QR_VERSION, name, username, pop, transport);

    ESP_LOGI(TAG, "Quét QR code này từ ứng dụng provisioning.");
    esp_qrcode_config_t cfg = ESP_QRCODE_CONFIG_DEFAULT();
    esp_qrcode_generate(&cfg, payload);
    ESP_LOGI(TAG, "Nếu không thấy QR code, copy URL sau vào trình duyệt:\n%s?data=%s",
             QRCODE_BASE_URL, payload);
    oled_clear();
    oled_print(0, 0, "%s", name);
    oled_display();
}

/*---------------------------------------------------------------
 * Hàm main chính: khởi tạo hệ thống, provisioning và khởi chạy UDP listener
 *--------------------------------------------------------------*/
void app_main(void)
{
    // khởi tạo màn oled
    if (oled_init() != ESP_OK)
    {
        ESP_LOGE("APP", "OLED init failed");
        return;
    }

    oled_clear();

    // Khởi tạo NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    }
    // Khởi tạo module motor (PWM, cấu hình GPIO)
    servo_init();
    pwm_init();

    // Khởi tạo network stack và event loop
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    wifi_event_group = xEventGroupCreate();

    // Đăng ký các event handler cho provisioning, Wi-Fi và IP
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_PROV_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL));

    // Khởi tạo Wi-Fi
    esp_netif_create_default_wifi_sta();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    // Cấu hình provisioning manager
    wifi_prov_mgr_config_t prov_config = {
        .scheme = wifi_prov_scheme_ble,
        .scheme_event_handler = WIFI_PROV_SCHEME_BLE_EVENT_HANDLER_FREE_BTDM};
    ESP_ERROR_CHECK(wifi_prov_mgr_init(prov_config));

    bool provisioned = false;
    wifi_prov_mgr_reset_provisioning();

    if (!provisioned)
    {
        ESP_LOGI(TAG, "Bắt đầu quá trình provisioning");
        oled_clear();
        oled_print(0, 5, "Start provisioning");
        oled_display();
        char service_name[12];
        get_device_service_name(service_name, sizeof(service_name));

        wifi_prov_security_t security = WIFI_PROV_SECURITY_2;
        const char *username = EXAMPLE_PROV_SEC2_USERNAME;
        const char *pop = EXAMPLE_PROV_SEC2_PWD;

        wifi_prov_security2_params_t sec2_params = {0};
        ESP_ERROR_CHECK(example_get_sec2_salt(&sec2_params.salt, &sec2_params.salt_len));
        ESP_ERROR_CHECK(example_get_sec2_verifier(&sec2_params.verifier, &sec2_params.verifier_len));

        const char *service_key = NULL;

        uint8_t custom_service_uuid[] = {
            0xb4, 0xdf, 0x5a, 0x1c, 0x3f, 0x6b, 0xf4, 0xbf,
            0xea, 0x4a, 0x82, 0x03, 0x04, 0x90, 0x1a, 0x02};

        wifi_prov_scheme_ble_set_service_uuid(custom_service_uuid);
        wifi_prov_mgr_endpoint_create("custom-data");
        wifi_prov_mgr_disable_auto_stop(1000);

        ESP_ERROR_CHECK(wifi_prov_mgr_start_provisioning(security,
                                                         (const void *)&sec2_params,
                                                         service_name,
                                                         service_key));
        wifi_prov_mgr_endpoint_register("custom-data", custom_prov_data_handler, NULL);

        wifi_prov_print_qr(service_name, username, pop, PROV_TRANSPORT_BLE);
    }
    else
    {
        ESP_LOGI(TAG, "Đã được provision, khởi động Wi-Fi STA");
        oled_clear();
        oled_print(0, 5, "Start Wi-Fi STA");
        oled_display();
        wifi_prov_mgr_deinit();
        wifi_init_sta();
    }

    // Vòng lặp chính: chờ sự kiện kết nối và giữ hệ thống hoạt động
    while (1)
    {
        xEventGroupWaitBits(wifi_event_group, WIFI_CONNECTED_EVENT, pdFALSE, pdTRUE, portMAX_DELAY);
        ESP_LOGI(TAG, "Hệ thống đang chạy với UDP listener và motor control");
        vTaskDelay(portMAX_DELAY);
    }
}
