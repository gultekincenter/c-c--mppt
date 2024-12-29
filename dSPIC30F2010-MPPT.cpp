#include <p30f2010.h>
#include <dsp.h>

// MPPT Sabitler
#define STEP_SIZE 0.01    // Gerilim adým boyutu
#define MAX_VOLTAGE 20.0  // Maksimum panel gerilimi
#define MIN_VOLTAGE 0.0   // Minimum panel gerilimi

// Global Deðiþkenler
float panelVoltage = 0.0;
float panelCurrent = 0.0;
float prevPower = 0.0;
float currentPower = 0.0;

// ADC Okuma Fonksiyonu
float readPanelVoltage() {
    // ADC kanalýndan gerilim okuma
    ADCON1 = 0x0000;  // ADC konfigürasyonu
    ADCHS = 0x0002;   // Kanal seçimi
    ADCON3 = 0x0F00; // Örnekleme zamaný

    ADCON1bits.ADON = 1;  // ADC'yi etkinleþtir
    ADCON1bits.SAMP = 1;  // Örneklemeyi baþlat

    while(!ADCON1bits.DONE);  // Dönüþüm tamamlanana kadar bekle

    return ADCBUF0 * 0.0049; // ADC deðerini gerilime çevir
}

float readPanelCurrent() {
    // Benzer þekilde akým okuma
    // Akým sensöründen ADC kanalý ile okuma
    ADCHS = 0x0003;  // Farklý bir kanal

    ADCON1bits.ADON = 1;
    ADCON1bits.SAMP = 1;

    while(!ADCON1bits.DONE);

    return ADCBUF0 * 0.01; // ADC deðerini akýma çevir
}

// MPPT Perturb and Observe Algoritmasý
void performMPPT() {
    // Geçerli güç ve gerilimi hesapla
    panelVoltage = readPanelVoltage();
    panelCurrent = readPanelCurrent();
    currentPower = panelVoltage * panelCurrent;

    // Güç deðiþimini hesapla
    if (currentPower > prevPower) {
        // Güç artýyorsa gerilimi artýr
        panelVoltage += STEP_SIZE;
    } else {
        // Güç azalýyorsa gerilimi azalt
        panelVoltage -= STEP_SIZE;
    }

    // Gerilim sýnýrlarýný kontrol et
    if (panelVoltage > MAX_VOLTAGE)
        panelVoltage = MAX_VOLTAGE;
    if (panelVoltage < MIN_VOLTAGE)
        panelVoltage = MIN_VOLTAGE;

    // PWM ile gerilim kontrolü
    PDC1 = (int)(panelVoltage * 100);  // PWM duty cycle ayarý

    // Bir sonraki iterasyon için güncelleme
    prevPower = currentPower;
}

// Ana fonksiyon
int main(void) {
    // Baþlangýç konfigürasyonlarý
    TRISB = 0x0000;  // Port konfigürasyonu

    // PWM konfigürasyonu
    PTCONbits.PTEN = 1;   // PWM modülünü etkinleþtir
    PWMCON1 = 0x0077;     // PWM konfigürasyonu

    // Timer konfigürasyonu
    T1CON = 0x8000;       // Timer1 konfigürasyonu

    while(1) {
        // MPPT algoritmasýný periyodik olarak çalýþtýr
        performMPPT();

        // Kýsa gecikme
        __delay_ms(100);
    }

    return 0;
}
