/*
 * Solemne 1 - Grupo 1 - BH1750 / GY-302 (iluminancia en lux, bus I2C)
 * Conexion: VCC -> 5V (modulo GY-302), GND -> GND, SDA -> A4, SCL -> A5
 *
 * NO requiere libreria externa: lectura directa por I2C con Wire.
 *
 * Direcciones posibles: 0x23 (ADDR al aire, tipico) o 0x5C (ADDR a VCC).
 * El programa detecta automaticamente cual responde.
 *
 * Comandos usados:
 *   0x01 : encender (power on)
 *   0x10 : modo continuo, resolucion alta (1 lx, 120 ms por medicion)
 * La hoja de datos indica: lux = valor_crudo / 1.2
 *
 * Salida:
 *   - lux : variable fisica (lectura cruda / 1.2)
 */

#include <Wire.h>

uint8_t dirBH = 0x23;

bool escribirBH(uint8_t comando) {
  Wire.beginTransmission(dirBH);
  Wire.write(comando);
  return Wire.endTransmission() == 0;   // 0 = el dispositivo respondio (ACK)
}

void setup() {
  Serial.begin(115200);
  Wire.begin();

  // Detectar direccion: probar 0x23 y luego 0x5C
  bool encontrado = false;
  uint8_t candidatas[2] = {0x23, 0x5C};
  for (uint8_t i = 0; i < 2; i++) {
    dirBH = candidatas[i];
    if (escribirBH(0x01)) {   // power on
      encontrado = true;
      break;
    }
  }

  if (!encontrado) {
    Serial.println(F("# BH1750 no detectado en 0x23 ni 0x5C. Revisar SDA=A4, SCL=A5, VCC y GND."));
    while (true) { delay(1000); }
  }

  escribirBH(0x10);   // modo continuo, alta resolucion
  delay(180);         // primera medicion lista (max 180 ms)

  Serial.print(F("# BH1750 detectado en 0x"));
  Serial.println(dirBH, HEX);
  Serial.println(F("tiempo_ms,lux"));
}

void loop() {
  static uint32_t t_ultima = 0;
  if (millis() - t_ultima < 1000) return;
  t_ultima = millis();

  Wire.requestFrom(dirBH, (uint8_t)2);
  if (Wire.available() < 2) {
    Serial.println(F("# BH1750: sin respuesta al leer lux"));
    return;
  }
  uint16_t crudo = ((uint16_t)Wire.read() << 8) | Wire.read();
  float lux = crudo / 1.2;   // conversion indicada por el fabricante (ROHM)

  Serial.print(millis());
  Serial.print(',');
  Serial.println(lux, 1);
}
