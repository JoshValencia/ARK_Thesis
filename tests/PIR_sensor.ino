#define PIR 6

void setup() {
  Serial.begin(9600);
  pinMode(PIR, INPUT);
}
void loop() {
  Serial.println(digitalRead(PIR) ? "Motion!" : "Clear");
  delay(300);
}