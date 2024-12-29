// Kütüphaneler ve Tanýmlamalar
#include <p30f2010.h>
#include <dsp.h>
#include <math.h>

// Sistem Parametreleri
#define PWM_FREQUENCY     20000   // 20 kHz PWM frekansý
#define INVERTER_FREQUENCY 50      // 50 Hz çýkýþ frekansý
#define VOLTAGE_REFERENCE 220.0    // AC çýkýþ gerilimi
#define PI 3.14159265358979

// MPPT Sabitleri
#define MPPT_STEP 0.01
#define MAX_VOLTAGE 40.0
#define MIN_VOLTAGE 10.0

// Global Deðiþkenler
volatile float solarVoltage = 0;
volatile float solarCurrent = 0;
volatile float batteryVoltage = 0;
volatile float mpptDutyCycle = 0;

// Sinüs Tablosu Tanýmlamasý
const int SINE_TABLE_SIZE = 256;
volatile int sineTable[256];

// ADC Konfigürasyon Fonksiyonlarý
void initADC() {
    ADCON1 = 0x0000;     // ADC konfigürasyonu
    ADCON2 = 0x0000;     // Örnekleme zamaný ayarlarý
    ADCON3 = 0x0F00;     // Dönüþüm zamaný
    ADCHSbits.CH0SA = 2; // Kanal seçimi
    ADPCFG = 0xFFFF;     // Dijital pine çevir
    ADPCFGbits.PCFG2 = 0; // Analog giriþ
    ADCON1bits.ADON = 1; // ADC'yi etkinleþtir
}

// PWM Konfigürasyon Fonksiyonu
void initPWM() {
    // PWM pin konfigürasyonu
    TRISB = 0x0000;      // Çýkýþ pini

    // PWM modülü konfigürasyonu
    PTCONbits.PTEN = 1;  // PWM modülünü etkinleþtir
    PWMCON1 = 0x0777;    // PWM kanallarý
    PTPER = (FCY / (PWM_FREQUENCY * 2)) - 1; // Periyot ayarý
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
        // Güç artýyorsa gerilimi artýr
        mpptDutyCycle += (deltaVoltage > 0) ? MPPT_STEP : -MPPT_STEP;
    } else {
        // Güç azalýyorsa gerilim yönünü tersine çevir
        mpptDutyCycle -= (deltaVoltage > 0) ? MPPT_STEP : -MPPT_STEP;
    }

    // Sýnýr kontrolleri
    if (mpptDutyCycle > 0.95) mpptDutyCycle = 0.95;
    if (mpptDutyCycle < 0.05) mpptDutyCycle = 0.05;

    prevVoltage = voltage;
    prevPower = currentPower;

    return mpptDutyCycle;
}

// Sinüs Tablosu Oluþturma
void generateSineTable() {
    for (int i = 0; i < SINE_TABLE_SIZE; i++) {
        // Normalize edilmiþ sinüs deðerleri
        float angle = (2 * PI * i) / SINE_TABLE_SIZE;
        sineTable[i] = (int)(sin(angle) * 32767);
    }
}

// Ýnverter Çýkýþ Kontrol Fonksiyonu
void inverterControl() {
    static int tableIndex = 0;
    static float frequency = INVERTER_FREQUENCY;

    // Sinüs tablosundan deðer okuma
    int sineValue = sineTable[tableIndex];

    // PWM duty cycle hesaplama
    float dutyCycle = (sineValue + 32767) / 65534.0;

    // PWM çýkýþý ayarlama
    PDC1 = (int)(dutyCycle * PTPER);

    // Tablo indeksini güncelleme
    tableIndex = (tableIndex + 1) % SINE_TABLE_SIZE;
}

// Aþýrý Gerilim Korumasý
void overvoltageProtection() {
    if (batteryVoltage > MAX_VOLTAGE) {
        // Acil durdurma
        PTCONbits.PTEN = 0;  // PWM durdur
    }
}

// Ana Kontrol Fonksiyonu
void solarInverterSystem() {
    // Solar panel gerilim ve akým ölçümü
    solarVoltage = readSolarVoltage();
    solarCurrent = readSolarCurrent();
    batteryVoltage = readBatteryVoltage();

    // MPPT algoritmasý
    float mpptDuty = performMPPT(solarVoltage, solarCurrent);

    // Aþýrý gerilim kontrolü
    overvoltageProtection();

    // Ýnverter çýkýþ kontrolü
    inverterControl();
}

// Ana Fonksiyon
int main(void) {
    // Baþlangýç konfigürasyonlarý
    initADC();
    initPWM();
    generateSineTable();

    // Kesme ayarlarý
    T1CON = 0x8000;  // Timer konfigürasyonu

    while(1) {
        // Solar inverter sistem döngüsü
        solarInverterSystem();

        // Kýsa gecikme
        __delay_ms(10);
    }

    return 0;
}

// Yardýmcý Okuma Fonksiyonlarý
float readSolarVoltage() {
    ADCHS = 0x0002;  // Kanal seçimi
    ADCON1bits.SAMP = 1;
    while(!ADCON1bits.DONE);
    return ADCBUF0 * 0.01;
}

float readSolarCurrent() {
    ADCHS = 0x0003;  // Kanal seçimi
    ADCON1bits.SAMP = 1;
    while(!ADCON1bits.DONE);
    return ADCBUF0 * 0.001;
}

float readBatteryVoltage() {
    ADCHS = 0x0004;  // Kanal seçimi
    ADCON1bits.SAMP = 1;
    while(!ADCON1bits.DONE);
    return ADCBUF0 * 0.1;
}
