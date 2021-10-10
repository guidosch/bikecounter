// pins
const int batteryVoltagePin = A0;

void setup()
{

  // The MKR WAN 1310 3.3V reference voltage for battery measurements
  analogReference(AR_DEFAULT);

  // open serial connection and wait
  Serial.begin(9600);
  while (!Serial)
    ;
}

void loop()
{
  Serial.print("Voltage = ");
  Serial.print(getBatteryVoltage());
  Serial.println(" V");
  delay(300000ul);
}

float getBatteryVoltage()
{
  // read the input on analog pin 0 (A1) and calculate the voltage
  return analogRead(batteryVoltagePin) * 3.3f / 1023.0f / 1.2f * (1.2f + 0.33f);
}

// get battery level [%]
float getBatteryLevel()
{
  float bV = getBatteryVoltage();
  // charch calculation
  // devided into two ranges >=3.8V and <3.8
  // curve parameters from pseudo invers matrix polynom
  if (bV >= 3.8f)
  {
    return -178.57f * bV * bV + 1569.6f * bV - 3342.0f;
  }
  else
  {
    return 1183.7f * bV * bV * bV * bV - 15843.0f * bV * bV * bV + 79461.0f * bV * bV - 177004.0f * bV + 147744.0f;
  }
}

// pars the battery level to a 3 bit indicator
// 0 = 0%
// 7 = 100%
byte parsBatLevel(float batteryLevel)
{
  //
  if (batteryLevel < 0.0f)
  {
    batteryLevel = 0.0f;
  }
  float fract = 100.0f / (2 * 2 * 2 - 1);
  float indicator = batteryLevel / fract;

  return (byte)(round(indicator));
}
