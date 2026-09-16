/*
 * Solemne 1 - Grupo 1 - Escaner I2C
 * Utilidad de diagnostico: recorre todas las direcciones I2C y muestra cuales responden.
 *
 * Resultado esperado en el montaje del Grupo 1:
 *   0x23 o 0x5C -> BH1750 (iluminancia)
 *   0x77        -> BMP180 (presion)
 *
 * Conexion para probar el bus: SDA -> A4, SCL -> A5, VCC -> 5V, GND -> GND (de cada modulo).
 */

#include <Wire.h>

void setup() {
  Serial.begin(115200);
  Wire.begin();
  Serial.println(F("Escaner I2C iniciado..."));
}

void loop() {
  uint8_t encontrados = 0;

  for (uint8_t direccion = 1; direccion < 127; direccion++) {
    Wire.beginTransmission(direccion);
    if (Wire.endTransmission() == 0) {
      Serial.print(F("Dispositivo encontrado en 0x"));
      if (direccion < 16) Serial.print('0');
      Serial.println(direccion, HEX);
      encontrados++;
    }
  }

  if (encontrados == 0) {
    Serial.println(F("No se encontro ningun dispositivo I2C. Revisar cableado SDA/SCL y alimentacion."));
  }

  Serial.println(F("--- fin del escaneo ---"));
  delay(3000);
}
