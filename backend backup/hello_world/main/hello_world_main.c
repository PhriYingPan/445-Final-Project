

#include <stdio.h>
#include <string.h>
#include <time.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "driver/gpio.h"
#include "driver/adc.h"
#include "driver/spi_master.h"
#include "esp_log.h"
#include "esp_http_client.h"
#include "esp_wifi.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "cJSON.h"
#include "esp_crt_bundle.h"

#include "esp_sntp.h"

// #include <u8g2.h>
// #include "u8g2_esp32_hal.h"


/* -------------------------------------------------------------------------- */
/*  Wi‑Fi credentials                                                         */
/* -------------------------------------------------------------------------- */
// #define WIFI_SSID "NETGEAR34"
// #define WIFI_PASS "silentspider842"
#define WIFI_SSID "Conan's iPhone (2)"
#define WIFI_PASS "wec6n3340tvv"

/* -------------------------------------------------------------------------- */
/*  Firebase Realtime‑DB                                                      */
/* -------------------------------------------------------------------------- */
#define FIREBASE_HOST "https://desklearningapp-default-rtdb.firebaseio.com"  /* no trailing slash */
#define FIREBASE_CURRENT_PROMPT FIREBASE_HOST "/currentPrompt.json"          /* GET  */


#define FIREBASE_RESPONSES_PATH  FIREBASE_HOST "/students/%s/responses.json" /* POST */

/* -------------------------------------------------------------------------- */
/*  Application constants                                                     */
/* -------------------------------------------------------------------------- */
#define STUDENT_ID "student1"
#define STUDENT_NAME "Ethan Ge"

#define RFID_TAG   "04A3C2"

/*  Button GPIOs – avoid 6–11 (SPI‑flash) and 34–39 (input‑only)              */
#define BTN_A 5
#define BTN_B 6
#define BTN_C 7
#define BTN_D 15
#define BTN_E 16

static const char *TAG = "ESP32_FIREBASE";

// CORRECT: ADC1_CHANNEL_8 is on GPIO 9
#define POT_CHANNEL ADC1_CHANNEL_8



static void obtain_time(void)
{
    /* 1-line setup: poll “pool.ntp.org” every 60 s (default) */
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, "pool.ntp.org");
    esp_sntp_init();

    /* block (max ~10 s) until we get a sane epoch */
    time_t now = 0;
    struct tm tm = {0};
    int retry = 0;
    const int max_retry = 20;
    while (tm.tm_year < (2024 - 1900) && ++retry < max_retry) {
        ESP_LOGI(TAG, "⏳ Waiting for NTP (%d/%d)…", retry, max_retry);
        vTaskDelay(pdMS_TO_TICKS(1000));
        time(&now);
        localtime_r(&now, &tm);
    }
    if (tm.tm_year < (2024 - 1900)) {
        ESP_LOGW(TAG, "⚠️  NTP sync failed—timestamps will be wrong");
    } else {
        ESP_LOGI(TAG, "✅ Time set: %s", asctime(&tm));
    }
}


/* -------------------------------------------------------------------------- */
/*  Prompt structure                                                          */
/* -------------------------------------------------------------------------- */
typedef struct {
    char question[128];
    char subject[32];
    char correctAnswer[4];
} Prompt;


/* =============================  NEW: LCD DEFS  ============================ */
#define PIN_MOSI 10    // DIN on the DOGS104
#define PIN_SCLK 11    // CLK
#define PIN_CS   13    // CS#
#define PIN_DC   14    // SA0 / D-C#
#define PIN_RST  12    // RST#

static const char *LCD_TAG = "DOGS104-U8g2";

// static spi_device_handle_t lcd;
// // static const char *LCD_TAG = "LCD";


