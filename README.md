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
| `Título\|Precio` (ej. `El principito\|450.00`) | Título y `$Precio` | Verde 2 s | Bip agudo (2000 Hz) |
| `NOT_FOUND`, `Book not found` o `Not found` | "Libro no encontrado" | Rojo 2 s | Bip grave (300 Hz) |
| Otra línea de texto | El texto tal cual | — | — |

- Se ignoran `\r` y las líneas vacías.
- Las líneas de más de 120 caracteres se descartan.
- Solo se muestra ASCII: acentos y `ñ` se omiten (limitación de la fuente de la OLED).

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
2. Queda en espera mostrando "Scan barcode / Waiting...".
3. Al recibir un mensaje por Serial, lo muestra y activa el LED y el buzzer correspondientes.

## Compilar y cargar

Proyecto PlatformIO ([platformio.ini](platformio.ini)): placa `uno`, `upload_port` y `monitor_port` en COM5, `monitor_speed` 9600. Librerías: Adafruit SSD1306 y Adafruit GFX.

```text
pio run                  # compilar
pio run -t upload        # cargar
pio device monitor       # monitor serie
```

COM5 solo puede estar abierto por un programa a la vez: cierra el Monitor Serial antes de ejecutar `bridge.py`, y cierra el puente antes de cargar el firmware o abrir el monitor.

## Estructura

- [src/main.cpp](src/main.cpp): firmware.
- [context/arduino.md](context/arduino.md): especificación original del protocolo.
