#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <U8g2lib.h>
#include <Wire.h>
#include <PID_v1.h>

// Pin Tan�mlamalar�
#define SOLAR_VOLTAGE_PIN     34
#define SOLAR_CURRENT_PIN     35
#define BATTERY_VOLTAGE_PIN   36
#define INVERTER_PWM_PIN      16
#define LCD_SCL_PIN           22
#define LCD_SDA_PIN           21

// Sistem Sabitleri
#define PWM_FREQUENCY         20000
#define INVERTER_FREQUENCY    50
#define VOLTAGE_REFERENCE     220.0

// MPPT Parametreleri
#define MPPT_STEP             0.01
#define MAX_VOLTAGE           40.0
#define MIN_VOLTAGE           10.0

// G�venlik E�ikleri
#define MAX_TEMPERATURE       75.0
#define MAX_POWER_THRESHOLD   500.0
#define MIN_BATTERY_VOLTAGE   10.5
#define MAX_BATTERY_VOLTAGE   14.5

// Wi-Fi Ayarlar�
const char* WIFI_SSID = "MPPT_Inverter";
const char* WIFI_PASS = "12345678";

// Sistem Parametreleri Yap�s�
struct SystemParameters {
    float solarVoltage;
    float solarCurrent;
    float solarPower;
    float batteryVoltage;
    float temperature;
    float inverterEfficiency;
    float mpptDutyCycle;
    uint8_t systemStatus;
};

// Global De�i�kenler
SystemParameters sysParams;
WebServer server(80);
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, LCD_SCL_PIN, LCD_SDA_PIN);

// PID Kontrol De�i�kenleri
double setpoint, input, output;
PID mpptPID(&input, &output, &setpoint, 2, 5, 1, DIRECT);

// Hata Bayraklar�
enum ErrorFlags {
    OVER_TEMPERATURE = 0x01,
    LOW_BATTERY      = 0x02,
    HIGH_BATTERY     = 0x04,
    OVERLOAD         = 0x08
};

// Geli�mi� MPPT Algoritmas�
float performAdvancedMPPT(float voltage, float current) {
    static float prevVoltage = 0;
    static float prevPower = 0;
    static float stepSize = MPPT_STEP;

    float currentPower = voltage * current;
    float deltaPower = currentPower - prevPower;
    float deltaVoltage = voltage - prevVoltage;

    // Perturb and Observe Algoritmas�
    if (deltaPower > 0) {
        stepSize = (deltaVoltage > 0) ? stepSize * 1.1 : stepSize * 0.9;
        sysParams.mpptDutyCycle += (deltaVoltage > 0) ? stepSize : -stepSize;
    } else {
        stepSize = (deltaVoltage > 0) ? stepSize * 0.9 : stepSize * 1.1;
        sysParams.mpptDutyCycle -= (deltaVoltage > 0) ? stepSize : -stepSize;
    }

    // S�n�r Kontrolleri
    sysParams.mpptDutyCycle = constrain(sysParams.mpptDutyCycle, 0.05, 0.95);

    // Verim Hesaplama
    sysParams.inverterEfficiency = currentPower / (voltage * current);

    prevVoltage = voltage;
    prevPower = currentPower;

    return sysParams.mpptDutyCycle;
}

// G�venlik Kontrolleri
void performSafetyChecks() {
    sysParams.systemStatus = 0;

    // S�cakl�k Kontrol�
    if (sysParams.temperature > MAX_TEMPERATURE) {
        sysParams.systemStatus |= OVER_TEMPERATURE;
        ledcWrite(0, 0); // PWM Durdur
    }

    // Batarya Gerilim Kontrolleri
    if (sysParams.batteryVoltage < MIN_BATTERY_VOLTAGE) {
        sysParams.systemStatus |= LOW_BATTERY;
    }

    if (sysParams.batteryVoltage > MAX_BATTERY_VOLTAGE) {
        sysParams.systemStatus |= HIGH_BATTERY;
    }

    // G�� A��m� Kontrol�
    if (sysParams.solarPower > MAX_POWER_THRESHOLD) {
        sysParams.systemStatus |= OVERLOAD;
        ledcWrite(0, 0);
    }
}

// LCD Ekran G�ncelleme
void updateLCDDisplay() {
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_ncenB08_tr);

    // Solar Bilgileri
    u8g2.setCursor(0, 10);
    u8g2.printf("Sol V: %.2fV", sysParams.solarVoltage);

    u8g2.setCursor(0, 20);
    u8g2.printf("Sol I: %.2fA", sysParams.solarCurrent);

    u8g2.setCursor(0, 30);
    u8g2.printf("Sol P: %.2fW", sysParams.solarPower);

    // Batarya Bilgileri
    u8g2.setCursor(0, 40);
    u8g2.printf("Bat V: %.2fV", sysParams.batteryVoltage);

    // Sistem Durumu
    u8g2.setCursor(0, 50);
    u8g2.printf("Durum: %s",
        (sysParams.systemStatus == 0) ? "Normal" : "Hata Var");

    u8g2.sendBuffer();
}

// Web Sunucu Y�nlendirmeleri
void setupWebServer() {
    server.on("/", HTTP_GET, []() {
        String html = "<!DOCTYPE html><html><body>";
        html += "<h1>MPPT Inverter Kontrol Paneli</h1>";
        html += "<p>Solar Gerilim: " + String(sysParams.solarVoltage) + " V</p>";
        html += "<p>Solar Ak�m: " + String(sysParams.solarCurrent) + " A</p>";
        html += "<p>Solar G��: " + String(sysParams.solarPower) + " W</p>";
        html += "<p>Batarya Gerilimi: " + String(sysParams.batteryVoltage) + " V</p>";
        html += "</body></html>";

        server.send(200, "text/html", html);
    });

    server.begin();
}

void setup() {
    Serial.begin(115200);

    // Pin Modlar�
    pinMode(SOLAR_VOLTAGE_PIN, INPUT);
    pinMode(SOLAR_CURRENT_PIN, INPUT);

    // LCD Ba�latma
    u8g2.begin();

    // Wi-Fi Ba�latma
    WiFi.softAP(WIFI_SSID, WIFI_PASS);

    // Web Sunucu Kurulumu
    setupWebServer();

    // PWM Ba�latma
    ledcSetup(0, PWM_FREQUENCY, 8);
    ledcAttachPin(INVERTER_PWM_PIN, 0);

    // PID Kontrol Ayarlar�
    mpptPID.SetMode(AUTOMATIC);
    mpptPID.SetOutputLimits(0, 255);
}

void loop() {
    // �l��mler
    sysParams.solarVoltage = analogRead(SOLAR_VOLTAGE_PIN) * (5.0 / 4095.0);
    sysParams.solarCurrent = analogRead(SOLAR_CURRENT_PIN) * (5.0 / 4095.0);
    sysParams.batteryVoltage = analogRead(BATTERY_VOLTAGE_PIN) * (5.0 / 4095.0);

    // G�� Hesaplama
    sysParams.solarPower = sysParams.solarVoltage * sysParams.solarCurrent;

    // MPPT Algoritmas�
    float dutyCycle = performAdvancedMPPT(sysParams.solarVoltage, sysParams.solarCurrent);

    // PWM Kontrol
    ledcWrite(0, dutyCycle * 255);

    // G�venlik Kontrolleri
    performSafetyChecks();

    // LCD G�ncelleme
    updateLCDDisplay();

    // Web Sunucu ��lemleri
    server.handleClient();

    delay(100);
}
