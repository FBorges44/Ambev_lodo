#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"

// ==================== CONFIGURAÇÕES I2C ====================
#define I2C_PORT i2c1
#define SDA_PIN 2
#define SCL_PIN 3
#define I2C_FREQ 100000  // 100kHz

// ==================== TCS34725 DEFINIÇÕES ====================
#define TCS34725_ADDR        0x29
#define TCS34725_COMMAND_BIT 0x80

// Registradores
#define TCS34725_ENABLE      0x00
#define TCS34725_ATIME       0x01
#define TCS34725_CONTROL     0x0F
#define TCS34725_ID          0x12
#define TCS34725_STATUS      0x13
#define TCS34725_CDATAL      0x14
#define TCS34725_CDATAH      0x15
#define TCS34725_RDATAL      0x16
#define TCS34725_RDATAH      0x17
#define TCS34725_GDATAL      0x18
#define TCS34725_GDATAH      0x19
#define TCS34725_BDATAL      0x1A
#define TCS34725_BDATAH      0x1B

// Bits de controle ENABLE
#define TCS34725_ENABLE_AIEN 0x10  // Interrupt enable
#define TCS34725_ENABLE_WEN  0x08  // Wait enable
#define TCS34725_ENABLE_AEN  0x02  // ADC enable
#define TCS34725_ENABLE_PON  0x01  // Power on

// Ganhos disponíveis
#define TCS34725_GAIN_1X     0x00
#define TCS34725_GAIN_4X     0x01
#define TCS34725_GAIN_16X    0x02
#define TCS34725_GAIN_60X    0x03

// Tempos de integração
#define TCS34725_INTEGRATIONTIME_2_4MS  0xFF  // 2.4ms
#define TCS34725_INTEGRATIONTIME_24MS   0xF6  // 24ms
#define TCS34725_INTEGRATIONTIME_101MS  0xD5  // 101ms
#define TCS34725_INTEGRATIONTIME_154MS  0xC0  // 154ms
#define TCS34725_INTEGRATIONTIME_700MS  0x00  // 700ms

// ==================== ESTRUTURA PARA DADOS ====================
typedef struct {
    uint16_t clear;
    uint16_t red;
    uint16_t green;
    uint16_t blue;
    bool valid;
} color_data_t;

// ==================== FUNÇÕES DE BAIXO NÍVEL ====================

// Escreve um byte em um registrador
bool tcs34725_write_register(uint8_t reg, uint8_t value) {
    uint8_t buf[2] = {TCS34725_COMMAND_BIT | reg, value};
    int result = i2c_write_blocking(I2C_PORT, TCS34725_ADDR, buf, 2, false);
    return result == 2;
}

// Lê um byte de um registrador
bool tcs34725_read_register(uint8_t reg, uint8_t *value) {
    uint8_t cmd = TCS34725_COMMAND_BIT | reg;
    
    // Escreve o comando
    int result = i2c_write_blocking(I2C_PORT, TCS34725_ADDR, &cmd, 1, true);
    if (result != 1) return false;
    
    // Lê o resultado
    result = i2c_read_blocking(I2C_PORT, TCS34725_ADDR, value, 1, false);
    return result == 1;
}

// Lê um valor de 16 bits (little-endian)
bool tcs34725_read_16bit(uint8_t reg, uint16_t *value) {
    uint8_t buf[2];
    uint8_t cmd = TCS34725_COMMAND_BIT | reg;
    
    // Escreve o comando
    int result = i2c_write_blocking(I2C_PORT, TCS34725_ADDR, &cmd, 1, true);
    if (result != 1) return false;
    
    // Lê 2 bytes
    result = i2c_read_blocking(I2C_PORT, TCS34725_ADDR, buf, 2, false);
    if (result != 2) return false;
    
    *value = (uint16_t)(buf[0] | (buf[1] << 8));  // Little-endian
    return true;
}

// ==================== FUNÇÕES DE DEBUG ====================

