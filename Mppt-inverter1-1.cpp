// ... (Includes and Defines)

// Use uint16_t for ADC values
uint16_t readADC(uint8_t channel) {
    // ... (ADC reading code)
    return ((ADRESH << 8) + ADRESL);
}

void mppt_algorithm(void) {
    static float previous_duty_cycle = 0.5; // Store previous duty cycle
    float voltage = (float)readADC(0) * (ADC_VREF / ADC_RESOLUTION);
    float current = (float)readADC(1) * (ADC_VREF / ADC_RESOLUTION);
    float current_power = voltage * current;

    if (current_power > previous_power) {
        duty_cycle += MPPT_STEP; // Perturb duty cycle directly
    } else {
        duty_cycle -= MPPT_STEP;
    }

    duty_cycle = (duty_cycle > DUTY_CYCLE_MAX) ? DUTY_CYCLE_MAX : (duty_cycle < DUTY_CYCLE_MIN) ? DUTY_CYCLE_MIN : duty_cycle;

    CCPR1L = (unsigned char)(duty_cycle * PWM_PERIOD);

    previous_power = current_power;
    previous_voltage = voltage;
    previous_duty_cycle = duty_cycle; // Update previous duty cycle
}

// Example of using a timer for non-blocking delays (Illustrative)
volatile uint8_t timer_flag = 0;

void __interrupt() isr(void) {
    if (TMR0IF) { // Check Timer0 interrupt flag
        TMR0IF = 0; // Clear the flag
        timer_flag = 1;
    }
}

void main(void) {
    // ... (Initialization)
    // Configure Timer0 for desired interrupt frequency
    while (1) {
        // ... (ADC readings and calculations)

        mppt_algorithm();
        inverter_control();
        displayData(voltage, current, power);

        if (timer_flag) {
            timer_flag = 0;
            // Code to run at the timer's frequency (e.g., display updates)
        }
    }
}

// ... (Other functions)
