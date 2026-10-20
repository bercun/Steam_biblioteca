// Biblioteca Steam: recibe por Serial "Titulo|Precio" o "NOT_FOUND"
// y lo muestra en una OLED con LEDs y buzzer.
#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
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
constexpr unsigned long RESULT_SHOW_MS = 5000;
unsigned long waitingAt = 0;
bool waitingPending = false;

constexpr unsigned long LOGO_INTERVAL_MS = 10000;
constexpr unsigned long LOGO_DURATION_MS = 1000;
bool idle = false;
bool logoOn = false;
unsigned long idleSince = 0;
unsigned long logoUntil = 0;

void scheduleWaiting() {
  idle = false;
  logoOn = false;
  waitingAt = millis() + RESULT_SHOW_MS;
  waitingPending = true;
}

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

// Imprime con salto de linea por palabras (o por caracteres si la palabra no cabe).
// Devuelve false si no cabe en maxLines.
bool printWrapped(const char* text, uint8_t size, int cols, int maxLines) {
  const int lineH = 8 * size;
  const int charW = 6 * size;
  int line = 0;
  int col = 0;
  const char* p = text;
  while (*p) {
    if (*p == ' ') {
      if (col > 0 && col < cols) { col++; }
      p++;
      continue;
    }
    const char* e = p;
    while (*e && *e != ' ') e++;
    int wordLen = e - p;
    if (col > 0 && col + wordLen > cols) {
      line++;
      col = 0;
    }
    for (; p < e; p++) {
      if (col >= cols) {
        line++;
        col = 0;
      }
      if (line >= maxLines) return false;
      display.setCursor(col * charW, line * lineH);
      display.write(*p);
      col++;
    }
  }
  return true;
}

void showTitlePrice(const char* title, const char* price) {
  if (!oledOk) return;
  char priceLine[24];
  snprintf(priceLine, sizeof(priceLine), "$%s", price);
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  // Titulo en letra grande (3 lineas de 10 columnas); si no cabe, letra chica.
  display.setTextSize(2);
  if (!printWrapped(title, 2, 10, 3)) {
    display.clearDisplay();
    display.setTextSize(1);
    printWrapped(title, 1, 21, 6);
  }

  display.setTextSize(2);
  display.setCursor(0, 48);
  display.print(priceLine);
  display.display();
}

void showWaiting() {
  showLines("Scan barcode", "Waiting...", 2);
}

