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

int pinHeatRed = RX;
int pinHeatBlack = TX;

void setup() {
    pinMode(pinDeviceLEDRed, OUTPUT);
    pinMode(pinDeviceLEDGreen, OUTPUT);
    pinMode(pinDeviceLEDBlue, OUTPUT);

    analogWrite(pinDeviceLEDRed, 0);
    analogWrite(pinDeviceLEDGreen, 0);
    analogWrite(pinDeviceLEDBlue, 0);

    pinMode(pinHeatRed, OUTPUT);
    pinMode(pinHeatBlack, OUTPUT);

    analogWrite(pinHeatRed, 0);
    analogWrite(pinHeatBlack, 0);
}

void loop() {
    pinMode(pinHeatRed, OUTPUT);
    pinMode(pinHeatBlack, OUTPUT);

    analogWrite(pinDeviceLEDRed, 255);
    analogWrite(pinHeatRed, 100);

    delay(20000);

    pinMode(pinHeatRed, OUTPUT);
    pinMode(pinHeatBlack, OUTPUT);

    analogWrite(pinHeatRed, 0);
    analogWrite(pinDeviceLEDRed, 0);

    delay(20000);
}
