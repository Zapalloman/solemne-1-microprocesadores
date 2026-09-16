/*
 * Solemne 1 - Grupo 1 - MQ135
 * Sensor: MQ135 (respuesta a gases/calidad de aire)
 * Conexion: AO -> A0, VCC -> 5V, GND -> GND
 *
 * IMPORTANTE (la solemne lo exige): la lectura de analogRead() es una LECTURA ADC CRUDA.
 * NO se convierte a ppm. Solo se reporta:
 *   - mq135_raw : valor ADC de 0 a 1023 (resolucion 10 bits del ATmega328P)
 *   - mq135_volt: voltaje calculado = raw * 5.0 / 1023
 *
 * El MQ135 necesita precalentamiento (burn-in). El fabricante recomienda 24-48 h
 * para estabilizar; en clases bastan unos minutos, pero la lectura deriva al inicio.
 */

#define PIN_MQ135 A0

void setup() {
  Serial.begin(115200);
  pinMode(PIN_MQ135, INPUT);
  Serial.println(F("tiempo_ms,mq135_raw,mq135_volt"));
}

void loop() {
  static uint32_t t_ultima = 0;
  if (millis() - t_ultima < 1000) return;  // una muestra por segundo
  t_ultima = millis();

  int raw = analogRead(PIN_MQ135);          // lectura cruda 0..1023 (entero)
  float voltaje = raw * (5.0 / 1023.0);     // equivalencia voltaje del ADC de 10 bits

  Serial.print(millis());
  Serial.print(',');
  Serial.print(raw);
  Serial.print(',');
  Serial.println(voltaje, 3);
}
