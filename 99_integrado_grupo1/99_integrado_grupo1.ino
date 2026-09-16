/*
 * Solemne 1 - Grupo 1 - Estacion ambiental inteligente (CODIGO INTEGRADO)
 *
 * Integra los 5 sensores del Grupo 1 sobre un Arduino Uno:
 *   - DHT11        -> D2   (digital, protocolo de 1 cable)
 *   - Sonido       -> D3   (digital, umbral por comparador LM393)
 *   - MQ135        -> A0   (analogico, ADC crudo - NO ppm)
 *   - BMP180       -> I2C  (A4=SDA, A5=SCL, direccion 0x77)
 *   - BH1750       -> I2C  (A4=SDA, A5=SCL, direccion 0x23 o 0x5C)
 *
 * Genera UNA fila CSV por segundo con la forma:
 *   tiempo_ms,temperatura_C,humedad_pct,presion_hPa,lux,mq135_raw,sonido_estado,sonido_flancos_1s
 *
 * El DHT11 se lee cada 2 s (limite del datasheet) y se mantiene el ultimo
 * valor valido entre lecturas. El resto de sensores se leen cada 1 s.
 * sonido_flancos_1s es una columna DERIVADA: cuenta los cambios de estado
 * del sensor de sonido en el ultimo segundo (asi no se pierde un aplauso corto).
 */

#include <Wire.h>
#include <DHT.h>

// ---------------- Pines ----------------
#define PIN_DHT      2
#define PIN_SONIDO   3
#define PIN_MQ135    A0
#define TIPO_DHT     DHT11

// ---------------- BMP180 (Wire directo) ----------------
#define DIR_BMP180   0x77

int32_t ac1, ac2, ac3, b1, b2, mb, mc, md;
uint32_t ac4, ac5, ac6;
int32_t b5;

// ---------------- BH1750 (Wire directo) ----------------
uint8_t dirBH = 0x23;

// ---------------- DHT ----------------
DHT dht(PIN_DHT, TIPO_DHT);

// ---------------- Estado de los sensores I2C ----------------
bool bmp_ok = false;
bool bh_ok  = false;

// ================= BMP180: utilidades =================
uint16_t bmpLeer16(uint8_t registro) {
  Wire.beginTransmission(DIR_BMP180);
  Wire.write(registro);
  Wire.endTransmission();
  Wire.requestFrom((uint8_t)DIR_BMP180, (uint8_t)2);
  return ((uint16_t)Wire.read() << 8) | Wire.read();
}

bool bmpIniciar() {
  Wire.beginTransmission(DIR_BMP180);
  if (Wire.endTransmission() != 0) return false;
  if (bmpLeer16(0xD0) != 0x55) return false;

  ac1 = (int16_t)bmpLeer16(0xAA);
  ac2 = (int16_t)bmpLeer16(0xAC);
  ac3 = (int16_t)bmpLeer16(0xAE);
  ac4 = bmpLeer16(0xB0);
  ac5 = bmpLeer16(0xB2);
  ac6 = bmpLeer16(0xB4);
  b1  = (int16_t)bmpLeer16(0xB6);
  b2  = (int16_t)bmpLeer16(0xB8);
  mb  = (int16_t)bmpLeer16(0xBA);
  mc  = (int16_t)bmpLeer16(0xBC);
  md  = (int16_t)bmpLeer16(0xBE);
  return true;
}

int32_t bmpLeerTemperatura() {
  Wire.beginTransmission(DIR_BMP180);
  Wire.write(0xF4);
  Wire.write(0x2E);
  Wire.endTransmission();
  delay(5);

  int32_t ut = (int32_t)bmpLeer16(0xF6);
  int32_t x1 = ((ut - (int32_t)ac6) * (int32_t)ac5) >> 15;
  int32_t x2 = ((int32_t)mc * 2048) / (x1 + md);
  b5 = x1 + x2;
  return (b5 + 8) >> 4;   // 0.1 °C
}

int32_t bmpLeerPresion() {
  const uint8_t oss = 3;
  Wire.beginTransmission(DIR_BMP180);
  Wire.write(0xF4);
  Wire.write(0x34 | (oss << 6));
  Wire.endTransmission();
  delay(26);

  Wire.beginTransmission(DIR_BMP180);
  Wire.write(0xF6);
  Wire.endTransmission();
  Wire.requestFrom((uint8_t)DIR_BMP180, (uint8_t)3);
  int32_t up = ((int32_t)Wire.read() << 16) | ((int32_t)Wire.read() << 8) | Wire.read();
  up = up >> (8 - oss);

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
  if (b7 < 0x80000000UL) p = (b7 * 2) / b4;
  else                   p = (b7 / b4) * 2;
  x1 = (p >> 8) * (p >> 8);
  x1 = (x1 * 3038) >> 16;
  x2 = (-7357 * p) >> 16;
  p = p + ((x1 + x2 + 3791) >> 4);

  return p;   // Pascales
}

