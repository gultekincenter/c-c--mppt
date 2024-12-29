#include "lcdsoft1.h"
#include "functions.h"
#include <math.h> // Matematiksel fonksiyonlar için

#define buzzer    LATDbits.LATD1
#define connect   LATEbits.LATE0
#define fan       LATEbits.LATE2
#define protect   LATEbits.LATE8
#define heat  700

#define SAMPLE_RATE 1000 // Örnekleme hýzý (Hz)
#define FREQUENCY 50     // Sinüs dalga frekansý (Hz)
#define AMPLITUDE 2390   // Maksimum PWM deðeri (0-2390 arasý)

signed int global_duty;
signed int duty_1, duty_2, duty_3;
int switch_condition;
int ad_switch = 0;
int factory[15] = {1, 800, 12, 12, 2500, 40, 0, 148, 138, 140, 120, 50, 80, 88};
int setting[15];
char arr[6];
int speedlimit = 20;

struct {
    unsigned int downkey:1;
    unsigned int upkey:1;
    unsigned int setkey:1;
    unsigned int setup:1;
    unsigned int gravity:1;
    unsigned int loadon:1;
    unsigned int solon:1;
    unsigned int chrcorrect:1;
    unsigned int chon:1;
    unsigned int swon:1;
    unsigned int pwmopen:1;
    unsigned int solraising:1;
    unsigned int chrraising:1;
    unsigned int fault:1;
    unsigned int stop:1;
    unsigned int msgrtn:1;
    unsigned int pvtemp:1;
    unsigned int loadonled:1;
    unsigned int bklte:1;
    unsigned int faultled:1;
} flags;

unsigned int rising = 0;
unsigned int pol = 0;
unsigned int *adjust;
unsigned int *ptr;
unsigned int adj;
unsigned int *value;
signed int moov, mwhtmp;
unsigned int pvmax;
unsigned int pvmin;
unsigned int counter, post;
unsigned int fault = 0;
unsigned int millisec = 0;
unsigned int sec = 0;
unsigned int min = 0;
unsigned int mintemp = 0;
unsigned long hrs = 0;
long btv;

// Saf sinüs dalga üretimi için deðiþkenler
unsigned int sine_wave[360]; // Sinüs dalga deðerleri için dizi

// Fonksiyonlar
unsigned int pv_sense();
void interrupt_Init(void);

// Sinüs dalga deðerlerini hesapla
void generate_sine_wave() {
    for (int i = 0; i < 360; i++) {
        sine_wave[i] = (unsigned int)((AMPLITUDE / 2) * (sin(i * PI / 180) + 1)); // 0-2390 arasý deðerler
    }
}

// Saf sinüs dalga üret
void generate_sine() {
    static int index = 0; // Dizi indeksini tut
    PDC1 = sine_wave[index]; // PWM çýkýþýný güncelle

    index++; // Ýndeksi artýr
    if (index >= 360) {
        index = 0; // Ýndeks sýfýrlanýr
    }
}

unsigned int solwattdisp, max_solwatt, chrtemp, chrtmr, batwatts, bat_v, soladc, hes1adc, ttmr, bzdly, batfultemp;
unsigned int soldisp, batdisp, chdisp, bath, batl, batfloat, batful, stmr, setuptmr, kwhtemp, kwh, mwh, lcdtmr, champs, ofset1, ofset2;
unsigned int solvolt, solh, soll, batvolt, batamps, solamps, keyvalue, heatntc, keyavg, ktmr, btmr, exittmr, hes1tmr, batcurtmr;
unsigned int batsel, ampsel, sol_mv, bat_mv, loadonv, loadofv, batnos, solmax, eraser, solhes, bathes, ct2, batcurrent, solampdisp;
signed int batclb, solclb, dummy_cycle;