// ---- Animacion de inicio (tomada del proyecto test pantalla oled) ----
constexpr int SCREEN_WIDTH = 128;
constexpr int SCREEN_HEIGHT = 64;
constexpr int IMAGE_SIZE = 64;
const unsigned char PUZZLE_IMAGE[] PROGMEM = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x1F, 0xFF, 0xFF, 0xF8, 0x1F, 0xFF, 0xFF, 0xF8,
  0x30, 0x00, 0x00, 0x06, 0x30, 0x00, 0x00, 0x0C,
  0x60, 0x00, 0x00, 0x02, 0x60, 0x00, 0x00, 0x02,
  0x40, 0x00, 0x00, 0x03, 0x40, 0x00, 0x00, 0x03,
  0x40, 0x00, 0x00, 0x01, 0x40, 0x00, 0x00, 0x03,
  0x40, 0x00, 0x00, 0x01, 0x40, 0x00, 0x00, 0x01,
  0x40, 0x00, 0x00, 0x01, 0x40, 0x00, 0x00, 0x01,
  0x40, 0x00, 0x00, 0x01, 0x40, 0x00, 0x00, 0x01,
  0x40, 0x00, 0x00, 0x01, 0xC0, 0x00, 0x00, 0x01,
  0x40, 0x00, 0x00, 0x3D, 0xC0, 0x00, 0x00, 0x01,
  0x40, 0x00, 0x00, 0x77, 0x80, 0x00, 0x00, 0x01,
  0x40, 0x00, 0x00, 0xFF, 0x80, 0x00, 0x00, 0x01,
  0x40, 0x00, 0x01, 0xE0, 0x00, 0x00, 0x00, 0x01,
  0x40, 0x00, 0x01, 0xC0, 0x00, 0x00, 0x00, 0x01,
  0x40, 0x00, 0x01, 0xC0, 0x00, 0x00, 0x00, 0x01,
  0x40, 0x00, 0x01, 0x80, 0x00, 0x00, 0x00, 0x01,
  0x40, 0x00, 0x01, 0xC0, 0x00, 0x00, 0x00, 0x01,
  0x40, 0x00, 0x01, 0xE0, 0x00, 0x00, 0x00, 0x01,
  0x40, 0x00, 0x00, 0xFF, 0x80, 0x00, 0x00, 0x01,
  0x40, 0x00, 0x00, 0xF7, 0x80, 0x00, 0x00, 0x01,
  0x40, 0x00, 0x00, 0x3F, 0xC0, 0x00, 0x00, 0x01,
  0x40, 0x00, 0x00, 0x01, 0xC0, 0x03, 0xC0, 0x01,
  0x40, 0x00, 0x00, 0x01, 0xC0, 0x0F, 0xF0, 0x01,
  0x40, 0x00, 0x00, 0x01, 0xC0, 0x1F, 0xF8, 0x01,
  0x40, 0x00, 0x00, 0x01, 0xC0, 0x1C, 0x3C, 0x01,
  0x40, 0x00, 0x00, 0x01, 0xC0, 0x38, 0x1C, 0x01,
  0x40, 0x00, 0x00, 0x01, 0x40, 0x38, 0x14, 0x03,
  0x40, 0x00, 0x00, 0x03, 0x40, 0x38, 0x1C, 0x02,
  0x60, 0x00, 0x00, 0x02, 0x60, 0x1C, 0x1C, 0x06,
  0x38, 0x00, 0x00, 0x0C, 0x38, 0x1C, 0x1C, 0x1C,
  0x0F, 0xF8, 0x0F, 0xF8, 0x0F, 0xF8, 0x1F, 0xF8,
  0x0F, 0xF8, 0x1F, 0xF8, 0x0F, 0xF8, 0x0F, 0xFC,
  0x38, 0x3C, 0x1C, 0x1C, 0x38, 0x00, 0x00, 0x1E,
  0x20, 0x1C, 0x1C, 0x06, 0x20, 0x00, 0x00, 0x07,
  0x40, 0x18, 0x1C, 0x03, 0x40, 0x00, 0x00, 0x02,
  0x40, 0x18, 0x0C, 0x01, 0x40, 0x00, 0x00, 0x03,
  0x40, 0x1C, 0x1C, 0x01, 0xC0, 0x00, 0x00, 0x01,
  0x40, 0x1C, 0x3C, 0x01, 0xC0, 0x00, 0x00, 0x01,
  0x40, 0x1F, 0xF8, 0x01, 0xC0, 0x00, 0x00, 0x01,
  0x40, 0x0E, 0x30, 0x01, 0xC0, 0x00, 0x00, 0x01,
  0x40, 0x03, 0xE0, 0x01, 0xC0, 0x00, 0x00, 0x01,
  0x40, 0x00, 0x00, 0x01, 0xFE, 0x00, 0x00, 0x01,
  0x40, 0x00, 0x00, 0x01, 0x9F, 0x00, 0x00, 0x01,
  0x40, 0x00, 0x00, 0x00, 0xFF, 0x80, 0x00, 0x01,
  0x40, 0x00, 0x00, 0x00, 0x03, 0x80, 0x00, 0x01,
  0x40, 0x00, 0x00, 0x00, 0x01, 0xC0, 0x00, 0x01,
  0x40, 0x00, 0x00, 0x00, 0x01, 0x40, 0x00, 0x01,
  0x40, 0x00, 0x00, 0x00, 0x01, 0x40, 0x00, 0x01,
  0x40, 0x00, 0x00, 0x00, 0x01, 0xC0, 0x00, 0x01,
  0x40, 0x00, 0x00, 0x00, 0x63, 0x80, 0x00, 0x01,
  0x40, 0x00, 0x00, 0x00, 0xFD, 0x80, 0x00, 0x01,
  0x40, 0x00, 0x00, 0x01, 0xFF, 0x00, 0x00, 0x01,
  0x40, 0x00, 0x00, 0x01, 0xC0, 0x00, 0x00, 0x01,
  0x40, 0x00, 0x00, 0x01, 0xC0, 0x00, 0x00, 0x01,
  0x40, 0x00, 0x00, 0x01, 0xC0, 0x00, 0x00, 0x01,
  0x40, 0x00, 0x00, 0x01, 0xC0, 0x00, 0x00, 0x01,
  0x40, 0x00, 0x00, 0x01, 0xC0, 0x00, 0x00, 0x01,
  0x40, 0x00, 0x00, 0x01, 0xC0, 0x00, 0x00, 0x03,
  0x40, 0x00, 0x00, 0x03, 0x40, 0x00, 0x00, 0x03,
  0x60, 0x00, 0x00, 0x02, 0x60, 0x00, 0x00, 0x06,
  0x30, 0x00, 0x00, 0x0E, 0x30, 0x00, 0x00, 0x0C,
  0x1F, 0xFF, 0xFF, 0xF8, 0x1F, 0xFF, 0xFF, 0xF8,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

