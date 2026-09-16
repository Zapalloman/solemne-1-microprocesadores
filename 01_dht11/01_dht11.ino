/*
 * Solemne 1 - Grupo 1 - DHT11
 * Sensor: DHT11 (temperatura y humedad relativa)
 * Conexion: DATA -> D2, VCC -> 5V, GND -> GND
 *
 * Requiere la libreria "DHT sensor library" (Adafruit) + "Adafruit Unified Sensor".
 *
 * Que muestra por serial:
 *   - tiempo_ms: milisegundos desde que arranco el programa (lectura cruda de millis())
 *   - temperatura_C: variable fisica calculada por la libreria (grados Celsius)
 *   - humedad_pct: variable fisica calculada por la libreria (porcentaje de humedad relativa)
 *
 * Nota: el DHT11 entrega internamente un dato DIGITAL de 40 bits
 * (8 humedad entera + 8 humedad decimal + 8 temperatura entera + 8 decimal + 8 checksum).
 * No hay "voltaje" que medir: la libreria decodifica ese protocolo de un solo cable.
 */

#include <DHT.h>

#define PIN_DHT   2        // pin de datos del DHT11
#define TIPO_DHT  DHT11    // modelo del sensor

DHT dht(PIN_DHT, TIPO_DHT);

void setup() {
  Serial.begin(115200);
  dht.begin();

  // Fila de encabezado (solo una vez, al inicio)
  Serial.println(F("tiempo_ms,temperatura_C,humedad_pct"));
}

void loop() {
  // El DHT11 no acepta lecturas mas rapidas que 1 vez por segundo (datasheet Aosong).
  // Usamos 2 s para dar margen y evitar lecturas invalidas.
  static uint32_t t_ultima = 0;
  if (millis() - t_ultima < 2000) return;
  t_ultima = millis();

  float humedad = dht.readHumidity();       // %RH
  float temperatura = dht.readTemperature(); // °C (sin argumento = Celsius)

  // Si la lectura falla, la libreria devuelve NaN.
  // NO se imprime fila de datos: se informa el error como comentario.
  if (isnan(humedad) || isnan(temperatura)) {
    Serial.println(F("# DHT11: lectura invalida. Revisar DATA en D2, alimentacion y GND comun."));
    return;
  }

  Serial.print(millis());
  Serial.print(',');
  Serial.print(temperatura, 1);
  Serial.print(',');
  Serial.println(humedad, 1);
}
