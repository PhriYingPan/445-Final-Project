


// #include <stdio.h>
// #include "freertos/FreeRTOS.h"
// #include "freertos/task.h"
// #include "driver/gpio.h"
// #include "esp_log.h"

// // #define SWITCH1 4
// #define SWITCH2 5
// #define SWITCH3 6

// static const char *TAG = "SWITCH_MONITOR";

// void app_main(void) {
//     // Configure GPIOs as inputs
//     // gpio_reset_pin(SWITCH1);
//     gpio_reset_pin(SWITCH2);
//     gpio_reset_pin(SWITCH3);

//     // gpio_set_direction(SWITCH1, GPIO_MODE_INPUT);
//     gpio_set_direction(SWITCH2, GPIO_MODE_INPUT);
//     gpio_set_direction(SWITCH3, GPIO_MODE_INPUT);

//     while (1) {
//         // Read switch states
//         // int state1 = gpio_get_level(SWITCH1);
//         int state2 = gpio_get_level(SWITCH2);
//         int state3 = gpio_get_level(SWITCH3);

//         // Print the switch states to Serial Monitor
//         ESP_LOGI(TAG, "Switch States: %d, %d", state2, state3);

//         vTaskDelay(pdMS_TO_TICKS(500));  // Delay 500ms before reading again
//     }
// }
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_http_client.h"
#include "esp_wifi.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "cJSON.h"

#include "esp_crt_bundle.h"

/* -------------------------------------------------------------------------- */
/*  Wi‑Fi credentials                                                         */
/* -------------------------------------------------------------------------- */
#define WIFI_SSID "NETGEAR34"
#define WIFI_PASS "silentspider842"

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
#define RFID_TAG   "04A3C2"

/*  Button GPIOs – avoid 6–11 (SPI‑flash) and 34–39 (input‑only)              */
#define BTN_A 5
#define BTN_B 6
#define BTN_C 7
#define BTN_D 15
#define BTN_E 16

static const char *TAG = "ESP32_FIREBASE";



/* -------------------------------------------------------------------------- */
/*  Prompt structure                                                          */
/* -------------------------------------------------------------------------- */
typedef struct {
    char question[128];
    char subject[32];
    char correctAnswer[4];
} Prompt;

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
    if (gpio_get_level(BTN_A)) return 'A';
    if (gpio_get_level(BTN_B)) return 'B';
    if (gpio_get_level(BTN_C)) return 'C';
    if (gpio_get_level(BTN_D)) return 'D';
    if (gpio_get_level(BTN_E)) return 'E';
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

    // char *buf = malloc(len + 1);
    // esp_http_client_read_response(client, buf, len);
    // buf[len] = '\0';

    // ESP_LOGI(TAG, "Fetched currentPrompt raw: %s", buf);   // ← debug print



    // cJSON *json = cJSON_Parse(buf);
    // free(buf);
    // esp_http_client_cleanup(client);
    

    ESP_ERROR_CHECK_WITHOUT_ABORT(esp_http_client_perform(client));

    if (!resp_buf || resp_used == 0) {
        ESP_LOGE(TAG, "Empty body");
        esp_http_client_cleanup(client);
        return false;
    }

    ESP_LOGI(TAG, "Fetched JSON (%zu bytes): %s", resp_used, resp_buf);
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

    time_t now; time(&now);

    cJSON *json = cJSON_CreateObject();
    cJSON_AddNumberToObject(json, "timestamp", now);
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
    wifi_init_sta();                       // returns after IP ready
    gpio_init_buttons();

    /* demo: send one hard‑coded answer */
    char selected = 'C';
    int  emotion  = 5;                     // replace with ADC value mapping

    ESP_LOGI(TAG, "RFID: %s | Answer: %c | Emotion: %d", RFID_TAG, selected, emotion);
    send_to_firebase(emotion, selected);

    /* or poll buttons continuously
    while (1) {
        char ans = detect_answer_button();
        if (ans) {
            int emotion = 5;  // read potentiometer here
            send_to_firebase(emotion, ans);
            vTaskDelay(pdMS_TO_TICKS(2000));
        } else {
            vTaskDelay(pdMS_TO_TICKS(100));
        }
    }
    */
}
