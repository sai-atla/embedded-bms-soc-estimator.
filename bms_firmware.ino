#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
#define SCREEN_ADDRESS 0x3C

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Hardware Pin Configuration
const int ADC_PIN = A0;
const int ALARM_PIN = 13;

// Voltage Divider & ADC Calibration Constants
const float V_REF = 5.00;              // Microcontroller operating rail (Volts)
const float R1 = 10000.0;              // Upper divider resistor (10k Ohm)
const float R2 = 10000.0;              // Lower divider resistor (10k Ohm)
const float DIVIDER_RATIO = (R1 + R2) / R2; // Hardware scale multiplier (2.0)

// Single-Cell Li-ion Operating Envelope
const float V_CELL_MAX = 4.20;          // 100% State-of-Charge (SoC)
const float V_CELL_MIN = 3.00;          // 0% Cut-off limit
const float V_ALERT_THRESHOLD = 3.30;   // Low-voltage critical alarm

// DSP Circular Moving Average Filter Parameters
const int FILTER_SAMPLES = 16;
int adc_buffer[FILTER_SAMPLES];
int filter_index = 0;
long running_sum = 0;

// Function Prototypes
float readFilteredVoltage();
int estimateLiIonSoC(float voltage);
void renderTelemetry(float voltage, int soc);

void setup() {
  Serial.begin(115200);
  pinMode(ADC_PIN, INPUT);
  pinMode(ALARM_PIN, OUTPUT);
  digitalWrite(ALARM_PIN, LOW);

  // Pre-fill circular filter buffer with initial ADC reading
  int initial_sample = analogRead(ADC_PIN);
  for (int i = 0; i < FILTER_SAMPLES; i++) {
    adc_buffer[i] = initial_sample;
    running_sum += initial_sample;
  }

  // Initialize SSD1306 OLED via I2C
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("[FATAL] SSD1306 allocation failed. Check I2C wiring."));
    while (true); // Lock execution on hardware failure
  }

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(12, 28);
  display.println(F("BMS TELEMETRY INIT"));
  display.display();
  delay(1200);
}

void loop() {
  static unsigned long last_sample_time = 0;
  const unsigned long sample_interval_ms = 200; // 5 Hz non-blocking execution

  if (millis() - last_sample_time >= sample_interval_ms) {
    last_sample_time = millis();

    // 1. Acquire filtered voltage and compute SoC
    float cell_voltage = readFilteredVoltage();
    int soc_percent = estimateLiIonSoC(cell_voltage);

    // 2. Evaluate safety comparator threshold
    if (cell_voltage <= V_ALERT_THRESHOLD) {
      digitalWrite(ALARM_PIN, HIGH); // Trigger under-voltage warning LED
    } else {
      digitalWrite(ALARM_PIN, LOW);
    }

    // 3. Emit structured telemetry frame over UART (115200 baud)
    Serial.print(F("[TELEM] V_Cell: "));
    Serial.print(cell_voltage, 3);
    Serial.print(F(" V | SoC: "));
    Serial.print(soc_percent);
    Serial.print(F(" % | Status: "));
    Serial.println(cell_voltage <= V_ALERT_THRESHOLD ? F("UNDERVOLTAGE_ALERT") : F("NOMINAL"));

    // 4. Update graphic interface
    renderTelemetry(cell_voltage, soc_percent);
  }
}

// Low-latency O(1) Circular Moving Average Digital Filter
float readFilteredVoltage() {
  running_sum -= adc_buffer[filter_index];
  int new_sample = analogRead(ADC_PIN);
  adc_buffer[filter_index] = new_sample;
  running_sum += new_sample;

  filter_index = (filter_index + 1) % FILTER_SAMPLES;
  float average_raw = (float)running_sum / FILTER_SAMPLES;

  // Convert 10-bit quantized ADC count (0-1023) to true input voltage
  float v_adc_pin = (average_raw * V_REF) / 1023.0;
  return v_adc_pin * DIVIDER_RATIO;
}

// 4-Region Piecewise Non-Linear SoC Model for Li-ion NMC Chemistry
int estimateLiIonSoC(float voltage) {
  if (voltage >= V_CELL_MAX) return 100;
  if (voltage <= V_CELL_MIN) return 0;

  if (voltage > 3.95) {
    return 80 + (int)((voltage - 3.95) / (4.20 - 3.95) * 20.0);
  } else if (voltage > 3.75) {
    return 40 + (int)((voltage - 3.75) / (3.95 - 3.75) * 40.0);
  } else if (voltage > 3.45) {
    return 10 + (int)((voltage - 3.45) / (3.75 - 3.45) * 30.0);
  } else {
    return (int)((voltage - 3.00) / (3.45 - 3.00) * 10.0);
  }
}

// Telemetry Display Driver Routine
void renderTelemetry(float voltage, int soc) {
  display.clearDisplay();

  // Header Bar
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print(F("BMS MONITOR"));
  display.setCursor(85, 0);
  display.print(soc <= 15 ? F("ALERT") : F("OK"));
  display.drawLine(0, 10, 128, 10, SSD1306_WHITE);

  // Cell Voltage Output
  display.setCursor(0, 16);
  display.print(F("Voltage : "));
  display.print(voltage, 2);
  display.print(F(" V"));

  // State-of-Charge (SoC) Output
  display.setCursor(0, 28);
  display.print(F("SoC Est : "));
  display.print(soc);
  display.print(F(" %"));

  // Battery Progress Bar Outline & Fill
  display.drawRect(0, 44, 120, 16, SSD1306_WHITE);
  display.fillRect(120, 48, 4, 8, SSD1306_WHITE); // Battery positive terminal tip
  int gauge_fill = map(soc, 0, 100, 0, 114);
  if (gauge_fill > 0) {
    display.fillRect(3, 47, gauge_fill, 10, SSD1306_WHITE);
  }

  display.display();
}