// /* 5×8 font taken from classic PC BIOS (only printable 0x20–0x7F). */
// static const uint8_t font5x8[96][5] = {
//     {0x00,0x00,0x00,0x00,0x00}, /* 0x20 ' ' */
//     {0x00,0x00,0x5f,0x00,0x00}, /* 0x21 '!' */
//     {0x00,0x07,0x00,0x07,0x00}, /* 0x22 '"' */
//     {0x14,0x7f,0x14,0x7f,0x14}, /* 0x23 '#' */
//     {0x24,0x2a,0x7f,0x2a,0x12}, /* 0x24 '$' */
//     {0x23,0x13,0x08,0x64,0x62}, /* 0x25 '%' */
//     {0x36,0x49,0x55,0x22,0x50}, /* 0x26 '&' */
//     {0x00,0x05,0x03,0x00,0x00}, /* 0x27 ''' */
//     {0x00,0x1c,0x22,0x41,0x00}, /* 0x28 '(' */
//     {0x00,0x41,0x22,0x1c,0x00}, /* 0x29 ')' */
//     {0x14,0x08,0x3e,0x08,0x14}, /* 0x2A '*' */
//     {0x08,0x08,0x3e,0x08,0x08}, /* 0x2B '+' */
//     {0x00,0x50,0x30,0x00,0x00}, /* 0x2C ',' */
//     {0x08,0x08,0x08,0x08,0x08}, /* 0x2D '-' */
//     {0x00,0x60,0x60,0x00,0x00}, /* 0x2E '.' */
//     {0x20,0x10,0x08,0x04,0x02}, /* 0x2F '/' */
//     {0x3e,0x51,0x49,0x45,0x3e}, /* 0x30 '0' */
//     {0x00,0x42,0x7f,0x40,0x00}, /* 0x31 '1' */
//     {0x42,0x61,0x51,0x49,0x46}, /* 0x32 '2' */
//     {0x21,0x41,0x45,0x4b,0x31}, /* 0x33 '3' */
//     {0x18,0x14,0x12,0x7f,0x10}, /* 0x34 '4' */
//     {0x27,0x45,0x45,0x45,0x39}, /* 0x35 '5' */
//     {0x3c,0x4a,0x49,0x49,0x30}, /* 0x36 '6' */
//     {0x01,0x71,0x09,0x05,0x03}, /* 0x37 '7' */
//     {0x36,0x49,0x49,0x49,0x36}, /* 0x38 '8' */
//     {0x06,0x49,0x49,0x29,0x1e}, /* 0x39 '9' */
//     {0x00,0x36,0x36,0x00,0x00}, /* 0x3A ':' */
//     {0x00,0x56,0x36,0x00,0x00}, /* 0x3B ';' */
//     {0x08,0x14,0x22,0x41,0x00}, /* 0x3C '<' */
//     {0x14,0x14,0x14,0x14,0x14}, /* 0x3D '=' */
//     {0x00,0x41,0x22,0x14,0x08}, /* 0x3E '>' */
//     {0x02,0x01,0x51,0x09,0x06}, /* 0x3F '?' */
//     {0x32,0x49,0x79,0x41,0x3e}, /* 0x40 '@' */
//     {0x7e,0x11,0x11,0x11,0x7e}, /* 0x41 'A' */
//     {0x7f,0x49,0x49,0x49,0x36}, /* 0x42 'B' */
//     {0x3e,0x41,0x41,0x41,0x22}, /* 0x43 'C' */
//     {0x7f,0x41,0x41,0x22,0x1c}, /* 0x44 'D' */
//     {0x7f,0x49,0x49,0x49,0x41}, /* 0x45 'E' */
//     {0x7f,0x09,0x09,0x09,0x01}, /* 0x46 'F' */
//     {0x3e,0x41,0x49,0x49,0x7a}, /* 0x47 'G' */
//     {0x7f,0x08,0x08,0x08,0x7f}, /* 0x48 'H' */
//     {0x00,0x41,0x7f,0x41,0x00}, /* 0x49 'I' */
//     {0x20,0x40,0x41,0x3f,0x01}, /* 0x4A 'J' */
//     {0x7f,0x08,0x14,0x22,0x41}, /* 0x4B 'K' */
//     {0x7f,0x40,0x40,0x40,0x40}, /* 0x4C 'L' */
//     {0x7f,0x02,0x0c,0x02,0x7f}, /* 0x4D 'M' */
//     {0x7f,0x04,0x08,0x10,0x7f}, /* 0x4E 'N' */
//     {0x3e,0x41,0x41,0x41,0x3e}, /* 0x4F 'O' */
//     {0x7f,0x09,0x09,0x09,0x06}, /* 0x50 'P' */
//     {0x3e,0x41,0x51,0x21,0x5e}, /* 0x51 'Q' */
//     {0x7f,0x09,0x19,0x29,0x46}, /* 0x52 'R' */
//     {0x46,0x49,0x49,0x49,0x31}, /* 0x53 'S' */
//     {0x01,0x01,0x7f,0x01,0x01}, /* 0x54 'T' */
//     {0x3f,0x40,0x40,0x40,0x3f}, /* 0x55 'U' */
//     {0x1f,0x20,0x40,0x20,0x1f}, /* 0x56 'V' */
//     {0x3f,0x40,0x38,0x40,0x3f}, /* 0x57 'W' */
//     {0x63,0x14,0x08,0x14,0x63}, /* 0x58 'X' */
//     {0x07,0x08,0x70,0x08,0x07}, /* 0x59 'Y' */
//     {0x61,0x51,0x49,0x45,0x43}, /* 0x5A 'Z' */
//     {0x00,0x7f,0x41,0x41,0x00}, /* 0x5B '[' */
//     {0x02,0x04,0x08,0x10,0x20}, /* 0x5C '\' */
//     {0x00,0x41,0x41,0x7f,0x00}, /* 0x5D ']' */
//     {0x04,0x02,0x01,0x02,0x04}, /* 0x5E '^' */
//     {0x40,0x40,0x40,0x40,0x40}, /* 0x5F '_' */
//     {0x00,0x01,0x02,0x04,0x00}, /* 0x60 '`' */
//     {0x20,0x54,0x54,0x54,0x78}, /* 0x61 'a' */
//     {0x7f,0x48,0x44,0x44,0x38}, /* 0x62 'b' */
//     {0x38,0x44,0x44,0x44,0x20}, /* 0x63 'c' */
//     {0x38,0x44,0x44,0x48,0x7f}, /* 0x64 'd' */
//     {0x38,0x54,0x54,0x54,0x18}, /* 0x65 'e' */
//     {0x08,0x7e,0x09,0x01,0x02}, /* 0x66 'f' */
//     {0x0c,0x52,0x52,0x52,0x3e}, /* 0x67 'g' */
//     {0x7f,0x08,0x04,0x04,0x78}, /* 0x68 'h' */
//     {0x00,0x44,0x7d,0x40,0x00}, /* 0x69 'i' */
//     {0x20,0x40,0x44,0x3d,0x00}, /* 0x6A 'j' */
//     {0x7f,0x10,0x28,0x44,0x00}, /* 0x6B 'k' */
//     {0x00,0x41,0x7f,0x40,0x00}, /* 0x6C 'l' */
//     {0x7c,0x04,0x18,0x04,0x7c}, /* 0x6D 'm' */
//     {0x7c,0x08,0x04,0x04,0x78}, /* 0x6E 'n' */
//     {0x38,0x44,0x44,0x44,0x38}, /* 0x6F 'o' */
//     {0x7c,0x14,0x14,0x14,0x08}, /* 0x70 'p' */
//     {0x08,0x14,0x14,0x18,0x7c}, /* 0x71 'q' */
//     {0x7c,0x08,0x04,0x04,0x08}, /* 0x72 'r' */
//     {0x48,0x54,0x54,0x54,0x20}, /* 0x73 's' */
//     {0x04,0x3f,0x44,0x40,0x20}, /* 0x74 't' */
//     {0x3c,0x40,0x40,0x20,0x7c}, /* 0x75 'u' */
//     {0x1c,0x20,0x40,0x20,0x1c}, /* 0x76 'v' */
//     {0x3c,0x40,0x30,0x40,0x3c}, /* 0x77 'w' */
//     {0x44,0x28,0x10,0x28,0x44}, /* 0x78 'x' */
//     {0x0c,0x50,0x50,0x50,0x3c}, /* 0x79 'y' */
//     {0x44,0x64,0x54,0x4c,0x44}, /* 0x7A 'z' */
//     {0x00,0x08,0x36,0x41,0x00}, /* 0x7B '{' */
//     {0x00,0x00,0x7f,0x00,0x00}, /* 0x7C '|' */
//     {0x00,0x41,0x36,0x08,0x00}, /* 0x7D '}' */
//     {0x08,0x08,0x2a,0x1c,0x08}, /* 0x7E '~' */
//     {0x00,0x00,0x00,0x00,0x00}  /* 0x7F (DEL) */
// };

