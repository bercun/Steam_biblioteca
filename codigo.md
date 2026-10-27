# Guía del código del firmware

Este documento explica los fragmentos más importantes del firmware de [src/main.cpp](src/main.cpp): cómo recibe los datos, cómo interpreta el resultado y cómo lo presenta en la pantalla OLED.

## Flujo general

El Arduino no consulta directamente una base de datos ni una página web. Recibe el resultado que otro programa envía por el puerto USB/Serial:

```text
Página web / base de datos -> programa puente en la PC -> Serial USB -> Arduino -> OLED, LED y buzzer
```

La comunicación Serial está configurada a **9600 baudios**. Cada mensaje termina con un salto de línea (`\n`). Por ejemplo:

```text
El principito|450.00\n
NOT_FOUND\n
```

En un resultado encontrado, el carácter `|` separa el título del precio. Un mensaje de no encontrado puede ser `NOT_FOUND`, `Book not found` o `Not found`.

## 1. Librerías y configuración

Al principio de `main.cpp` se incluyen las librerías para el hardware:

```cpp
#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <string.h>
```

- `Arduino.h` proporciona funciones como `Serial`, `pinMode`, `digitalWrite`, `millis()` y `tone()`.
- `Wire.h` permite comunicarse con la pantalla mediante I2C.
- `Adafruit_GFX.h` aporta las funciones para dibujar texto, figuras y bitmaps.
- `Adafruit_SSD1306.h` controla la pantalla OLED.
- `string.h` se usa para manipular y comparar las cadenas recibidas.

Los pines, direcciones I2C y límites principales se declaran como constantes:

```cpp
constexpr uint8_t LED_GREEN_PIN = 2;
constexpr uint8_t LED_RED_PIN = 3;
constexpr uint8_t BUZZER_PIN = 4;
constexpr uint8_t OLED_ADDRESSES[] = {0x3C, 0x3D};
constexpr size_t MAX_LINE = 120;
```

La pantalla intenta inicializarse en las direcciones `0x3C` y `0x3D`. El buffer puede guardar como máximo 120 caracteres de una línea. La placa, las dependencias y la velocidad Serial se configuran en [platformio.ini](platformio.ini).

## 2. Inicio del firmware: `setup()`

Arduino ejecuta `setup()` una vez al encender o reiniciar:

```cpp
void setup() {
  Serial.begin(9600);
  pinMode(LED_GREEN_PIN, OUTPUT);
  pinMode(LED_RED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  // Inicializa I2C y busca la OLED en sus direcciones posibles.
  ...
  testInicial();
  animacionInicio();
  showWaiting();
}
```

Primero inicia Serial a la misma velocidad que el programa puente y configura LED y buzzer como salidas. Después inicializa I2C y busca la pantalla. Si la encuentra, ejecuta una prueba de hardware, la animación de inicio y finalmente muestra `Scan barcode / Waiting...`.

## 3. Cómo recibe la información: `readSerial()`

La función `loop()` llama repetidamente a `readSerial()`. Esta función consume los bytes disponibles en Serial y construye una línea en `lineBuf`:

```cpp
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
```

- `Serial.available()` indica si llegaron bytes.
- `\r` se ignora, para aceptar finales de línea de Windows (`\r\n`).
- `\n` marca el final del mensaje. Se agrega `\0` para que el buffer pueda tratarse como una cadena C y se entrega a `handleLine()`.
- Si la línea supera los 120 caracteres, se marca como desbordada y se descarta al llegar al salto de línea. Así se evita escribir fuera del buffer.
- Al terminar cada línea, se reinician el contador y la marca de desbordamiento.

Por eso el programa que envía los datos debe terminar cada resultado con un salto de línea; sin él, el Arduino no procesa todavía la línea.

## 4. Cómo interpreta la línea: `handleLine()`

`handleLine()` prepara y clasifica el mensaje antes de dibujarlo:

1. `toAscii()` elimina bytes que no son ASCII. La fuente de la OLED usada aquí no muestra correctamente caracteres UTF-8 como acentos o `ñ`.
2. `trim()` quita espacios y tabulaciones al principio y al final.
3. Si la línea está vacía, se ignora.
4. `isNotFound()` comprueba los textos de no encontrado sin distinguir mayúsculas y minúsculas.
5. Si no es un error, `strchr(text, '|')` busca el separador entre título y precio.

La decisión principal es equivalente a:

