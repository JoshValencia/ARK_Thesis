#define RELAY 7

// You should hear a click from the relay each time.

void setup() {
  pinMode(RELAY, OUTPUT);
}
void loop() {
  digitalWrite(RELAY, HIGH); delay(2000); // ON
  digitalWrite(RELAY, LOW);  delay(2000); // OFF
}