int flag = 0;
long result, result1, ch_amps, chpower;
int batfunction = 0;
int solfunction = 0;
int pptfunction = 0;
int tlimit = 500;
unsigned int tmrs = 0;
unsigned int defaults, batavg, solavg, *temp;
unsigned int New_PW_Out;
unsigned int mode;
unsigned int code, adchanel;
int dec = 0;
int ctmr;
int keytmr = 0;
unsigned int track = 0;
unsigned int backlite = 0;
unsigned int initcount, yaxis;
unsigned int litedly = 0;

// Beep fonksiyonu
void beep(int s) {
    char ts;
    for (ts = 0; ts < s; ts++) {
        buzzer = 1;
        delay_ms(7);
        buzzer = 0;
        delay_ms(7);
    }
}

// PWM kontrol fonksiyonu
void pwm_control(void) {
    if (global_duty > 2390)
        global_duty = 2390;

    if (global_duty < 1)
        global_duty = 0;

    switch (switch_condition) {
        case 0:
            duty_1 = (global_duty - 0);
            PDC1 = dutycycle_limit(duty_1);

            duty_2 = (global_duty - 820);
            PDC2 = dutycycle_limit(duty_2);

            duty_3 = (global_duty - 1640);
            PDC3 = dutycycle_limit(duty_3);

            switch_condition = 1;
            break;

        case 1:
            duty_1 = (global_duty - 820);
            PDC1 = dutycycle_limit(duty_1);

            duty_2 = (global_duty - 1640);
            PDC2 = dutycycle_limit(duty_2);

            duty_3 = (global_duty - 0);
            PDC3 = dutycycle_limit(duty_3);

            switch_condition = 2;
            break;

        case 2:
            duty_1 = (global_duty - 1640);
            PDC1 = dutycycle_limit(duty_1);

            duty_2 = (global_duty - 0);
            PDC2 = dutycycle_limit(duty_2);

            duty_3 = (global_duty - 820);
            PDC3 = dutycycle_limit(duty_3);

            switch_condition = 0;
            break;
    }
}

// Kesme fonksiyonlarý
void __attribute__((__interrupt__, __auto_psv__)) _INT0Interrupt(void) {
    if (IFS0bits.INT0IF == 1) {
        IFS0bits.INT0IF = 0;
        __asm__ volatile ("reset");
        OVDCON = 0X0000;
        PDC1 = PDC2 = PDC3 = 0;
        lcd_init();
    }
}

void __attribute__((__interrupt__, __auto_psv__)) _ADCInterrupt(void) {
    IFS0bits.ADIF = 0;

    // Batarya ve güneþ enerjisi okuma iþlemleri...
}

