# Librerías y referencias

Contexto de las librerías usadas en el firmware (`src/main.cpp`), para consultar más adelante.

## Adafruit SSD1306 (^2.5.9)

Driver de pantallas OLED monocromo con chip SSD1306 por I2C o SPI. Se usa con una OLED 128x64 por I2C (dirección 0x3C o 0x3D).

- Repositorio y ejemplos: https://github.com/adafruit/Adafruit_SSD1306
- Registro PlatformIO: https://registry.platformio.org/libraries/adafruit/Adafruit%20SSD1306

## Adafruit GFX Library (^1.11.9)

Librería base de gráficos de Adafruit (dependencia de SSD1306): texto, líneas, círculos, rectángulos redondeados y bitmaps.

- Guía oficial: https://learn.adafruit.com/adafruit-gfx-graphics-library
- Repositorio: https://github.com/adafruit/Adafruit-GFX-Library

Funciones usadas en el proyecto: `setTextSize`, `setCursor`, `write`, `print`, `drawCircle`, `fillCircle`, `drawRoundRect`, `drawLine`, `drawPixel`, `drawBitmap`.

## Notas

- La fuente integrada solo soporta ASCII (sin acentos ni ñ); cada carácter mide 6x8 px multiplicado por el tamaño de texto.
- Las imágenes se guardan en memoria flash con `PROGMEM` para ahorrar RAM (la Uno tiene solo 2 KB).
