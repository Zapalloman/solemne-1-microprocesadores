/*
 * Solemne 1 - Grupo 1 - Sensor de sonido digital
 * Sensor: modulo de sonido con microfono electret + comparador LM393 (tipo KY-038)
 * Conexion: DO -> D3, VCC -> 5V, GND -> GND
 *
 * El modulo tiene potenciometro: gira hasta que el LED del modulo apague en silencio
 * y encienda al aplaudir/hablar fuerte. Ese umbral define la salida digital.
 *
 * Salida:
 *   - estado_sonido: 0/1 instantaneo (la mayoria de los modulos: 1 en reposo, 0 al detectar)
 *   - flancos: cantidad de cambios de estado detectados en el ultimo segundo
 *
 * Se muestrea cada 2 ms para no perder eventos cortos (un aplauso dura decenas de ms).
 */

#define PIN_SONIDO 3

void setup() {
  Serial.begin(115200);
  pinMode(PIN_SONIDO, INPUT);
  Serial.println(F("tiempo_ms,estado_sonido,flancos_ultimo_segundo"));
}

void loop() {
  static uint8_t  estado_previo = 0;
  static uint32_t flancos = 0;
  static uint32_t t_ultimo_muestreo = 0;
  static uint32_t t_ultimo_reporte = 0;

  uint32_t ahora = millis();

  // Muestreo rapido del estado digital (cada 2 ms)
  if (ahora - t_ultimo_muestreo >= 2) {
    t_ultimo_muestreo = ahora;
    uint8_t estado_actual = digitalRead(PIN_SONIDO);
    if (estado_actual != estado_previo) {
      flancos++;                 // hubo un evento acustico que supero el umbral
      estado_previo = estado_actual;
    }
  }

  // Reporte por serial una vez por segundo
  if (ahora - t_ultimo_reporte >= 1000) {
    t_ultimo_reporte = ahora;
    Serial.print(ahora);
    Serial.print(',');
    Serial.print(estado_previo);
    Serial.print(',');
    Serial.println(flancos);
    flancos = 0;
  }
}
