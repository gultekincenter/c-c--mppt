// C++ ile MPPT ve İnverter Kontrolü

#include <xc.h>
#include <stdint.h>
#include "lcd.h"

// Konfigürasyon bitleri ve sabitler
#define _XTAL_FREQ 20000000  // 20MHz kristal
#define ADC_VREF 5.0         // ADC referans voltajı
#define ADC_RESOLUTION 1023.0 // ADC çözünürlüğü
#define PWM_PERIOD 255       // PWM periyodu
#define DUTY_CYCLE_MAX 1.0   // Maksimum duty cycle
#define DUTY_CYCLE_MIN 0.0   // Minimum duty cycle
#define MPPT_STEP 0.02       // MPPT algoritması adım boyutu

// Fonksiyon prototipleri
void initADC(void);
void mppt_algorithm(void);
void inverter_control(void);
float readADC(uint8_t channel);
void displayData(float voltage, float current, float power);
void init_timers(void);

// Global değişkenler
static float previous_power = 0; 
static float previous_voltage = 0; // Önceki voltaj değeri
static float duty_cycle = 0.5;   

void main(void) {
    float voltage, current, power; 
    
    initADC();                     
    lcd_init();                   
    init_timers();               
    
    while (1) {
        voltage = readADC(0) * (ADC_VREF / ADC_RESOLUTION); 
        current = readADC(1) * (ADC_VREF / ADC_RESOLUTION); 
        power = voltage * current; 
        
        mppt_algorithm();          
        inverter_control();        
        displayData(voltage, current, power); 
        
        __delay_ms(100);          
    }
}

void initADC(void) {
    ADCON0 = 0x41;  
    ADCON1 = 0x0E;  
    ADIE = 0;       
    ADIF = 0;       
}

float readADC(uint8_t channel) {
    ADCON0 &= 0xC5;                  
    ADCON0 |= (channel << 3);        
    __delay_us(20);                  
    GO_nDONE = 1;                    
    while(GO_nDONE);                 
    return ((ADRESH << 8) + ADRESL); 
}

void mppt_algorithm(void) {
    float current_power;             
    float voltage = readADC(0) * (ADC_VREF / ADC_RESOLUTION); 
    float current = readADC(1) * (ADC_VREF / ADC_RESOLUTION); 
    
    current_power = voltage * current; 
    
    if (current_power > previous_power) { 
        duty_cycle += (voltage > previous_voltage) ? MPPT_STEP : -MPPT_STEP; 
    } else {
        duty_cycle += (voltage > previous_voltage) ? -MPPT_STEP : MPPT_STEP; 
    }
    
    duty_cycle = (duty_cycle > DUTY_CYCLE_MAX) ? DUTY_CYCLE_MAX : (duty_cycle < DUTY_CYCLE_MIN) ? DUTY_CYCLE_MIN : duty_cycle;
    
    CCPR1L = (unsigned char)(duty_cycle * PWM_PERIOD); 
    
    previous_power = current_power; 
    previous_voltage = voltage; 
}

void inverter_control(void) {
    static uint8_t sine_index = 0; 
    const uint8_t SINE_TABLE[32] = { 
        128,159,187,213,234,250,261,267,267,261,250,234,
        213,187,159,128,97,69,43,22,6,0,0,0,6,22,43,69,97,128
    };
    
    CCPR2L = SINE_TABLE[sine_index]; 
    
    sine_index = (sine_index + 1) % 32; 
}

void init_timers(void) {
    T2CON = 0x06;    
    PR2 = PWM_PERIOD; 
    CCP1CON = 0x0C;  
    
    T1CON = 0x06;    
    PR1 = PWM_PERIOD; 
    CCP2CON = 0x0C;  
    
    TMR2ON = 1;      
    TMR1ON = 1;      
}

void displayData(float voltage, float current, float power) {
    lcd_clear();                
    lcd_goto(0,0);             
    lcd_puts("V:");            
    lcd_putfloat(voltage, 2);  
    lcd_puts("V");             
    
    lcd_goto(0,1);             
    lcd_puts("I:");            
    lcd_putfloat(current, 2);  
    lcd_puts("A");             
    
    lcd_goto(8,1);             
    lcd_puts("P:");            
    lcd_putfloat(power, 1);    
    lcd_puts("W");             
}
