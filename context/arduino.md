# Especificación para el firmware Arduino (VS Code + PlatformIO)

Implementa el firmware para recibir por el puerto USB/Serial los resultados que envía el programa puente de la computadora y mostrarlos en una pantalla LCD 16x2 I2C.

## Origen de los datos

El Arduino **no se conecta directamente a MySQL ni a `search.php`**. El flujo existente es:

```text
Base de datos -> search.php -> bridge.py en la computadora -> Serial USB -> Arduino
```

El puente existente está en `info arduino/bridge.py`. Después de consultar la API, escribe al puerto Serial el resultado como texto UTF-8 terminado en salto de línea (`\n`). El puente usa **9600 baudios**.

## Configuración PlatformIO

Configura `platformio.ini` para la placa Arduino que está conectada. El puerto solicitado para cargar y monitorear el firmware es **COM5**; la velocidad Serial es **9600**. Para una placa Uno, la configuración correspondiente de puerto y monitor es:

```ini
upload_port = COM5
monitor_port = COM5
monitor_speed = 9600
```

Selecciona en `platformio.ini` la plataforma, placa y framework que correspondan al hardware real. No supongas que la placa es Uno si no está confirmada.

## Protocolo Serial de entrada

Procesa un mensaje completo cada vez que recibas `\n`. Ignora `\r`, que puede venir como parte de un fin de línea Windows. Los mensajes que envía el puente son:

```text
El principito|450.00
Book not found
```

- En el primer formato, `|` separa el título y el precio. Recorta espacios de ambos campos.
- En `Book not found` (también puede aceptarse `Not found`), muestra el mensaje de no encontrado.
- Si llega una línea no vacía sin `|` y no es un mensaje de no encontrado, muéstrala en la primera línea como texto simple.
- Ignora líneas vacías.
- El puente limita la línea a 120 caracteres. Evita que el buffer de recepción crezca sin límite.
- Los títulos y mensajes pueden exceder los 16 caracteres visibles por renglón; recórtalos a 16 para la LCD.

## Comportamiento de la pantalla

Usa una LCD de **16 columnas x 2 filas** con interfaz I2C y la biblioteca `LiquidCrystal_I2C`.

- Al iniciar: primera fila `Scan barcode`, segunda fila `Waiting...`.
- Resultado encontrado: título en la primera fila y `$` seguido del precio en la segunda.
- Sin resultado: `Book not found` en la primera fila y la segunda vacía.
- Limpia la pantalla o sobrescribe los renglones al mostrar cada nuevo resultado, para que no queden caracteres de mensajes anteriores.

La dirección I2C habitual es `0x27`; si el hardware usa otra dirección, permite ajustarla (a menudo `0x3F`). Usa `Serial.begin(9600)` para que coincida con el puente y con `monitor_speed`.

## Puerto ocupado

COM5 solo puede estar abierto por un programa a la vez. Cierra el Monitor Serial de PlatformIO antes de ejecutar `bridge.py`; cierra el puente antes de abrir el Monitor Serial o cargar el firmware. El monitor sirve para depurar, pero no es el receptor que muestra los datos en la LCD: el firmware los lee desde `Serial`.