// Scanner I2C para encontrar dispositivos
void i2c_scanner() {
    printf("\n🔍 Escaneando barramento I2C...\n");
    printf("   ");
    for (int addr = 0; addr < 16; addr++) {
        printf("%3x", addr);
    }
    printf("\n");

    bool found = false;
    for (int addr = 0; addr < 128; addr += 16) {
        printf("%02x:", addr);
        for (int i = 0; i < 16; i++) {
            int address = addr + i;
            if (address < 8 || address > 119) {
                printf("   ");
                continue;
            }

            uint8_t dummy;
            int result = i2c_read_blocking(I2C_PORT, address, &dummy, 1, false);
            if (result >= 0) {
                printf(" %02x", address);
                found = true;
                if (address == TCS34725_ADDR) {
                    printf("*");  // Marca o TCS34725
                } else {
                    printf(" ");
                }
            } else {
                printf(" --");
            }
        }
        printf("\n");
    }
    
    if (!found) {
        printf("❌ Nenhum dispositivo I2C encontrado!\n");
    } else {
        printf("✅ Dispositivos encontrados (TCS34725 marcado com *)\n");
    }
    printf("\n");
}

// Verifica se o sensor está presente e funcionando
bool tcs34725_test_connection() {
    printf("🔗 Testando conexão com TCS34725...\n");
    
    uint8_t id;
    if (!tcs34725_read_register(TCS34725_ID, &id)) {
        printf("❌ Falha na comunicação I2C\n");
        return false;
    }
    
    printf("📋 ID do sensor: 0x%02X ", id);
    
    // IDs válidos para TCS34725: 0x44, 0x4D
    if (id == 0x44 || id == 0x4D) {
        printf("(TCS34725 válido ✅)\n");
        return true;
    } else {
        printf("(ID inválido ❌)\n");
        return false;
    }
}

// ==================== FUNÇÕES PRINCIPAIS DO SENSOR ====================

// Inicializa o sensor
bool tcs34725_init() {
    printf("⚙️  Inicializando TCS34725...\n");
    
    // 1. Power ON
    if (!tcs34725_write_register(TCS34725_ENABLE, TCS34725_ENABLE_PON)) {
        printf("❌ Falha ao ligar o sensor\n");
        return false;
    }
    sleep_ms(3);  // Aguarda estabilizar
    
    // 2. Enable ADC
    if (!tcs34725_write_register(TCS34725_ENABLE, 
                                TCS34725_ENABLE_PON | TCS34725_ENABLE_AEN)) {
        printf("❌ Falha ao ativar ADC\n");
        return false;
    }
    
    // 3. Configura tempo de integração (101ms - bom equilíbrio)
    if (!tcs34725_write_register(TCS34725_ATIME, TCS34725_INTEGRATIONTIME_101MS)) {
        printf("❌ Falha ao configurar tempo de integração\n");
        return false;
    }
    
    // 4. Configura ganho (4x - bom para maioria das situações)
    if (!tcs34725_write_register(TCS34725_CONTROL, TCS34725_GAIN_4X)) {
        printf("❌ Falha ao configurar ganho\n");
        return false;
    }
    
    sleep_ms(120);  // Aguarda primeira conversão (101ms + margem)
    
    printf("✅ TCS34725 inicializado com sucesso!\n");
    printf("   - Tempo de integração: 101ms\n");
    printf("   - Ganho: 4x\n\n");
    
    return true;
}

// Verifica se há dados disponíveis
bool tcs34725_data_ready() {
    uint8_t status;
    if (!tcs34725_read_register(TCS34725_STATUS, &status)) {
        return false;
    }
    return (status & 0x01) != 0;  // Bit 0 = AVALID
}

// Lê os dados de cor
bool tcs34725_read_color(color_data_t *data) {
    // Verifica se dados estão prontos
    if (!tcs34725_data_ready()) {
        data->valid = false;
        return false;
    }
    
    // Lê os 4 canais
    bool success = true;
    success &= tcs34725_read_16bit(TCS34725_CDATAL, &data->clear);
    success &= tcs34725_read_16bit(TCS34725_RDATAL, &data->red);
    success &= tcs34725_read_16bit(TCS34725_GDATAL, &data->green);
    success &= tcs34725_read_16bit(TCS34725_BDATAL, &data->blue);
    
    data->valid = success;
    return success;
}