void showImage() {
  display.clearDisplay();
  display.drawBitmap((SCREEN_WIDTH - IMAGE_SIZE) / 2, (SCREEN_HEIGHT - IMAGE_SIZE) / 2,
                     PUZZLE_IMAGE, IMAGE_SIZE, IMAGE_SIZE, SSD1306_WHITE);
  display.display();
}

void drawPuzzleConnector(int x, int y, int radius) {
  display.fillCircle(x, y, radius, SSD1306_BLACK);
  display.drawCircle(x, y, radius, SSD1306_WHITE);
}

void showPuzzle(int radius, int offset) {
  constexpr int margin = 2;
  constexpr int gap = 4;
  constexpr int tileWidth = (SCREEN_WIDTH - 2 * margin - gap) / 2;
  const int tileHeight = (SCREEN_HEIGHT - 2 * margin - gap) / 2;
  const int leftX = margin;
  const int rightX = margin + tileWidth + gap;
  const int topY = margin;
  const int bottomY = margin + tileHeight + gap;
  const int centerX = leftX + tileWidth + gap / 2;
  const int centerY = topY + tileHeight + gap / 2;

  display.clearDisplay();
  display.drawRoundRect(leftX + offset, topY + offset, tileWidth, tileHeight, 7, SSD1306_WHITE);
  display.drawRoundRect(rightX - offset, topY + offset, tileWidth, tileHeight, 7, SSD1306_WHITE);
  display.drawRoundRect(leftX + offset, bottomY - offset, tileWidth, tileHeight, 7, SSD1306_WHITE);
  display.drawRoundRect(rightX - offset, bottomY - offset, tileWidth, tileHeight, 7, SSD1306_WHITE);

  drawPuzzleConnector(centerX, topY + tileHeight / 2, radius);
  drawPuzzleConnector(centerX, bottomY + tileHeight / 2, radius);
  drawPuzzleConnector(leftX + tileWidth / 2, centerY, radius);
  drawPuzzleConnector(rightX + tileWidth / 2, centerY, radius);
  display.display();
}

