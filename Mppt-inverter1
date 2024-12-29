// Example improvements (adapt to your PIC)
#include <xc.h>
#include <stdint.h>
#include <stdio.h> // For sprintf (if needed for debugging)
#include "lcd.h"

// ... (Configuration bits and constants)

// Example ADC configuration (check your PIC's datasheet)
void initADC(void) {
    ADCON0 = 0x01; // Select channel AN0
    ADCON1 = 0x0E; // Configure AN0 as analog input, rest digital
    ADCON2 = 0x00; // Right justify, Tad = Fosc/2
    ADIE = 0;
    ADIF = 0;
    ADON = 1; // Turn on ADC
}

// Example Timer/PWM configuration (adjust for desired frequency)
void init_timers(void) {
    // Example for a 20kHz PWM frequency with a 20MHz clock
    PR2 = 249; // (Fosc / (4 * Prescaler * Fpwm)) - 1 = (20000000/(4*1*20000))-1 = 249
    T2CON = 0x04; // Prescaler 1:1, Timer2 on
    CCPR1L = 0; // Initialize duty cycle
    CCP1CON = 0x0C; // PWM mode
    TMR2ON = 1;

    // Timer1 for Inverter
    PR1 = 249;
    T1CON = 0x04;
    CCPR2L = 0;
    CCP2CON = 0x0C;
    TMR1ON = 1;
}

// Example of scaling for current measurement (replace with your actual scaling factor)
float readCurrent(void) {
    float adc_value = readADC(1);
    float current = adc_value * (ADC_VREF / ADC_RESOLUTION) * CURRENT_SENSOR_SCALE; // Add scaling
    return current;
}

// ... (Other functions)

void displayData(float voltage, float current, float power) {
    char buffer[20];
    lcd_clear();
    sprintf(buffer, "V:%.2fV", voltage);
    lcd_goto(0, 0);
    lcd_puts(buffer);
    sprintf(buffer, "I:%.2fA P:%.1fW", current, power);
    lcd_goto(0, 1);
    lcd_puts(buffer);
}
