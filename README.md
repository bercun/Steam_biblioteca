# Steam_biblioteca

Firmware Arduino para una biblioteca: muestra en una pantalla OLED el resultado de la búsqueda de un libro hecha desde una página web, con señales de LED y buzzer.

## Flujo

```text
Página web -> search.php (base de datos) -> bridge.py (PC) -> Serial USB (9600) -> Arduino
```

El Arduino no se conecta a la base de datos: solo recibe por Serial una línea de texto terminada en `\n`.

## Protocolo Serial (9600 baudios)

| Mensaje | Pantalla | LED | Buzzer |
|---|---|---|---|
| `Título\|Precio` (ej. `El principito\|450.00`) | Título en letra grande (ajustado por palabras; letra chica si es muy largo) y `$Precio` | Verde 2 s | Bip agudo (2000 Hz) |
| `NOT_FOUND`, `Book not found` o `Not found` | Animación de carita triste y luego "Libro no encontrado" | Rojo 2 s | Bip grave (300 Hz) |
| Otra línea de texto | El texto tal cual | — | — |

- Se ignoran `\r` y las líneas vacías.
- Las líneas de más de 120 caracteres se descartan.
- Solo se muestra ASCII: acentos y `ñ` se omiten (limitación de la fuente de la OLED).
- Cada resultado se muestra 5 segundos y luego vuelve a "Scan barcode / Waiting...".

## Hardware

| Componente | Conexión |
|---|---|
| Arduino Uno | USB, puerto COM5 |
| OLED SSD1306 128x64 (I2C, dirección 0x3C o 0x3D) | SDA/SCL (A4/A5) |
| LED verde | D2 |
| LED rojo | D3 |
| Buzzer | D4 |

## Funcionamiento

1. Al encender, hace un test inicial: pantalla, LED verde, LED rojo y buzzer.
2. Reproduce la animación de inicio: las letras de "Vortex 4" caen y rebotan, se muestra el logo del puzzle y las piezas laten.
3. Queda en espera mostrando "Scan barcode / Waiting...". En espera, cada 10 segundos muestra el logo del puzzle durante 1 segundo.
4. Al recibir un mensaje por Serial, lo muestra durante 5 segundos y activa el LED y el buzzer correspondientes; luego vuelve a la espera.

## Compilar y cargar

Proyecto PlatformIO ([platformio.ini](platformio.ini)): placa `uno`, `upload_port` y `monitor_port` en COM5, `monitor_speed` 9600. Librerías: Adafruit SSD1306 y Adafruit GFX.

```text
pio run                  # compilar
pio run -t upload        # cargar
pio device monitor       # monitor serie
```

COM5 solo puede estar abierto por un programa a la vez: cierra el Monitor Serial antes de ejecutar `bridge.py`, y cierra el puente antes de cargar el firmware o abrir el monitor.

## Librerías utilizadas

| Librería | Versión | Para qué se usa |
|---|---|---|
| Adafruit SSD1306 | ^2.5.9 | Controlar la pantalla OLED SSD1306 128x64 por I2C (`display.display()`, `invertDisplay()`, etc.). |
| Adafruit GFX Library | ^1.11.9 | Dibujo en pantalla: texto (`setTextSize`, `setCursor`), formas (`drawCircle`, `drawRoundRect`, `drawLine`) y bitmaps (`drawBitmap`, usado para el logo del puzzle). |
| Wire (incluida en Arduino) | — | Comunicación I2C con la OLED. |

Se declaran en `lib_deps` de [platformio.ini](platformio.ini).

### Más información

- Adafruit SSD1306 (repositorio y ejemplos): https://github.com/adafruit/Adafruit_SSD1306
- Adafruit GFX, guía oficial con todas las funciones de dibujo y texto: https://learn.adafruit.com/adafruit-gfx-graphics-library
- Adafruit GFX (repositorio): https://github.com/adafruit/Adafruit-GFX-Library
- Registro de PlatformIO: https://registry.platformio.org/libraries/adafruit/Adafruit%20SSD1306

## Includes de main.cpp

| Include | Para qué se usa |
|---|---|
| `<Arduino.h>` | Núcleo de Arduino: `setup`/`loop`, `Serial`, `pinMode`, `digitalWrite`, `delay`, `millis`, `tone`, `PROGMEM` y tipos como `uint8_t`. |
| `<Wire.h>` | Bus I2C. Se usa para detectar la OLED (`Wire.beginTransmission`) y es la base de la comunicación con la pantalla. |
| `<Adafruit_GFX.h>` | Funciones de dibujo: texto, círculos, líneas, rectángulos y bitmaps. |
| `<Adafruit_SSD1306.h>` | Driver de la pantalla: crea el objeto `display`, lo inicializa y envía el contenido con `display.display()`. |
| `<string.h>` | Funciones de texto para procesar el mensaje recibido: `strlen`, `strchr` (buscar `\|`) y `strcasecmp` (comparar sin distinguir mayúsculas). |

## Estructura

- [src/main.cpp](src/main.cpp): firmware.
- [context/arduino.md](context/arduino.md): especificación original del protocolo.
- [context/librerias.md](context/librerias.md): librerías y enlaces de referencia.