// /* --------------------- low‑level LCD helpers ------------------------------ */
// static inline void lcd_dc_low(void)  { gpio_set_level(LCD_DC, 0); }
// static inline void lcd_dc_high(void) { gpio_set_level(LCD_DC, 1); }



// static void lcd_cmd(uint8_t c)
// {
//     lcd_dc_low();
//     spi_transaction_t t = {.length = 8, .tx_buffer = &c};
//     spi_device_transmit(lcd, &t);
// }

// static void lcd_set_contrast(uint8_t reg_ratio, uint8_t volume)
// /* reg_ratio = 0x20–0x27 , volume = 0x00–0x3F */
// {
//     lcd_cmd(reg_ratio);        // coarse
//     lcd_cmd(0x81);             // electronic volume command
//     lcd_cmd(volume);           // fine
// }

// static void lcd_data(const uint8_t *d, size_t n)
// {
//     lcd_dc_high();
//     spi_transaction_t t = {.length = n * 8, .tx_buffer = d};
//     spi_device_transmit(lcd, &t);
// }

// static void lcd_set_addr(uint8_t page, uint8_t col)
// {
//     lcd_cmd(0xB0 | (page & 0x0F));
//     lcd_cmd(0x10 | (col >> 4));
//     lcd_cmd(0x00 | (col & 0x0F));
// }