// Ana fonksiyon
int main() {
    TRISF = 0X0000;
    TRISE = 0X0080;
    TRISD = 0X0000;
    TRISC = 0X0000;
    TRISB = 0XFFFF;
    OVDCON = 0X0000;
    PWMCON1 = 0x0000;
    PTCONbits.PTEN = 0;
    buzzer = 0;
    fan = 0;
    CNPU1bits.CN0PUE = 1;
    connect = 0;
    lcd_init();
    flags.setup = 0;
    flags.stop = 1;
    flags.pwmopen = 0;
    global_duty = 0;
    flags.bklte = 1;
    pol = 0;
    memread();

    // EEPROM'dan varsayýlan ayarlarý oku
    defaults = Eeprom_ReadWord(11);
    if (defaults != 50) {
        memwrite();
        __asm__ volatile ("reset");
    }

    // PWM Baþlatma
    init_PWM();
    batnos = setting[0];
    batsel = batnos * 12;
    ampsel = setting[1];
    sol_mv = setting[2];
    bat_mv = setting[3];
    solmax = setting[4];
    batclb = setting[5];
    solclb = setting[6];
    batful = setting[7] * batnos;
    batfloat = setting[8] * batnos;
    loadonv = setting[9] * batnos;
    loadofv = setting[10] * batnos;
    defaults = setting[11];
    kwh = Eeprom_ReadWord(50);
    kwhtemp = kwh;
    ofset1 = setting[12];
    ofset2 = setting[13];
    init_PWM();
    interrupt_Init();
    InitADC1();

    // Sinüs dalga deðerlerini oluþtur
    generate_sine_wave();

    __delay32(55000);
    __delay32(55000);

    while (1) {
        // Ana döngü
        while (PORTEbits.RE8 == 0) {
            backlite = 1;
            flags.faultled = 1;
            printmes(str80, 100, 1);
            printmes(str81, 100, 2);
        }

        // Saf sinüs dalga üret
        generate_sine(); // Sinüs dalgasýný üret

        flags.loadonled = 0;
        pptfunction = 0;
        flags.stop = 0;
        track = 1;

        dec = 1;
        pol = 1;
        temp = &chdisp;
        printmes(str14, 1, 3); // "CHARGING:     "
        pol = 0;
        dec = 1;

        temp = &batdisp;
        printmes(str12, 1, 1); // "BATT VOLT:     V"

        temp = &soldisp;
        printmes(str13, 1, 2); // "SOLAR VOLT:     "

        dec = 1;
        temp = &chpower;
        printmes(str09, 1, 4); // "TOTAL KW "

        if (kwh != kwhtemp)
            Eeprom_WriteWord(50, kwh);

        if (flags.setup == 1) {
            function_set();
        }

        // Hata Kontrolleri
        if (fault == 1) {
            while (1) {
                OVDCON = 0X0000;
                PDC1 = PDC2 = PDC3 = 0;
                buzzer = 1;
                printmes(str22, 100, 1); // fault1
                if (keyvalue > 1000) {
                    while (keyvalue > 1000);
                    __asm__ volatile ("reset");
                }
                if (keyvalue < 1000) {
                    while (keyvalue < 1000);
                    __asm__ volatile ("reset");
                }
            }
        }

        // Diðer hata durumlarý için benzer döngüler...
    }
}

// ADC Baþlatma Fonksiyonu
void InitADC1() {
    _ADON = 1; // ADC'yi aç
    ADCON1 = 0x00EC;
    ADCON3 = 0x0003;
    ADPCFG = 0x0000;
    _SMPI = 0x07;
    _ADCS = 0b111111;
    _SAMC = 0b11111;
    _ADRC = 1;
    ADCON2bits.CHPS = 0;
    _BUFM = 0;
    _ALTS = 0;
    _CH0NA = 0;
    _CSCNA = 1;
    ADCSSL = 0b111111;
    ADCON1bits.ADON = 1;
    IEC0bits.ADIE = 1; // ADC kesmesini etkinleþtir
}

// Millisaniye Gecikme Fonksiyonu
void delay_ms(unsigned int gs) {
    while (gs--) {
        __delay32(55000); // Gecikme süresi
    }
}

// Mikrosaniye Gecikme Fonksiyonu
void delay_us(unsigned int gs) {
    while (gs--) {
        __delay32(3000); // Gecikme süresi
    }
}

// ADC'den Deðer Okuma Fonksiyonu
int getvalue(int ch) {
    ADCON1bits.DONE = 0;
    ADCHS = ch;
    ADCON1bits.SAMP = 1;
    __delay32(50);
    ADCON1bits.SAMP = 0;
    while (!ADCON1bits.DONE);
    return ADCBUF1;
}

// Kesme Baþlatma Fonksiyonu
void interrupt_Init(void) {
    INTCON2 = 0x0001;     // MMA8452Q kesme çýkýþ sinyalinin düþen kenarýnda INT0'ý ayarla
    IFS0bits.INT0IF = 0;  // INT0 kesme bayraðýný temizle
    IEC0bits.INT0IE = 1;  // INT0 ISR'yi etkinle
    IPC0bits.INT0IP = 7;  // En yüksek öncelik
    IFS0bits.CNIF = 0;
}
