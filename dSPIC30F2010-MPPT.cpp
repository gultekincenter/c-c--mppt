#include <p30f2010.h>
#include <dsp.h>

// MPPT Sabitler
#define STEP_SIZE 0.01    // Gerilim ad�m boyutu
#define MAX_VOLTAGE 20.0  // Maksimum panel gerilimi
#define MIN_VOLTAGE 0.0   // Minimum panel gerilimi

// Global De�i�kenler
float panelVoltage = 0.0;
float panelCurrent = 0.0;
float prevPower = 0.0;
float currentPower = 0.0;

// ADC Okuma Fonksiyonu
float readPanelVoltage() {
    // ADC kanal�ndan gerilim okuma
    ADCON1 = 0x0000;  // ADC konfig�rasyonu
    ADCHS = 0x0002;   // Kanal se�imi
    ADCON3 = 0x0F00; // �rnekleme zaman�

    ADCON1bits.ADON = 1;  // ADC'yi etkinle�tir
    ADCON1bits.SAMP = 1;  // �rneklemeyi ba�lat

    while(!ADCON1bits.DONE);  // D�n���m tamamlanana kadar bekle

    return ADCBUF0 * 0.0049; // ADC de�erini gerilime �evir
}

float readPanelCurrent() {
    // Benzer �ekilde ak�m okuma
    // Ak�m sens�r�nden ADC kanal� ile okuma
    ADCHS = 0x0003;  // Farkl� bir kanal

    ADCON1bits.ADON = 1;
    ADCON1bits.SAMP = 1;

    while(!ADCON1bits.DONE);

    return ADCBUF0 * 0.01; // ADC de�erini ak�ma �evir
}

// MPPT Perturb and Observe Algoritmas�
void performMPPT() {
    // Ge�erli g�� ve gerilimi hesapla
    panelVoltage = readPanelVoltage();
    panelCurrent = readPanelCurrent();
    currentPower = panelVoltage * panelCurrent;

    // G�� de�i�imini hesapla
    if (currentPower > prevPower) {
        // G�� art�yorsa gerilimi art�r
        panelVoltage += STEP_SIZE;
    } else {
        // G�� azal�yorsa gerilimi azalt
        panelVoltage -= STEP_SIZE;
    }

    // Gerilim s�n�rlar�n� kontrol et
    if (panelVoltage > MAX_VOLTAGE)
        panelVoltage = MAX_VOLTAGE;
    if (panelVoltage < MIN_VOLTAGE)
        panelVoltage = MIN_VOLTAGE;

    // PWM ile gerilim kontrol�
    PDC1 = (int)(panelVoltage * 100);  // PWM duty cycle ayar�

    // Bir sonraki iterasyon i�in g�ncelleme
    prevPower = currentPower;
}

// Ana fonksiyon
int main(void) {
    // Ba�lang�� konfig�rasyonlar�
    TRISB = 0x0000;  // Port konfig�rasyonu

    // PWM konfig�rasyonu
    PTCONbits.PTEN = 1;   // PWM mod�l�n� etkinle�tir
    PWMCON1 = 0x0077;     // PWM konfig�rasyonu

    // Timer konfig�rasyonu
    T1CON = 0x8000;       // Timer1 konfig�rasyonu

    while(1) {
        // MPPT algoritmas�n� periyodik olarak �al��t�r
        performMPPT();

        // K�sa gecikme
        __delay_ms(100);
    }

    return 0;
}