// static void lcd_clear(void)
// {
//     uint8_t blank[104] = {0};
//     for (uint8_t p = 0; p < 8; ++p) {
//         lcd_set_addr(p, 0);
//         lcd_data(blank, sizeof(blank));
//     }
// }

// static void draw_char(uint8_t page, uint8_t col, char ch)
// {
//     if (ch < 0x20 || ch > 0x7F) ch = '?';
//     lcd_set_addr(page, col);
//     lcd_data(font5x8[ch - 0x20], 5);
//     uint8_t gap = 0; lcd_data(&gap, 1);
// }

// static void draw_str(uint8_t page, const char *s)
// {
//     uint8_t col = 0;
//     while (*s && col < 100) { draw_char(page, col, *s++); col += 6; }
// }

// /* --------------------- LCD initialisation (EA sample) --------------------- */
// static void lcd_init(void)
// {
//     /* GPIO for DC & RST */
//     gpio_config_t g = {.mode = GPIO_MODE_OUTPUT,
//                        .pin_bit_mask = (1ULL << LCD_DC) | (1ULL << LCD_RST)};
//     gpio_config(&g);

//     /* Reset pulse */
//     gpio_set_level(LCD_RST, 0);
//     vTaskDelay(pdMS_TO_TICKS(5));
//     gpio_set_level(LCD_RST, 1);

//     /* SPI bus @ 2 MHz (safe) */
//     spi_bus_config_t bus = {
//         .mosi_io_num = LCD_MOSI,
//         .miso_io_num = -1,
//         .sclk_io_num = LCD_SCLK,
//         .max_transfer_sz = 128
//     };  
//     spi_bus_initialize(LCD_SPI_HOST, &bus, SPI_DMA_CH_AUTO);

//     spi_device_interface_config_t dev = {
//         .clock_speed_hz = 2 * 1000 * 1000,
//         .mode = 0,
//         .spics_io_num = LCD_CS,
//         .queue_size = 4
//     };
//     spi_bus_add_device(LCD_SPI_HOST, &dev, &lcd);

//     /* EA‑DOGS104 power‑up sequence */
//     const uint8_t seq[] = {
//         0x3A, 0x09, 0x06, 0x1E,
//         0x39, 0x1B, 0x6E, 0x56, 0x7A,
//         0x38
//     };
//     for (size_t i = 0; i < sizeof(seq); i++) lcd_cmd(seq[i]);

//     lcd_cmd(0x0F);      /* display ON, cursor/blink OFF */
//     lcd_clear();
// }


