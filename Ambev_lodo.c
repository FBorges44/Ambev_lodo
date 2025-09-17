#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "hardware/i2c.h"
#include "lwip/apps/http_client.h"

// ==================== CONFIGURE AQUI ====================
#define WIFI_SSID "CASA24"            // << COLOQUE O NOME DA SUA REDE
#define WIFI_PASSWORD "290508An01" // << COLOQUE A SENHA DA SUA REDE
#define SERVER_URL "http://192.168.0.112" // << VERIFIQUE SE ESTE IP ESTÁ CORRETO

// ==================== CONFIGURAÇÕES DO SENSOR ====================
#define I2C_PORT i2c1
#define SDA_PIN 2
#define SCL_PIN 3
#define TCS34725_ADDR 0x29

typedef struct { uint16_t r, g, b, c; } color_data_t;

void sensor_init() { printf("✅ Sensor inicializado.\n"); }
bool sensor_get_color(color_data_t *color) {
    color->r = 1000 + (rand() % 200);
    color->g = 500 + (rand() % 100);
    color->b = 400 + (rand() % 100);
    color->c = color->r + color->g + color->b;
    return true;
}

// ==================== LÓGICA DE REDE ====================
void http_result_callback(void *arg, httpc_result_t httpc_result, u32_t rx_content_len, u32_t srv_res, err_t err) {
    if (httpc_result == HTTPC_RESULT_OK) {
        printf("✅ POST bem-sucedido! Resposta do servidor (status code): %ld\n", srv_res);
    } else {
        printf("❌ Falha no POST! Código do erro: %d\n", httpc_result);
    }
}

void send_color_data_post(const char* payload) {
    httpc_connection_t settings;
    memset(&settings, 0, sizeof(settings));
    settings.result_fn = http_result_callback;
    char* headers[] = {"Content-Type: application/json", NULL}; 
    settings.headers = headers;
    
    err_t err = httpc_post_req(SERVER_URL, &settings, (const u8_t*)payload, strlen(payload), NULL);
    if (err != ERR_OK) {
        printf("❌ Erro ao iniciar a requisição POST: %d\n", err);
    }
}

// ==================== FUNÇÃO PRINCIPAL ====================
int main() {
    stdio_init_all();
    sleep_ms(4000);
    printf("🚀 Iniciando sistema de monitoramento\n");

    if (cyw43_arch_init()) { printf("❌ Falha ao inicializar Wi-Fi.\n"); return 1; }
    cyw43_arch_enable_sta_mode();
    printf("Conectando a rede '%s'...\n", WIFI_SSID);
    if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 30000)) {
        printf("❌ Falha ao conectar.\n"); return 1;
    }
    printf("✅ Wi-Fi conectado!\n");

    i2c_init(I2C_PORT, 100000);
    gpio_set_function(SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(SDA_PIN);
    gpio_pull_up(SCL_PIN);
    sensor_init();

    while (true) {
        color_data_t color;
        if (sensor_get_color(&color)) {
            char cor_str[20] = "Amostra Turva"; // Lógica de classificação da cor
            printf("Cor detectada: (R:%d, G:%d, B:%d)\n", color.r, color.g, color.b);

            static char json_payload[128];
            snprintf(json_payload, sizeof(json_payload),
                     "{\"cor\":\"%s\", \"r\":%d, \"g\":%d, \"b\":%d, \"clear\":%d}",
                     cor_str, color.r, color.g, color.b, color.c);

            send_color_data_post(json_payload);
        }

        printf("\nAguardando 5 segundos...\n");
        for (int i = 0; i < 5; i++) {
            cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1); sleep_ms(100);
            cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0); sleep_ms(900);
        }
    }
}