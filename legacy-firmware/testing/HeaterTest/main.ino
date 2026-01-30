// ELECTRON PIN MAPPINGS
int pinBatteryLED = A0;
int pinBatteryAin = A1;
int pinDCinDetect = A2;
int pinQRTrigger = A3;
int pinSensorLED = A4;
int pinSolenoid = A5;
int pinDeviceLEDRed = B0;
int pinDeviceLEDGreen = B1;
int pinDeviceLEDBlue = B2;
int pinBluetoothMode = B3;
int pinBluetoothRX = C2;
int pinBluetoothTX = C3;
int pinAssaySDA = C4;
int pinAssaySCL = C5;
int pinControlSDA = D0;
int pinControlSCL = D1;
int pinLimitSwitch = D2;
int pinStepperSleep = D3;
int pinStepperDir = D4;
int pinStepperStep = D5;
int pinQRDecoderRX = RX;
int pinQRDecoderTX = TX;

int pinHeater = RX;
int pinHeaterLED = TX;

void setup() {
    pinMode(pinHeater, OUTPUT);
    pinMode(pinHeaterLED, OUTPUT);

    digitalWrite(pinHeater, LOW);
    digitalWrite(pinHeaterLED, LOW);
}

void loop() {
    digitalWrite(pinHeater, HIGH);
    digitalWrite(pinHeaterLED, HIGH);

    delay(1000);

    digitalWrite(pinHeater, LOW);
    digitalWrite(pinHeaterLED, LOW);

    delay(10000);
}