/* -------------------------------------------------------------------------- */
/*  Wi‑Fi helper (event‑group & handler)                                      */
/* -------------------------------------------------------------------------- */
static EventGroupHandle_t wifi_event_group;
#define WIFI_CONNECTED_BIT BIT0

static void wifi_event_handler(void *arg,
                               esp_event_base_t event_base,
                               int32_t event_id,
                               void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        ESP_LOGW(TAG, "Wi‑Fi disconnected — retrying…");
        esp_wifi_connect();
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
        ip_event_got_ip_t *e = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG, "✅ Got IP: " IPSTR, IP2STR(&e->ip_info.ip));
    }
}

static void wifi_init_sta(void)
{
    wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_NONE));          // keep RF on for TLS

    wifi_config_t sta_cfg = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS,
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
            .pmf_cfg = {.capable = true, .required = false},
        }
    };

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &sta_cfg));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(
                        WIFI_EVENT, ESP_EVENT_ANY_ID,
                        &wifi_event_handler, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
                        IP_EVENT, IP_EVENT_STA_GOT_IP,
                        &wifi_event_handler, NULL, NULL));

    ESP_ERROR_CHECK(esp_wifi_start());

    /* wait max 10 s until connected */
    EventBits_t bits = xEventGroupWaitBits(wifi_event_group,
                                           WIFI_CONNECTED_BIT,
                                           pdFALSE, pdFALSE,
                                           pdMS_TO_TICKS(10000));
    if (!(bits & WIFI_CONNECTED_BIT)) {
        ESP_LOGE(TAG, "❌ Wi‑Fi connection timed out");
    }
}

/* -------------------------------------------------------------------------- */
/*  GPIO helpers                                                              */
/* -------------------------------------------------------------------------- */
static void gpio_init_buttons(void)
{
    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pin_bit_mask = (1ULL << BTN_A) | (1ULL << BTN_B) |
                        (1ULL << BTN_C) | (1ULL << BTN_D) |
                        (1ULL << BTN_E)
    };
    gpio_config(&io_conf);
}

static char detect_answer_button(void)
{
    if (!gpio_get_level(BTN_A)) return 'A';
    if (!gpio_get_level(BTN_B)) return 'B';
    if (!gpio_get_level(BTN_C)) return 'C';
    if (!gpio_get_level(BTN_D)) return 'D';
    if (!gpio_get_level(BTN_E)) return 'E';
    return 0;
}

/* -------------------------------------------------------------------------- */
/*  Firebase helpers                                                          */
/* -------------------------------------------------------------------------- */
static char *resp_buf   = NULL;
static size_t resp_cap  = 0;
static size_t resp_used = 0;

static esp_err_t http_evt(esp_http_client_event_t *evt)
{
    switch (evt->event_id) {
    case HTTP_EVENT_ON_DATA:
        if (!resp_buf) {                       // first chunk → allocate
            resp_cap  = evt->data_len + 1;
            resp_buf  = malloc(resp_cap);
            resp_used = 0;
        }
        /* grow the buffer if needed */
        while (resp_used + evt->data_len + 1 > resp_cap) {
            resp_cap *= 2;
            resp_buf  = realloc(resp_buf, resp_cap);
        }
        memcpy(resp_buf + resp_used, evt->data, evt->data_len);
        resp_used += evt->data_len;
        break;

    case HTTP_EVENT_ON_FINISH:
        if (resp_buf) resp_buf[resp_used] = 0; // NUL‑terminate
        break;
    default:
        break;
    }
    return ESP_OK;
}