void showName() {
  const char text[] = "Vortex 4";
  constexpr int len = sizeof(text) - 1;
  constexpr int charW = 12;
  constexpr int charH = 16;
  constexpr int stagger = 3;
  constexpr int dropFrames = 12;
  const int startX = (SCREEN_WIDTH - len * charW) / 2;
  const int baseY = (SCREEN_HEIGHT - charH) / 2;

  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);

  // Cada letra cae desde arriba y rebota al llegar a su lugar.
  for (int f = 0; f < stagger * len + dropFrames + 10; f++) {
    display.clearDisplay();
    for (int i = 0; i < len; i++) {
      const int t = f - i * stagger;
      if (t < 0) continue;
      int y = baseY;
      if (t < dropFrames) {
        const float p = (float)t / dropFrames;
        y = baseY - (int)((1 - p * p) * (baseY + charH));
      } else if (t < dropFrames + 6) {
        y = baseY - (int)(6 * sin((t - dropFrames) * 0.52f));
      }
      display.setCursor(startX + i * charW, y);
      display.write(text[i]);
    }
    display.display();
    delay(35);
  }

  for (int k = 0; k < 2; k++) {
    display.invertDisplay(true);
    delay(120);
    display.invertDisplay(false);
    delay(120);
  }
}

// Carita triste: la boca se curva hacia abajo y cae una lagrima.
void drawSadFace(int mouthStep, int tearY) {
  constexpr int cx = SCREEN_WIDTH / 2;
  constexpr int cy = SCREEN_HEIGHT / 2;
  display.clearDisplay();
  display.drawCircle(cx, cy, 30, SSD1306_WHITE);
  display.fillCircle(cx - 11, cy - 8, 3, SSD1306_WHITE);
  display.fillCircle(cx + 11, cy - 8, 3, SSD1306_WHITE);
  // Cejas inclinadas.
  display.drawLine(cx - 17, cy - 17, cx - 7, cy - 13, SSD1306_WHITE);
  display.drawLine(cx + 17, cy - 17, cx + 7, cy - 13, SSD1306_WHITE);
  // Boca: arco hacia abajo que se va formando (mouthStep 0..8).
  for (int dx = -mouthStep; dx <= mouthStep; dx++) {
    const int dy = (dx * dx) / 8;
    display.drawPixel(cx + dx, cy + 14 + dy, SSD1306_WHITE);
    display.drawPixel(cx + dx, cy + 15 + dy, SSD1306_WHITE);
  }
  if (tearY >= 0) {
    display.fillCircle(cx - 11, cy - 3 + tearY, 2, SSD1306_WHITE);
  }
  display.display();
}

void animacionNoEncontrado() {
  if (!oledOk) return;
  for (int step = 0; step <= 8; step++) {
    drawSadFace(step, -1);
    delay(50);
  }
  for (int t = 0; t < 14; t++) {
    drawSadFace(8, t * 2);
    delay(50);
  }
}

void animacionInicio() {
  if (!oledOk) return;
  showName();
  delay(800);
  showImage();
  delay(1500);
  for (int frame = 0; frame < 50; frame++) {
    const float phase = frame * 0.25f;
    const int radius = 5 + (int)(3 * (sin(phase) + 1) / 2);
    const int offset = (int)(3 * (sin(phase) + 1) / 2);
    showPuzzle(radius, offset);
    delay(40);
  }
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
    startLed(LED_RED_PIN);
    tone(BUZZER_PIN, NOT_FOUND_TONE_HZ, NOT_FOUND_TONE_MS);
    animacionNoEncontrado();
    showLines("Libro no", "encontrado", 2);
    scheduleWaiting();
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
  scheduleWaiting();
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
  animacionInicio();
  showWaiting();
  idle = true;
  idleSince = millis();
}

void loop() {
  readSerial();
  if (waitingPending && static_cast<long>(millis() - waitingAt) >= 0) {
    waitingPending = false;
    showWaiting();
    idle = true;
    idleSince = millis();
  }
  if (idle && oledOk) {
    const unsigned long now = millis();
    if (!logoOn && now - idleSince >= LOGO_INTERVAL_MS) {
      showImage();
      logoOn = true;
      logoUntil = now + LOGO_DURATION_MS;
    } else if (logoOn && static_cast<long>(now - logoUntil) >= 0) {
      showWaiting();
      logoOn = false;
      idleSince = now;
    }
  }
  if (activeLed && static_cast<long>(millis() - ledOffAt) >= 0) {
    digitalWrite(activeLed, LOW);
    activeLed = 0;
  }
}
