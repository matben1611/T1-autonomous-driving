#define FRONT_LED 13
#define BACK_LED 15

void setup() {
  pinMode(FRONT_LED, OUTPUT);
  pinMode(BACK_LED, OUTPUT);
}

void loop() {
  digitalWrite(FRONT_LED, HIGH);
  digitalWrite(BACK_LED, HIGH);
  delay(1000);
  digitalWrite(FRONT_LED, LOW);
  digitalWrite(BACK_LED, LOW);
  delay(1000);
}