static bool fetch_current_prompt(Prompt *prompt)
{
    esp_http_client_config_t cfg = {
        .url = FIREBASE_CURRENT_PROMPT,
        .transport_type = HTTP_TRANSPORT_OVER_SSL,
        .crt_bundle_attach = esp_crt_bundle_attach,   // ➌ verify with bundle
        .timeout_ms = 10000,
        .event_handler    = http_evt
    };

    resp_buf = NULL;   /* reset global scratch buffer */
    resp_cap = resp_used = 0;
    esp_http_client_handle_t client = esp_http_client_init(&cfg);

    esp_http_client_set_header(client, "Accept-Encoding", "identity");


    esp_err_t err = esp_http_client_perform(client);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to fetch prompt: %s", esp_err_to_name(err));
        esp_http_client_cleanup(client);
        return false;
    }

    int status = esp_http_client_get_status_code(client);
    ESP_LOGI(TAG, "HTTP status: %d", status);
    if (status != 200) {
        ESP_LOGE(TAG, "Unexpected status %d ‑ aborting", status);
        esp_http_client_cleanup(client);
        return false;
    }

    int len = esp_http_client_get_content_length(client);
    if (len <= 0) {
        ESP_LOGE(TAG, "Prompt length 0");
        esp_http_client_cleanup(client);
        return false;
    }

    

    ESP_ERROR_CHECK_WITHOUT_ABORT(esp_http_client_perform(client));

    if (!resp_buf || resp_used == 0) {
        ESP_LOGE(TAG, "Empty body");
        esp_http_client_cleanup(client);
        return false;
    }

    // ESP_LOGI(TAG, "Fetched JSON (%zu bytes): %s", resp_used, resp_buf);
    cJSON *json = cJSON_Parse(resp_buf);
    free(resp_buf);                        // done with the scratch buffer
    esp_http_client_cleanup(client);


    if (!json) {
        ESP_LOGE(TAG, "Prompt JSON parse error");
        return false;
    }

    strncpy(prompt->question, cJSON_GetObjectItem(json, "question")->valuestring, sizeof(prompt->question));
    strncpy(prompt->subject,  cJSON_GetObjectItem(json, "subject")->valuestring,  sizeof(prompt->subject));
    strncpy(prompt->correctAnswer, cJSON_GetObjectItem(json, "correctAnswer")->valuestring, sizeof(prompt->correctAnswer));

    cJSON_Delete(json);
    return true;
}

static void send_to_firebase(int emotion, char selectedAnswer)
{
    Prompt prompt;
    if (!fetch_current_prompt(&prompt)) {
        ESP_LOGW(TAG, "Using fallback prompt");
        strcpy(prompt.question, "Unknown");
        strcpy(prompt.subject,  "Unknown");
        strcpy(prompt.correctAnswer, "Z");
    }

    char url[256];
    snprintf(url, sizeof(url), FIREBASE_RESPONSES_PATH, STUDENT_ID);

    // time_t now; time(&now);

    cJSON *json = cJSON_CreateObject();
    // cJSON_AddNumberToObject(json, "timestamp", now);
    int64_t now_ms = (int64_t)time(NULL) * 1000;   // seconds → ms
    cJSON_AddNumberToObject(json, "timestamp", now_ms);

    cJSON_AddStringToObject(json, "rfid", RFID_TAG);
    cJSON_AddNumberToObject(json, "emotion", emotion);

    char answerStr[2] = { selectedAnswer, '\0' };
    cJSON_AddStringToObject(json, "selectedAnswer", answerStr);

    cJSON_AddStringToObject(json, "question", prompt.question);
    cJSON_AddStringToObject(json, "subject",  prompt.subject);
    cJSON_AddStringToObject(json, "correctAnswer", prompt.correctAnswer);
    cJSON_AddBoolToObject(json, "isCorrect", selectedAnswer == prompt.correctAnswer[0]);

    char *post_data = cJSON_PrintUnformatted(json);

    esp_http_client_config_t cfg = {
        .url = url,
        .transport_type = HTTP_TRANSPORT_OVER_SSL,
        .crt_bundle_attach = esp_crt_bundle_attach,   // ➌ verify with bundle
        .timeout_ms = 10000,
    };
    esp_http_client_handle_t client = esp_http_client_init(&cfg);
    esp_http_client_set_method(client, HTTP_METHOD_POST);
    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_post_field(client, post_data, strlen(post_data));

    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "✅ Data sent to Firebase.");
    } else {
        ESP_LOGE(TAG, "❌ Error sending data: %s", esp_err_to_name(err));
    }

    /* ✳ NEW – update LCD with latest values */
    // lcd_update_screen(STUDENT_NAME, selectedAnswer, emotion);

    esp_http_client_cleanup(client);
    cJSON_Delete(json);
    free(post_data);
}

