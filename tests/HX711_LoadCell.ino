#include "HX711.h"
HX711 scale;

// Open Serial Monitor. Numbers should change when you place something on the load cell.

void setup() {
  Serial.begin(9600);
  scale.begin(3, 2); // DT=3, SCK=2
  scale.tare();
  Serial.println("Scale ready. Place weight.");
}
void loop() {
  Serial.println(scale.get_units(5));
  delay(500);
}