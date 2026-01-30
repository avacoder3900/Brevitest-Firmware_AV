#include "main.h"
#include "Serial4/Serial4.h"

/////////////////////////////////////////////////////////////
//                                                         //
//                       BLUETOOTH                         //
//                                                         //
/////////////////////////////////////////////////////////////

int set_wake(String cmd) {
    char s = cmd.charAt(0);
    if (s == 'H' || s == 'h') {
        digitalWrite(pinBluetoothWake, HIGH);
    }
    if (s == 'L' || s == 'l') {
        digitalWrite(pinBluetoothWake, LOW);
    }
    return 1;
}

int set_cmd(String cmd) {
    char s = cmd.charAt(0);
    if (s == 'H' || s == 'h') {
        digitalWrite(pinBluetoothCmd, HIGH);
    }
    if (s == 'L' || s == 'l') {
        digitalWrite(pinBluetoothCmd, LOW);
    }
    return 1;
}

char command_result[30];
int send_bluetooth_command(String cmd) {
    int indx = 0;
    unsigned long timeout;

    Serial4.println(cmd);
    Serial.println(cmd);

    timeout = millis() + 5000;
    while (!Serial4.available()) {
        if (millis() > timeout) {
            Serial.println("Error: Timeout");
            return -1;
        }
        Particle.process();
    }
    while (Serial4.available()) {
        command_result[indx++] = (char) Serial4.read();
        if (indx >= 30) {
            Serial.println("Error: Overflow");
            return -2;
        }
        delay(1);
    }
    command_result[indx] = '\0';
    Serial.print(command_result);
    return 1;
}


/////////////////////////////////////////////////////////////
//                                                         //
//                          SETUP                          //
//                                                         //
/////////////////////////////////////////////////////////////

void setup() {
    int c;

    Particle.function("set_wake", set_wake);
    Particle.function("set_cmd", set_cmd);

    pinMode(pinBluetoothWake, OUTPUT);
    pinMode(pinBluetoothCmd, OUTPUT);

    digitalWrite(pinBluetoothCmd, LOW);
    digitalWrite(pinBluetoothWake, HIGH);

    Serial.begin(115200);     // standard serial port
    Serial4.begin(9600);

    Serial.println("Press any key to begin");
    while (!Serial.available()) {
        Particle.process();
    }
    while (Serial.available()) {
        c = Serial.read();
    }
    while (Serial4.available()) {
        c = Serial4.read();
    }
    /*c = send_bluetooth_command("+");
    c = (c > 0) ? send_bluetooth_command("SF,1") : c;*/
    c = send_bluetooth_command("SF,1");
    c = (c > 0) ? send_bluetooth_command("SB,1") : c;
    c = (c > 0) ? send_bluetooth_command("SR,A0006000") : c;
    c = (c > 0) ? send_bluetooth_command("SS,00000001") : c;
    c = (c > 0) ? send_bluetooth_command("PZ") : c;
    c = (c > 0) ? send_bluetooth_command("PS,5998A6F223774919BED5B1AF98C28985") : c;
    c = (c > 0) ? send_bluetooth_command("PC,75395B6367CB4684BC2B52985A28B249,08,10") : c;
    c = (c > 0) ? send_bluetooth_command("PC,60A9B533F5314432880CA6781A78EA10,12,10") : c;
    c = (c > 0) ? send_bluetooth_command("R,1") : c;
}

/////////////////////////////////////////////////////////////
//                                                         //
//                           LOOP                          //
//                                                         //
/////////////////////////////////////////////////////////////

void loop(){
  while (Serial.available()) {
    Serial4.write(Serial.read());
  }

  while (Serial4.available()) {
    Serial.write(Serial4.read());
  }
}
