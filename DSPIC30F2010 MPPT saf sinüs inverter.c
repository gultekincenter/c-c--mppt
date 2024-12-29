// K�t�phaneler ve Tan�mlamalar
#include <p30f2010.h>
#include <dsp.h>
#include <math.h>

// Sistem Parametreleri
#define PWM_FREQUENCY     20000   // 20 kHz PWM frekans�
#define INVERTER_FREQUENCY 50      // 50 Hz ��k�� frekans�
#define VOLTAGE_REFERENCE 220.0    // AC ��k�� gerilimi
#define PI 3.14159265358979

// MPPT Sabitleri
#define MPPT_STEP 0.01
#define MAX_VOLTAGE 40.0
#define MIN_VOLTAGE 10.0

// Global De�i�kenler
volatile float solarVoltage = 0;
volatile float solarCurrent = 0;
volatile float batteryVoltage = 0;
volatile float mpptDutyCycle = 0;

// Sin�s Tablosu Tan�mlamas�
const int SINE_TABLE_SIZE = 256;
volatile int sineTable[256];

// ADC Konfig�rasyon Fonksiyonlar�
void initADC() {
    ADCON1 = 0x0000;     // ADC konfig�rasyonu
    ADCON2 = 0x0000;     // �rnekleme zaman� ayarlar�
    ADCON3 = 0x0F00;     // D�n���m zaman�
    ADCHSbits.CH0SA = 2; // Kanal se�imi
    ADPCFG = 0xFFFF;     // Dijital pine �evir
    ADPCFGbits.PCFG2 = 0; // Analog giri�
    ADCON1bits.ADON = 1; // ADC'yi etkinle�tir
}

// PWM Konfig�rasyon Fonksiyonu
void initPWM() {
    // PWM pin konfig�rasyonu
    TRISB = 0x0000;      // ��k�� pini

    // PWM mod�l� konfig�rasyonu
    PTCONbits.PTEN = 1;  // PWM mod�l�n� etkinle�tir
    PWMCON1 = 0x0777;    // PWM kanallar�
    PTPER = (FCY / (PWM_FREQUENCY * 2)) - 1; // Periyot ayar�
}

// MPPT Algoritmas�
float performMPPT(float voltage, float current) {
    static float prevVoltage = 0;
    static float prevPower = 0;

    float currentPower = voltage * current;
    float deltaPower = currentPower - prevPower;
    float deltaVoltage = voltage - prevVoltage;

    // Perturb and Observe Algoritmas�
    if (deltaPower > 0) {
        // G�� art�yorsa gerilimi art�r
        mpptDutyCycle += (deltaVoltage > 0) ? MPPT_STEP : -MPPT_STEP;
    } else {
        // G�� azal�yorsa gerilim y�n�n� tersine �evir
        mpptDutyCycle -= (deltaVoltage > 0) ? MPPT_STEP : -MPPT_STEP;
    }

    // S�n�r kontrolleri
    if (mpptDutyCycle > 0.95) mpptDutyCycle = 0.95;
    if (mpptDutyCycle < 0.05) mpptDutyCycle = 0.05;

    prevVoltage = voltage;
    prevPower = currentPower;

    return mpptDutyCycle;
}

// Sin�s Tablosu Olu�turma
void generateSineTable() {
    for (int i = 0; i < SINE_TABLE_SIZE; i++) {
        // Normalize edilmi� sin�s de�erleri
        float angle = (2 * PI * i) / SINE_TABLE_SIZE;
        sineTable[i] = (int)(sin(angle) * 32767);
    }
}

// �nverter ��k�� Kontrol Fonksiyonu
void inverterControl() {
    static int tableIndex = 0;
    static float frequency = INVERTER_FREQUENCY;

    // Sin�s tablosundan de�er okuma
    int sineValue = sineTable[tableIndex];

    // PWM duty cycle hesaplama
    float dutyCycle = (sineValue + 32767) / 65534.0;

    // PWM ��k��� ayarlama
    PDC1 = (int)(dutyCycle * PTPER);

    // Tablo indeksini g�ncelleme
    tableIndex = (tableIndex + 1) % SINE_TABLE_SIZE;
}

// A��r� Gerilim Korumas�
void overvoltageProtection() {
    if (batteryVoltage > MAX_VOLTAGE) {
        // Acil durdurma
        PTCONbits.PTEN = 0;  // PWM durdur
    }
}

// Ana Kontrol Fonksiyonu
void solarInverterSystem() {
    // Solar panel gerilim ve ak�m �l��m�
    solarVoltage = readSolarVoltage();
    solarCurrent = readSolarCurrent();
    batteryVoltage = readBatteryVoltage();

    // MPPT algoritmas�
    float mpptDuty = performMPPT(solarVoltage, solarCurrent);

    // A��r� gerilim kontrol�
    overvoltageProtection();

    // �nverter ��k�� kontrol�
    inverterControl();
}

// Ana Fonksiyon
int main(void) {
    // Ba�lang�� konfig�rasyonlar�
    initADC();
    initPWM();
    generateSineTable();

    // Kesme ayarlar�
    T1CON = 0x8000;  // Timer konfig�rasyonu

    while(1) {
        // Solar inverter sistem d�ng�s�
        solarInverterSystem();

        // K�sa gecikme
        __delay_ms(10);
    }

    return 0;
}

// Yard�mc� Okuma Fonksiyonlar�
float readSolarVoltage() {
    ADCHS = 0x0002;  // Kanal se�imi
    ADCON1bits.SAMP = 1;
    while(!ADCON1bits.DONE);
    return ADCBUF0 * 0.01;
}

float readSolarCurrent() {
    ADCHS = 0x0003;  // Kanal se�imi
    ADCON1bits.SAMP = 1;
    while(!ADCON1bits.DONE);
    return ADCBUF0 * 0.001;
}

float readBatteryVoltage() {
    ADCHS = 0x0004;  // Kanal se�imi
    ADCON1bits.SAMP = 1;
    while(!ADCON1bits.DONE);
    return ADCBUF0 * 0.1;
}