// ================= BH1750: utilidades =================
bool bhEscribir(uint8_t comando) {
  Wire.beginTransmission(dirBH);
  Wire.write(comando);
  return Wire.endTransmission() == 0;
}

bool bhIniciar() {
  uint8_t candidatas[2] = {0x23, 0x5C};
  for (uint8_t i = 0; i < 2; i++) {
    dirBH = candidatas[i];
    if (bhEscribir(0x01)) {      // power on
      bhEscribir(0x10);          // modo continuo alta resolucion
      delay(180);
      return true;
    }
  }
  return false;
}

// ================= Programa principal =================
void setup() {
  Serial.begin(115200);
  pinMode(PIN_SONIDO, INPUT);
  pinMode(PIN_MQ135, INPUT);

  Wire.begin();
  dht.begin();

  bmp_ok = bmpIniciar();
  bh_ok  = bhIniciar();
  if (!bmp_ok) Serial.println(F("# ADVERTENCIA: BMP180 no detectado (0x77)"));
  if (!bh_ok)  Serial.println(F("# ADVERTENCIA: BH1750 no detectado (0x23/0x5C)"));

  // Encabezado del CSV (la primera fila del archivo)
  Serial.println(F("tiempo_ms,temperatura_C,humedad_pct,presion_hPa,lux,mq135_raw,sonido_estado,sonido_flancos_1s"));

  delay(1200);   // dar tiempo al primer muestreo del DHT11
}

void loop() {
  static uint32_t t_muestra      = 0;    // control del periodo de 1 s
  static uint32_t t_dht          = 0;    // el DHT11 se lee cada 2 s
  static float    temperatura    = NAN;
  static float    humedad        = NAN;
  static uint32_t t_sonido       = 0;    // muestreo rapido del sonido (2 ms)
  static uint8_t  sonido_prev    = 0;
  static uint32_t sonido_flancos = 0;    // cambios de estado en el ultimo segundo

  uint32_t ahora = millis();

  // --- Muestreo rapido del sensor de sonido (cada 2 ms) ---
  if (ahora - t_sonido >= 2) {
    t_sonido = ahora;
    uint8_t estado_actual = digitalRead(PIN_SONIDO);
    if (estado_actual != sonido_prev) {
      sonido_flancos++;
      sonido_prev = estado_actual;
    }
  }

  // --- DHT11 cada 2 segundos (manteniendo ultimo valor valido) ---
  if (ahora - t_dht >= 2000) {
    t_dht = ahora;
    float t = dht.readTemperature();
    float h = dht.readHumidity();
    if (!isnan(t) && !isnan(h)) {
      temperatura = t;
      humedad = h;
    }
  }

  // --- Una fila CSV por segundo ---
  if (ahora - t_muestra < 1000) return;
  t_muestra = ahora;

  int   mq135_raw     = analogRead(PIN_MQ135);
  uint8_t sonido_estado = sonido_prev;
  uint32_t flancos_segundo = sonido_flancos;
  sonido_flancos = 0;   // se reinicia el contador para el siguiente segundo

  float presion_hPa = NAN;
  if (bmp_ok) {
    bmpLeerTemperatura();                  // actualiza b5 (necesario para compensar)
    presion_hPa = bmpLeerPresion() / 100.0;
  }

  float lux = NAN;
  if (bh_ok) {
    Wire.requestFrom(dirBH, (uint8_t)2);
    if (Wire.available() >= 2) {
      uint16_t crudo = ((uint16_t)Wire.read() << 8) | Wire.read();
      lux = crudo / 1.2;
    }
  }

  Serial.print(millis());
  Serial.print(',');
  Serial.print(temperatura, 1);
  Serial.print(',');
  Serial.print(humedad, 1);
  Serial.print(',');
  Serial.print(presion_hPa, 2);
  Serial.print(',');
  Serial.print(lux, 1);
  Serial.print(',');
  Serial.print(mq135_raw);
  Serial.print(',');
  Serial.print(sonido_estado);
  Serial.print(',');
  Serial.println(flancos_segundo);
}
