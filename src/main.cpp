// Biblioteca Steam: recibe por Serial "Titulo|Precio" o "NOT_FOUND"
// y lo muestra en una OLED con LEDs y buzzer.
#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>

constexpr uint8_t LED_GREEN_PIN = 2;
constexpr uint8_t LED_RED_PIN = 3;
constexpr uint8_t BUZZER_PIN = 4;
constexpr uint8_t OLED_ADDRESSES[] = {0x3C, 0x3D};
constexpr size_t MAX_LINE = 120;
constexpr unsigned long LED_ON_MS = 2000;

constexpr unsigned int NOT_FOUND_TONE_HZ = 300;
constexpr unsigned long NOT_FOUND_TONE_MS = 600;
constexpr unsigned int FOUND_TONE_HZ = 2000;
constexpr unsigned long FOUND_TONE_MS = 150;

Adafruit_SSD1306 display(128, 64, &Wire, -1);
bool oledOk = false;

char lineBuf[MAX_LINE + 1];
size_t lineLen = 0;
bool lineOverflow = false;

unsigned long ledOffAt = 0;
uint8_t activeLed = 0;

void showLines(const char* l1, const char* l2 = "", uint8_t size2 = 1) {
  if (!oledOk) return;
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println(l1);
  display.setTextSize(size2);
  display.setCursor(0, 32);
  display.println(l2);
  display.display();
}

void showTitlePrice(const char* title, const char* price) {
  if (!oledOk) return;
  char priceLine[24];
  snprintf(priceLine, sizeof(priceLine), "$%s", price);
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println(title);
  display.setTextSize(2);
  display.setCursor(0, 48);
  display.println(priceLine);
  display.display();
}

void showWaiting() {
  showLines("Scan barcode", "Waiting...", 2);
}

void startLed(uint8_t pin) {
  digitalWrite(LED_GREEN_PIN, LOW);
  digitalWrite(LED_RED_PIN, LOW);
  digitalWrite(pin, HIGH);
  activeLed = pin;
  ledOffAt = millis() + LED_ON_MS;
}

void testInicial() {
  showLines("TEST INICIAL", "Pantalla OK");
  delay(800);
  showLines("TEST INICIAL", "LED verde");
  digitalWrite(LED_GREEN_PIN, HIGH);
  delay(600);
  digitalWrite(LED_GREEN_PIN, LOW);
  showLines("TEST INICIAL", "LED rojo");
  digitalWrite(LED_RED_PIN, HIGH);
  delay(600);
  digitalWrite(LED_RED_PIN, LOW);
  showLines("TEST INICIAL", "Buzzer");
  tone(BUZZER_PIN, 2000, 200);
  delay(300);
  tone(BUZZER_PIN, NOT_FOUND_TONE_HZ, NOT_FOUND_TONE_MS);
  delay(700);
}

char* trim(char* text) {
  while (*text == ' ' || *text == '\t') text++;
  char* end = text + strlen(text);
  while (end > text && (end[-1] == ' ' || end[-1] == '\t')) {
    *--end = '\0';
  }
  return text;
}

void toAscii(char* text) {
  char* output = text;
  for (; *text; text++) {
    if (static_cast<uint8_t>(*text) < 128) {
      *output++ = *text;
    }
  }
  *output = '\0';
}

bool isNotFound(const char* text) {
  return strcasecmp(text, "NOT_FOUND") == 0 ||
         strcasecmp(text, "Book not found") == 0 ||
         strcasecmp(text, "Not found") == 0;
}

void handleLine(char* line) {
  toAscii(line);
  char* text = trim(line);
  if (*text == '\0') return;

  if (isNotFound(text)) {
    showLines("Libro no", "encontrado", 2);
    startLed(LED_RED_PIN);
    tone(BUZZER_PIN, NOT_FOUND_TONE_HZ, NOT_FOUND_TONE_MS);
    return;
  }

  char* separator = strchr(text, '|');
  if (separator) {
    *separator = '\0';
    char* title = trim(text);
    char* price = trim(separator + 1);
    showTitlePrice(title, price);
    startLed(LED_GREEN_PIN);
    tone(BUZZER_PIN, FOUND_TONE_HZ, FOUND_TONE_MS);
  } else {
    showLines(text);
  }
}

void readSerial() {
  while (Serial.available() > 0) {
    const char c = static_cast<char>(Serial.read());
    if (c == '\r') continue;

    if (c == '\n') {
      if (!lineOverflow) {
        lineBuf[lineLen] = '\0';
        handleLine(lineBuf);
      }
      lineLen = 0;
      lineOverflow = false;
    } else if (lineLen < MAX_LINE) {
      lineBuf[lineLen++] = c;
    } else {
      lineOverflow = true;
    }
  }
}

void setup() {
  Serial.begin(9600);
  pinMode(LED_GREEN_PIN, OUTPUT);
  pinMode(LED_RED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(LED_GREEN_PIN, LOW);
  digitalWrite(LED_RED_PIN, LOW);
  Wire.begin();

  for (uint8_t address : OLED_ADDRESSES) {
    Wire.beginTransmission(address);
    if (Wire.endTransmission() == 0 &&
        display.begin(SSD1306_SWITCHCAPVCC, address)) {
      oledOk = true;
      break;
    }
  }

  testInicial();
  showWaiting();
}

void loop() {
  readSerial();
  if (activeLed && static_cast<long>(millis() - ledOffAt) >= 0) {
    digitalWrite(activeLed, LOW);
    activeLed = 0;
  }
}
