const int pwmPin = 4;
const int pwmFrequency = 20000; // 19 kHz
const int pwmResolution = 11;   // 10-bit resolution (1024 levels)
const int pwmValue1 = 0;
const int pwmValue2 = 0;
const int blinkFrequency = 200;

void setup() {
  pinMode(27, OUTPUT); // Set IO27 as output
  digitalWrite(27, HIGH); // Enable output on IO27 (set it to HIGH)
  
  ledcSetup(0, pwmFrequency, pwmResolution); // Configure PWM
  ledcAttachPin(pwmPin, 0); // Attach PWM to IO4 (Channel 0)
}

void loop() {
  // Alternately set the PWM duty cycle to pwmValue1 and pwmValue2
  ledcWrite(0, pwmValue1);
  delayMicroseconds(1000000 / (2 * blinkFrequency)); // Half-period delay

  ledcWrite(0, pwmValue2);
  delayMicroseconds(1000000 / (2 * blinkFrequency)); // Half-period delay
}