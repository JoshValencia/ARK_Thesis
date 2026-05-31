#define IN1 8
#define IN2 9
#define EN  10

// Motor should spin 3 seconds, stop 2 seconds, repeat.

void setup() {
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(EN, OUTPUT);
}
void loop() {
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  analogWrite(EN, 200);
  delay(3000);
  digitalWrite(IN1, LOW);
  analogWrite(EN, 0);
  delay(2000);
}