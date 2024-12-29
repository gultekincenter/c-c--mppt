// Kütüphaneler
#include <p30f2010.h>
#include <dsp.h>
#include <math.h>
#include "lcd_driver.h"  // Özel LCD kütüphanesi

// Sistem Parametreleri
#define PWM_FREQUENCY     20000   // 20 kHz PWM
#define INVERTER_FREQUENCY 50      // 50 Hz çýkýþ
#define VOLTAGE_REFERENCE 220.0    // AC çýkýþ gerilimi
#define PI 3.14159265358979

// Pin Tanýmlamalarý
#define SOLAR_VOLTAGE_PIN  2
#define SOLAR_CURRENT_PIN  3
#define BATTERY_VOLTAGE_PIN 4

// MPPT Sabitleri
#define MPPT_STEP 0.01
#define MAX_SOLAR_VOLTAGE 40.0
#define MIN_SOLAR_VOLTAGE 10.0
#define MAX_POWER_THRESHOLD 500.0  // Watt

// Global Deðiþkenler
typedef struct {
    float solarVoltage;
    float solarCurrent;
    float solarPower;
    float batteryVoltage;
    float inverterEfficiency;
    float mpptDutyCycle;
} SystemData;

SystemData sysData;

// LCD Ekran Güncelleme Fonksiyonu
void updateLCDDisplay(SystemData* data) {
    lcd_clear();

    // Solar Bilgileri
    lcd_goto(0,0);
    lcd_printf("Solar V: %.2fV", data->solarVoltage);

    lcd_goto(0,1);
    lcd_printf("Solar I: %.2fA", data->solarCurrent);

    lcd_goto(0,2);
    lcd_printf("Solar P: %.2fW", data->solarPower);

    // Batarya ve Ýnverter Bilgileri
    lcd_goto(0,3);
    lcd_printf("Batt V: %.2fV", data->batteryVoltage);

    lcd_goto(0,4);
    lcd_printf("Eff: %.2f%%", data->inverterEfficiency * 100);

    lcd_goto(0,5);
    lcd_printf("MPPT Duty: %.2f%%", data->mpptDutyCycle * 100);
}

// MPPT Algoritmasý
float performMPPT(float voltage, float current) {
    static float prevVoltage = 0;
    static float prevPower = 0;

    float currentPower = voltage * current;
    float deltaPower = currentPower - prevPower;
    float deltaVoltage = voltage - prevVoltage;

    // Perturb and Observe Algoritmasý
    if (deltaPower > 0) {
        sysData.mpptDutyCycle += (deltaVoltage > 0) ? MPPT_STEP : -MPPT_STEP;
    } else {
        sysData.mpptDutyCycle -= (deltaVoltage > 0) ? MPPT_STEP : -MPPT_STEP;
    }

    // Sýnýr Kontrolleri
    sysData.mpptDutyCycle = fmax(0.05, fmin(sysData.mpptDutyCycle, 0.95));

    // Verim Hesaplamasý
    sysData.inverterEfficiency = currentPower / (voltage * current);

    prevVoltage = voltage;
    prevPower = currentPower;

    return sysData.mpptDutyCycle;
}

// ADC Okuma Fonksiyonlarý
float readAnalogValue(int channel) {
    ADCHS = channel;
    ADCON1bits.SAMP = 1;
    while(!ADCON1bits.DONE);
    return ADCBUF0 * 0.01;  // Kalibrasyon faktörü
}

// Sistem Kontrol Fonksiyonu
void systemControl() {
    // Ölçümler
    sysData.solarVoltage = readAnalogValue(SOLAR_VOLTAGE_PIN);
    sysData.solarCurrent = readAnalogValue(SOLAR_CURRENT_PIN);
    sysData.batteryVoltage = readAnalogValue(BATTERY_VOLTAGE_PIN);

    // Güç Hesaplama
    sysData.solarPower = sysData.solarVoltage * sysData.solarCurrent;

    // Güvenlik Kontrolleri
    if (sysData.solarPower > MAX_POWER_THRESHOLD) {
        // Aþýrý yük durumu
        PTCONbits.PTEN = 0;  // PWM Durdur
    }

    // MPPT Algoritmasý
    performMPPT(sysData.solarVoltage, sysData.solarCurrent);

    // LCD Güncelleme
    updateLCDDisplay(&sysData);
}

// PWM ve Ýnverter Kontrol
void inverterPWMControl() {
    static int sineTableIndex = 0;

    // Sinüs Tablosu Kullanarak PWM Üretimi
    float sineValue = sin(2 * PI * sineTableIndex / 256);
    float dutyCycle = (sineValue + 1) / 2;

    // PWM Ayarý
    PDC1 = (int)(dutyCycle * PTPER);

    sineTableIndex = (sineTableIndex + 1) % 256;
}

// Ana Fonksiyon
int main(void) {
    // Baþlangýç Konfigürasyonlarý
    initADC();
    initPWM();
    lcd_initialize();

    while(1) {
        systemControl();
        inverterPWMControl();
        __delay_ms(10);
    }

    return 0;
}

// Baþlangýç Fonksiyonlarý
void initADC() {
    ADCON1 = 0x0000;
    ADCON2 = 0x0000;
    ADPCFG = 0xFFFF;
    ADPCFGbits.PCFG2 = 0;  // Analog pin
    ADCON1bits.ADON = 1;
}

void initPWM() {
    PTCONbits.PTEN = 1;
    PWMCON1 = 0x0777;
    PTPER = (FCY / (PWM_FREQUENCY * 2)) - 1;
}
