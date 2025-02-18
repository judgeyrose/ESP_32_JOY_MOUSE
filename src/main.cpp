#include <Arduino.h>
#include <BleMouse.h>

// Joystick pins (adjust based on your wiring)
#define JOY_X A0  // X-axis (analog)
#define JOY_Y A1  // Y-axis (analog)
#define JOY_BTN D2 // Button (digital)

// LED pins
#define GREEN_LED 7 // Green LED for button press feedback
#define RED_LED 5   // Red LED to indicate disconnected state

// Mouse movement sensitivity
#define DEADZONE 500   // Ignore small movements
#define SPEED 0.5f     // Adjust cursor speed (fractional value allowed)

// Inactivity timeout (in milliseconds)
#define INACTIVITY_TIMEOUT 60000 // 1 minute

RTC_DATA_ATTR int bootCount = 0;

BleMouse bleMouse("JerkMate x9000");
unsigned long lastActivityTime = 0;

// Method to print the reason by which ESP32 has been awakened from sleep
void print_wakeup_reason() {
    esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
    switch (wakeup_reason) {
        case ESP_SLEEP_WAKEUP_EXT0: Serial.println("Wakeup caused by external signal using RTC_IO"); break;
        case ESP_SLEEP_WAKEUP_EXT1: Serial.println("Wakeup caused by external signal using RTC_CNTL"); break;
        case ESP_SLEEP_WAKEUP_TIMER: Serial.println("Wakeup caused by timer"); break;
        case ESP_SLEEP_WAKEUP_TOUCHPAD: Serial.println("Wakeup caused by touchpad"); break;
        case ESP_SLEEP_WAKEUP_ULP: Serial.println("Wakeup caused by ULP program"); break;
        default: Serial.printf("Wakeup was not caused by deep sleep: %d\n", wakeup_reason); break;
    }
}

void setup() {
    Serial.begin(115200);
    delay(1000); // Take some time to open up the Serial Monitor

    // Increment boot number and print it every reboot
    ++bootCount;
    Serial.println("Boot number: " + String(bootCount));

    // Print the wakeup reason for ESP32
    print_wakeup_reason();

    pinMode(JOY_BTN, INPUT_PULLUP);
    pinMode(GREEN_LED, OUTPUT);
    pinMode(RED_LED, OUTPUT);

    analogWrite(GREEN_LED, 0); // Set initial brightness to 0 (off)
    analogWrite(RED_LED, 0);   // Set initial brightness to 0 (off)

    // Initialize BLE Mouse
    bleMouse.begin();
    Serial.println("BLE initialized and advertising started");

    lastActivityTime = millis();
}

void loop() {
    if (!bleMouse.isConnected()) {
        analogWrite(RED_LED, 5); // Dim red LED when disconnected
        //Serial.println("LED State: RED ON (disconnected)");
        return;
    } else {
        analogWrite(RED_LED, 0); // Turn off red LED when connected
        //Serial.println("LED State: RED OFF (connected)");
    }

    int x = analogRead(JOY_X) - 2048; // Center around 2048 for 12-bit ADC
    int y = analogRead(JOY_Y) - 2048; // Center around 2048 for 12-bit ADC
    bool buttonPressed = digitalRead(JOY_BTN) == LOW;

    // Check for activity
    if (abs(x) > DEADZONE || abs(y) > DEADZONE || buttonPressed) {
        lastActivityTime = millis();
    }

    // Print raw joystick values to the serial monitor
    //Serial.print("Raw X: ");
    //Serial.print(x);
    //Serial.print(" Raw Y: ");
    //Serial.print(y);
    //Serial.print(" Button: ");
    //Serial.println(buttonPressed ? "Pressed" : "Released");

    if (abs(x) > DEADZONE || abs(y) > DEADZONE) {
        bleMouse.move((x / 128) * SPEED, (-y / 128) * SPEED);
    }

    if (buttonPressed) {
        bleMouse.press(MOUSE_LEFT);
        analogWrite(GREEN_LED, 5); // Dim green LED when button is pressed
        //Serial.println("LED State: GREEN ON (button pressed)");
    } else {
        bleMouse.release(MOUSE_LEFT);
        analogWrite(GREEN_LED, 0); // Turn off green LED when button is released
        //Serial.println("LED State: GREEN OFF (button released)");
    }

    // Check for inactivity
    if (millis() - lastActivityTime > INACTIVITY_TIMEOUT) {
        Serial.println("Entering deep sleep due to inactivity");
        esp_deep_sleep_enable_gpio_wakeup(BIT(JOY_BTN), ESP_GPIO_WAKEUP_GPIO_LOW); // Wake up on button press (LOW state)
        Serial.println("Going to sleep now");
        esp_deep_sleep_start();
    }

    delay(10); // Adjust the delay as needed
}
