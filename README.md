# Solemne 1 — Grupo 1: Estación ambiental (Arduino Uno)

Código, datos de la prueba y visor para la estación ambiental con
**DHT11 + BMP180 + BH1750 + MQ135 + sensor de sonido**.

## Estructura

```
├── 01_dht11/                  DHT11: temperatura y humedad (DATA -> D2)
├── 02_bmp180/                 BMP180: presión (I2C, driver propio, 0x77)
├── 03_bh1750/                 BH1750: iluminancia en lux (I2C, driver propio)
├── 04_mq135/                  MQ135: calidad de aire (AO -> A0, ADC crudo)
├── 05_sonido/                 Sonido: micrófono + LM393 (DO -> D3)
├── 06_escaner_i2c/            Diagnóstico del bus I2C (direcciones conectadas)
├── 99_integrado_grupo1/       PROGRAMA INTEGRADO (los 5 sensores juntos)
├── datos/
│   └── csv_combinado_grupo1_20260916.csv   Datos de la prueba (16-09-2026)
└── herramientas/
    └── gui_datos.html         Visor de datos (se abre en el navegador)
```

## Compilar y subir (arduino-cli)

```bash
arduino-cli board list                                    # ver puerto
arduino-cli compile -b arduino:avr:uno 99_integrado_grupo1
arduino-cli upload  -b arduino:avr:uno -p /dev/cu.usbserial-XXXX 99_integrado_grupo1
arduino-cli monitor -p /dev/cu.usbserial-XXXX -b arduino:avr:uno --config baudrate=115200
```

Los sketches individuales se compilan igual, cambiando la carpeta.
`01_dht11` requiere la librería DHT de Adafruit (incluida en `../librerias/`).

## Datos de la prueba

`datos/csv_combinado_grupo1_20260916.csv` — 420 filas, captura verificada del
16-09-2026, con columna `segmento`:

- **A**: captura integrada de la mañana (lux y sonido reales; MQ135 con falla de conexión).
- **B**: captura individual del MQ135 ya reparado (raw 17→50 con estímulos).

Columnas: `segmento, tiempo_ms, temperatura_C, humedad_pct, presion_hPa, lux,
mq135_raw, sonido_estado, sonido_flancos_1s`. Las columnas sin sensor
funcional quedan en `nan` (no se inventaron valores).

## Visor de datos (GUI)

```bash
open herramientas/gui_datos.html
```

- Trae los datos de arriba **embebidos** (botón "Datos de ejemplo").
- **Cargar CSV**: acepta uno o varios archivos.
- Filtros por segmento (A/B) y por origen; cada traza muestra mín / máx / prom.
- **Exportar visibles**: CSV con las series desplegadas.
- **Exportar features (ML)**: valor, media móvil, desviación estándar y marca
  de dato faltante por canal.

## Estado del hardware (16-09-2026)

| Sensor | Estado |
|---|---|
| BH1750 | ✅ funciona (~45 a 1260 lx) |
| Sonido | ✅ funciona (flancos al aplaudir) |
| MQ135 | ✅ funciona (requiere conexión directa, la protoboard fallaba) |
| DHT11 | ✅ funciona(requiere conexión directa)|
| BMP180 | ❌ deja el bus I2C muerto; no soldado |
