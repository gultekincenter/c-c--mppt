#include <p30f2010.h>
#include <libpic30.h>
#include <math.h>
#include "nrf24l01.h"
#include "lcd.h"

// Sistem Sabitleri
#define PWM_FREQUENCY         20000
#define INVERTER_FREQUENCY    50
#define VOLTAGE_REFERENCE     220.0
#define PI                    3.14159

// MPPT Parametreleri
#define MPPT_STEP             0.01
#define MAX_VOLTAGE           40.0
#define MIN_VOLTAGE           10.0

// Pin Tanýmlamalarý
#define SOLAR_VOLTAGE_PIN     AN2
#define SOLAR_CURRENT_PIN     AN3
#define BATTERY_VOLTAGE_PIN   AN4
#define TEMPERATURE_PIN       AN5

// Güvenlik Eþikleri
#define MAX_TEMPERATURE       75.0
#define MAX_POWER_THRESHOLD   500.0
#define MIN_BATTERY_VOLTAGE   10.5
#define MAX_BATTERY_VOLTAGE   14.5

// Kablosuz Haberleþme Yapýsý
typedef struct {
    float solarVoltage;
    float solarCurrent;
    float batteryVoltage;
    float temperature;
    uint8_t systemStatus;
} WirelessData;

// Sistem Parametreleri Yapýsý
typedef struct {
    float solarVoltage;
    float solarCurrent;
    float solarPower;
    float batteryVoltage;
    float temperature;
    float inverterEfficiency;
    float mpptDutyCycle;
    uint8_t systemStatus;
} SystemParameters;

// Global Deðiþkenler
SystemParameters sysParams;
WirelessData wirelessData;
NRF24L01 nrfModule;
volatile uint16_t errorFlags = 0;

// Hata Bayraklarý
enum ErrorFlags {
    OVER_TEMPERATURE = 0x01,
    LOW_BATTERY      = 0x02,
    HIGH_BATTERY     = 0x04,
    OVERLOAD         = 0x08
};

// MPPT Algoritmasý (Geliþmiþ Perturb and Observe)
float performAdvancedMPPT(float voltage, float current) {
    static float prevVoltage = 0;
    static float prevPower = 0;
    static float stepSize = MPPT_STEP;

    float currentPower = voltage * current;
    float deltaPower = currentPower - prevPower;
    float deltaVoltage = voltage - prevVoltage;

    // Geliþmiþ Perturb and Observe
    if (deltaPower > 0) {
        // Pozitif yönde güç artýþý
        stepSize = (deltaVoltage > 0) ? stepSize * 1.1 : stepSize * 0.9;
        sysParams.mpptDutyCycle += (deltaVoltage > 0) ? stepSize : -stepSize;
    } else {
        // Negatif yönde güç azalýþý
        stepSize = (deltaVoltage > 0) ? stepSize * 0.9 : stepSize * 1.1;
        sysParams.mpptDutyCycle -= (deltaVoltage > 0) ? stepSize : -stepSize;
    }

    // Sýnýr Kontrolleri
    sysParams.mpptDutyCycle = fmax(0.05, fmin(sysParams.mpptDutyCycle, 0.95));

    // Verim Hesaplama
    sysParams.inverterEfficiency = currentPower / (voltage * current);

    prevVoltage = voltage;
    prevPower = currentPower;

    return sysParams.mpptDutyCycle;
}

// Güvenlik Kontrolleri
void performSafetyChecks() {
    // Sýcaklýk Kontrolü
    if (sysParams.temperature > MAX_TEMPERATURE) {
        errorFlags |= OVER_TEMPERATURE;
        PTCONbits.PTEN = 0; // PWM Durdur
    }

    // Batarya Gerilim Kontrolleri
    if (sysParams.batteryVoltage < MIN_BATTERY_VOLTAGE) {
        errorFlags |= LOW_BATTERY;
    }

    if (sysParams.batteryVoltage > MAX_BATTERY_VOLTAGE) {
        errorFlags |= HIGH_BATTERY;
    }

    // Güç Aþýmý Kontrolü
    if (sysParams.solarPower > MAX_POWER_THRESHOLD) {
        errorFlags |= OVERLOAD;
        PTCONbits.PTEN = 0;
    }
}

// Kablosuz Veri Gönderme
void sendWirelessData() {
    wirelessData.solarVoltage = sysParams.solarVoltage;
    wirelessData.solarCurrent = sysParams.solarCurrent;
    wirelessData.batteryVoltage = sysParams.batteryVoltage;
    wirelessData.temperature = sysParams.temperature;
    wirelessData.systemStatus = errorFlags;

    nrf24l01_send((uint8_t*)&wirelessData, sizeof(WirelessData));
}

// LCD Ekran Güncelleme
void updateLCDDisplay() {
    lcd_clear();

    // Solar Bilgileri
    lcd_printf("Sol V: %.2fV\n", sysParams.solarVoltage);
    lcd_printf("Sol I: %.2fA\n", sysParams.solarCurrent);
    lcd_printf("Sol P: %.2fW\n", sysParams.solarPower);

    // Batarya Bilgileri
    lcd_printf("Bat V: %.2fV\n", sysParams.batteryVoltage);

    // Sistem Durumu
    lcd_printf("Durum: %s\n",
        (errorFlags == 0) ? "Normal" : "Hata Var");
}

// Ana Sistem Kontrol Fonksiyonu
void systemControl() {
    // Ölçümler
    sysParams.solarVoltage = readAnalogValue(SOLAR_VOLTAGE_PIN);
    sysParams.solarCurrent = readAnalogValue(SOLAR_CURRENT_PIN);
    sysParams.batteryVoltage = readAnalogValue(BATTERY_VOLTAGE_PIN);
    sysParams.temperature = readTemperature(TEMPERATURE_PIN);

    // Güç Hesaplama
    sysParams.solarPower = sysParams.solarVoltage * sysParams.solarCurrent;

    // MPPT Algoritmasý
    performAdvancedMPPT(sysParams.solarVoltage, sysParams.solarCurrent);

    // Güvenlik Kontrolleri
    performSafetyChecks();

    // LCD Güncelleme
    updateLCDDisplay();

    // Kablosuz Veri Gönderme
    sendWirelessData();
}

// Baþlatma Fonksiyonlarý
void initSystem() {
    // ADC Baþlatma
    ADCInit();

    // PWM Baþlatma
    PWMInit();

    // LCD Baþlatma
    lcd_init();

    // NRF24L01 Baþlatma
    nrf24l01_init(&nrfModule);
}

// Ana Fonksiyon
int main(void) {
    // Sistem Baþlatma
    initSystem();

    while(1) {
        // Sistem Kontrol Döngüsü
        systemControl();

        // PWM Kontrol
        inverterPWMControl();

        // Kýsa Gecikme
        __delay_ms(10);
    }

    return 0;
}