```cpp
if (isNotFound(text)) {
  // LED rojo, tono grave, animación triste y mensaje de error.
} else if (strchr(text, '|')) {
  // Separa título y precio; muestra ambos y activa LED verde y tono agudo.
} else {
  // Muestra la línea recibida como texto simple.
}
```

Para un título y precio, el código reemplaza temporalmente `|` con `\0`. Así obtiene dos cadenas independientes: el título antes del separador y el precio después. Luego quita los espacios sobrantes de cada campo.

## 5. Cómo muestra la información

### Resultado encontrado

`showTitlePrice(title, price)` limpia el buffer de la pantalla, dibuja el título y el precio, y finalmente actualiza físicamente la OLED:

```cpp
display.clearDisplay();
display.setTextColor(SSD1306_WHITE);
// El título se ajusta a varias líneas.
...
display.setCursor(0, 48);
display.print(priceLine);
display.display();
```

`printWrapped()` ajusta el título a las dimensiones de la pantalla, intentando mantener las palabras juntas. Primero prueba con texto de tamaño 2; si no cabe en las líneas disponibles, limpia la pantalla y lo intenta con tamaño 1. El precio se prepara con el símbolo `$` y se dibuja en la parte inferior.

Después de mostrar un resultado encontrado, `handleLine()` activa el LED verde durante 2 segundos y emite un tono agudo. El resultado permanece en pantalla durante 5 segundos.

### Resultado no encontrado

Si el mensaje coincide con alguno de los textos aceptados, el firmware:

- activa el LED rojo durante 2 segundos;
- reproduce un tono grave;
- anima una cara triste;
- muestra `Libro no / encontrado`;
- programa el regreso a la pantalla de espera después de 5 segundos.

### Texto simple

Si la línea no está vacía, no es un error conocido y no contiene `|`, se muestra con `showLines(text)`. Este formato sirve para enviar un mensaje sencillo sin precio.

### Funciones generales de dibujo

`showLines(l1, l2, size2)` es una función reutilizable para limpiar la pantalla y escribir hasta dos líneas. `showWaiting()` la utiliza para presentar `Scan barcode` y `Waiting...`.

El patrón básico para dibujar con la biblioteca Adafruit es:

```cpp
display.clearDisplay();                 // Borra el buffer gráfico
display.setTextColor(SSD1306_WHITE);     // Color del texto
display.setCursor(0, 0);                 // Posición inicial
display.println("Texto");               // Dibuja en el buffer
display.display();                      // Envía el buffer a la pantalla
```

Es importante llamar a `display.display()` al final: las operaciones de texto y dibujo modifican primero un buffer en memoria.

## 6. Qué hace `loop()`

`loop()` se repite continuamente mientras la placa está encendida. Sus tareas principales son:

1. Llamar a `readSerial()` para procesar los datos entrantes.
2. Comprobar si ya pasaron los 5 segundos del resultado y volver a la pantalla de espera.
3. Durante la espera, mostrar el logo del rompecabezas brevemente cada 10 segundos.
4. Apagar el LED activo cuando se cumplen sus 2 segundos.

Los tiempos de espera y del LED se controlan con `millis()` en vez de detener el programa con `delay()` para esas tareas, por lo que el ciclo puede seguir atendiendo Serial. Las animaciones sí usan pausas con `delay()` y bloquean temporalmente el ciclo mientras se ejecutan.

## 7. Animaciones y señales

El bitmap `PUZZLE_IMAGE` contiene los píxeles del logo y se guarda con `PROGMEM` en la memoria flash para ahorrar la poca RAM disponible en un Arduino Uno. `showImage()` lo dibuja centrado en la OLED. Otras funciones dibujan las piezas del rompecabezas, el nombre `Vortex 4` y la cara triste.

`startLed(pin)` apaga ambos LED, enciende el indicado y registra cuándo debe apagarse. La frecuencia y duración de cada sonido se definen en las constantes de tono y se reproducen con `tone()`.

## Resumen del recorrido de un resultado

Para un mensaje como `El principito|450.00\n`, el recorrido es:

```text
Serial USB
  -> readSerial() reúne bytes hasta \n
  -> handleLine() limpia y separa título/precio
  -> showTitlePrice() dibuja en el buffer OLED
  -> display.display() actualiza la pantalla
  -> LED verde y buzzer indican que se encontró el libro
  -> loop() vuelve a la pantalla de espera al cumplirse 5 segundos
```
