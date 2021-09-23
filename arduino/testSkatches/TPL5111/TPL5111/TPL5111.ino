int donePin = 7;
int clkInterruptPin = 3;

void setup() {
  // put your setup code here, to run once:
  pinMode(donePin, OUTPUT);
  digitalWrite(donePin, LOW);
  Serial.begin(9600);
  
  pinMode(clkInterruptPin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(clkInterruptPin), onTimerInterrupt, RISING);
}

void loop() {
  // put your main code here, to run repeatedly:
  Serial.print("Nothing happened: InterruptPin = ");
  Serial.println(digitalRead(clkInterruptPin));
  delay(1000);
}

void onTimerInterrupt() {
  Serial.println("Timer interrupt");
  Serial.println("Do stuff");
  digitalWrite(donePin, HIGH);
  digitalWrite(donePin, LOW);
}
