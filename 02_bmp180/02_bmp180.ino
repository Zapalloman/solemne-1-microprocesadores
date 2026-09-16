/*
 * Solemne 1 - Grupo 1 - BMP180 (presion atmosferica, bus I2C)
 * Conexion: VCC -> 5V (modulo GY-68 trae regulador y adaptacion de nivel),
 *           GND -> GND, SDA -> A4, SCL -> A5
 *
 * NO requiere libreria externa: se implementa la lectura directa por I2C con Wire,
 * siguiendo el algoritmo de compensacion del datasheet Bosch BMP180.
 *
 * Registros usados:
 *   0xAA..0xBF : 11 coeficientes de calibracion (22 bytes), propios de cada chip
 *   0xD0       : chip ID (BMP180 = 0x55)
 *   0xF4       : registro de control (inicia conversion)
 *   0xF6..0xF8 : resultado de la conversion
 *
 * Salida:
 *   - presion_hPa : variable fisica (hectopascales)
 *   - bmp_temp_C  : temperatura interna del chip (se usa para compensar la presion)
 */

#include <Wire.h>

#define DIR_BMP180 0x77   // direccion I2C fija del BMP180

// Coeficientes de calibracion (se leen del chip al inicio)
int32_t ac1, ac2, ac3, b1, b2, mb, mc, md;
uint32_t ac4, ac5, ac6;

int32_t b5;   // valor intermedio de temperatura, necesario para compensar presion

// ---------- Utilidades I2C ----------
uint16_t leer16(uint8_t registro) {
  Wire.beginTransmission(DIR_BMP180);
  Wire.write(registro);
  Wire.endTransmission();
  Wire.requestFrom((uint8_t)DIR_BMP180, (uint8_t)2);
  uint16_t valor = ((uint16_t)Wire.read() << 8) | Wire.read();
  return valor;
}

// Lee temperatura cruda UT y calcula la temperatura compensada (0.1 °C)
int32_t leerTemperatura() {
  Wire.beginTransmission(DIR_BMP180);
  Wire.write(0xF4);
  Wire.write(0x2E);            // orden: medir temperatura
  Wire.endTransmission();
  delay(5);                    // el datasheet indica 4.5 ms maximos

  int32_t ut = (int32_t)leer16(0xF6);

  int32_t x1 = ((ut - (int32_t)ac6) * (int32_t)ac5) >> 15;
  int32_t x2 = ((int32_t)mc * 2048) / (x1 + md);
  b5 = x1 + x2;
  return (b5 + 8) >> 4;        // unidad: 0.1 °C
}

// Lee presion cruda UP (sobremuestreo oss=3, el mas preciso) y la compensa -> Pa
int32_t leerPresion() {
  const uint8_t oss = 3;

  Wire.beginTransmission(DIR_BMP180);
  Wire.write(0xF4);
  Wire.write(0x34 | (oss << 6));  // orden: medir presion, oversampling 3
  Wire.endTransmission();
  delay(26);                      // oss=3 -> 25.5 ms maximos

  Wire.beginTransmission(DIR_BMP180);
  Wire.write(0xF6);
  Wire.endTransmission();
  Wire.requestFrom((uint8_t)DIR_BMP180, (uint8_t)3);
  int32_t up = ((int32_t)Wire.read() << 16) | ((int32_t)Wire.read() << 8) | Wire.read();
  up = up >> (8 - oss);

  // ---- Algoritmo de compensacion del datasheet (aritmetica entera) ----
  int32_t x1, x2, x3, b3, b6;
  uint32_t b4, b7;

  b6 = b5 - 4000;
  x1 = ((int32_t)b2 * (b6 * b6 >> 12)) >> 11;
  x2 = ((int32_t)ac2 * b6) >> 11;
  x3 = x1 + x2;
  b3 = ((((int32_t)ac1 * 4 + x3) << oss) + 2) >> 2;
  x1 = ((int32_t)ac3 * b6) >> 13;
  x2 = ((int32_t)b1 * ((b6 * b6) >> 12)) >> 16;
  x3 = ((x1 + x2) + 2) >> 2;
  b4 = (ac4 * (uint32_t)(x3 + 32768)) >> 15;
  b7 = ((uint32_t)up - b3) * (50000 >> oss);

  int32_t p;
  if (b7 < 0x80000000UL) {
    p = (b7 * 2) / b4;
  } else {
    p = (b7 / b4) * 2;
  }
  x1 = (p >> 8) * (p >> 8);
  x1 = (x1 * 3038) >> 16;
  x2 = (-7357 * p) >> 16;
  p = p + ((x1 + x2 + 3791) >> 4);

  return p;   // unidad: Pascales
}

void setup() {
  Serial.begin(115200);
  Wire.begin();

  // Verificar que el chip responde
  Wire.beginTransmission(DIR_BMP180);
  if (Wire.endTransmission() != 0) {
    Serial.println(F("# BMP180 no detectado. Revisar SDA=A4, SCL=A5, alimentacion y GND."));
    while (true) { delay(1000); }
  }

  // Verificar chip ID (0x55)
  uint16_t id = leer16(0xD0);
  if (id != 0x55) {
    Serial.print(F("# BMP180: chip ID inesperado 0x"));
    Serial.println(id, HEX);
    while (true) { delay(1000); }
  }

  // Leer coeficientes de calibracion
  ac1 = (int16_t)leer16(0xAA);
  ac2 = (int16_t)leer16(0xAC);
  ac3 = (int16_t)leer16(0xAE);
  ac4 = leer16(0xB0);
  ac5 = leer16(0xB2);
  ac6 = leer16(0xB4);
  b1  = (int16_t)leer16(0xB6);
  b2  = (int16_t)leer16(0xB8);
  mb  = (int16_t)leer16(0xBA);
  mc  = (int16_t)leer16(0xBC);
  md  = (int16_t)leer16(0xBE);
  int32_t en1 = (int16_t)leer16(0xBF);  // (no usado, solo referencia)

  Serial.println(F("tiempo_ms,presion_hPa,bmp_temp_C"));
}

void loop() {
  static uint32_t t_ultima = 0;
  if (millis() - t_ultima < 1000) return;
  t_ultima = millis();

  int32_t temperatura01 = leerTemperatura();  // 0.1 °C
  int32_t presion_pa    = leerPresion();      // Pa

  Serial.print(millis());
  Serial.print(',');
  Serial.print(presion_pa / 100.0, 2);        // Pa -> hPa
  Serial.print(',');
  Serial.println(temperatura01 / 10.0, 1);    // 0.1 °C -> °C
}