// Converte para RGB normalizado (0-255)
void color_to_rgb(color_data_t *color, uint8_t *r, uint8_t *g, uint8_t *b) {
    if (!color->valid || color->clear == 0) {
        *r = *g = *b = 0;
        return;
    }
    
    // Normaliza baseado no canal clear
    *r = (uint8_t)((color->red * 255) / color->clear);
    *g = (uint8_t)((color->green * 255) / color->clear);
    *b = (uint8_t)((color->blue * 255) / color->clear);
}

// ==================== FUNÇÃO PRINCIPAL ====================

int main() {
    stdio_init_all();
    sleep_ms(2000);  // Aguarda terminal USB conectar
    
    printf("\n==================================================\n");
    printf("🎨 SENSOR DE COR TCS34725 - RASPBERRY PI PICO W\n");
    printf("==================================================\n\n");
    
    // 1. Inicializa I2C
    printf("🔧 Configurando I2C...\n");
    i2c_init(I2C_PORT, I2C_FREQ);
    gpio_set_function(SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(SDA_PIN);
    gpio_pull_up(SCL_PIN);
    printf("✅ I2C configurado: SDA=GP%d, SCL=GP%d, %dkHz\n\n", 
           SDA_PIN, SCL_PIN, I2C_FREQ/1000);
    
    // 2. Scanner I2C (debug)
    i2c_scanner();
    
    // 3. Testa conexão com sensor
    if (!tcs34725_test_connection()) {
        printf("💥 ERRO: TCS34725 não encontrado!\n");
        printf("   Verifique as conexões:\n");
        printf("   - VCC → 3.3V\n");
        printf("   - GND → GND\n");
        printf("   - SDA → GP14\n");
        printf("   - SCL → GP15\n\n");
        while (true) {
            printf("❌ Sensor não detectado. Reinicie após verificar conexões.\n");
            sleep_ms(5000);
        }
    }
    
    // 4. Inicializa sensor
    if (!tcs34725_init()) {
        printf("💥 ERRO: Falha na inicialização do sensor!\n");
        while (true) {
            printf("❌ Falha na inicialização. Reinicie o sistema.\n");
            sleep_ms(5000);
        }
    }
    
    // 5. Loop principal de leitura
    printf("🎯 Iniciando leituras de cor (Ctrl+C para parar):\n");
    printf("---------------------------------------------------\n");
    
    color_data_t color_data;
    uint8_t r, g, b;
    int reading_count = 0;
    
    while (true) {
        if (tcs34725_read_color(&color_data)) {
            reading_count++;
            
            // Converte para RGB normalizado
            color_to_rgb(&color_data, &r, &g, &b);
            
            // Exibe dados brutos
            printf("%04d | Raw: C=%5u R=%5u G=%5u B=%5u | ", 
                   reading_count, color_data.clear, color_data.red, 
                   color_data.green, color_data.blue);
            
            // Exibe RGB normalizado
            printf("RGB: %3u,%3u,%3u | ", r, g, b);
            
            // Detecta cor dominante
            if (color_data.clear > 100) {  // Só analisa se há luz suficiente
                if (r > g && r > b && r > 80) {
                    printf("🔴 VERMELHO");
                } else if (g > r && g > b && g > 80) {
                    printf("🟢 VERDE");
                } else if (b > r && b > g && b > 80) {
                    printf("🔵 AZUL");
                } else if (r > 150 && g > 150 && b > 150) {
                    printf("⚪ BRANCO");
                } else if (r < 50 && g < 50 && b < 50) {
                    printf("⚫ PRETO");
                } else {
                    printf("🎨 MISTO");
                }
            } else {
                printf("🌙 ESCURO");
            }
            
            printf("\n");
            
        } else {
            printf("⚠️  Aguardando dados do sensor...\n");
        }
        
        sleep_ms(500);  // Leitura a cada 500ms
    }
    
    return 0;
}