/* -------------------------------------------------------------------------- */
/*  Main                                                                      */
/* -------------------------------------------------------------------------- */
void app_main(void)
{
    ESP_ERROR_CHECK(nvs_flash_init());

    // lcd_init();
    // lcd_set_contrast(0x23, 0x28);   // was 0x26 → try 0x28
    // draw_str(0, "Hello Ethan Ge");

    // /* -------- HAL pin mapping ---------- */
    // u8g2_esp32_hal_t cfg = U8G2_ESP32_HAL_DEFAULT;
    // cfg.bus.spi.clk   = PIN_SCLK;
    // cfg.bus.spi.mosi  = PIN_MOSI;
    // cfg.bus.spi.cs    = PIN_CS;
    // cfg.dc    = PIN_DC;
    // cfg.reset = PIN_RST;
    // u8g2_esp32_hal_init(cfg);

    // /* -------- Choose the right constructor ---------- *
    //  * UC1701 == controller used on EA-DOGS104-A.
    //  * The library has a ready-made setup for it:       */
    // u8g2_t u8g2;
    // u8g2_Setup_uc1701_ea_dogs102_f(
    //     &u8g2, U8G2_R0,                       // no rotation
    //     u8g2_esp32_spi_byte_cb,               // SPI byte callback
    //     u8g2_esp32_gpio_and_delay_cb);        // GPIO & delays

    // /* -------- Power-up sequence ---------- */
    // ESP_LOGI(LCD_TAG, "Init display");
    // u8g2_InitDisplay(&u8g2);      // sends the UC1701 init sequence
    // u8g2_SetPowerSave(&u8g2, 0);  // wake up
    // u8g2_ClearBuffer(&u8g2);

    // /* -------- Draw something ---------- */
    // // u8g2_SetFont(&u8g2, u8g2_font_6x10_tf);   // any built-in font
    // // u8g2_DrawStr(&u8g2, 0, 12, "Hello Ethan Ge");

    // u8g2_SetDrawColor(&u8g2, 1);                 // 1 = set pixels (black)
    // u8g2_DrawBox(&u8g2, 0, 0,                    // x, y
    // u8g2_GetDisplayWidth(&u8g2),    // full width  (104 px on DOGS104)
    // u8g2_GetDisplayHeight(&u8g2));  // full height ( 64 px)

    // /* -------- Send to LCD ---------- */
    // u8g2_SendBuffer(&u8g2);        // <= nothing shows until you call this!

    /* done */
    // vTaskDelete(NULL);

    wifi_init_sta();                       // returns after IP ready
    obtain_time();            // blocks until time acquired
    gpio_init_buttons();

    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(POT_CHANNEL, ADC_ATTEN_DB_11);

    gpio_num_t pot_gpio;
    if (adc1_pad_get_io_num(POT_CHANNEL, &pot_gpio) == ESP_OK) {
        ESP_LOGI(TAG, "Reading potentiometer on GPIO%d (ADC1_CH%d)", pot_gpio, POT_CHANNEL);
    } else {
        ESP_LOGE(TAG, "adc1_pad_get_io_num failed");
    }


    // or poll buttons continuously
    while (1) {
        char ans = detect_answer_button();
        if (ans) {
            // int emotion = 5;  // read potentiometer here

            

            int raw = adc1_get_raw(POT_CHANNEL);

            // 2) map to 1–10
            // raw: 0…4095 → mapped: 0…9
            int mapped = (raw * 10) / 4096; 
            // invert so 0→10, 9→1
            int emotion = 10 - mapped;
            // clamp just in case
            if      (emotion < 1)  emotion = 1;
            else if (emotion > 10) emotion = 10;
            ESP_LOGI(TAG, "Raw Pot Value %d", raw);

            ESP_LOGI(TAG, "RFID: %s | Answer: %c | Emotion: %d", RFID_TAG, ans, emotion);
            send_to_firebase(emotion, ans);

            vTaskDelay(pdMS_TO_TICKS(2000));
        } else {
            vTaskDelay(pdMS_TO_TICKS(100));
        }
    }
    
}
