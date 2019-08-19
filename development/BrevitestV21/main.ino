SYSTEM_THREAD(ENABLED);

int step = A0;
int dir = A1;

void setup() {
    pinMode(step, OUTPUT);
    pinMode(dir, OUTPUT);

    digitalWrite(step, LOW);
    digitalWrite(dir, LOW);

    delay(10);
}

void loop() {
    int i;

    digitalWrite(dir, HIGH);
    delay(5);
    SINGLE_THREADED_BLOCK() {
        for (i = 0; i < 200; i++) {
            digitalWrite(step, HIGH);
            delayMicroseconds(1000);
            digitalWrite(step, LOW);
            delayMicroseconds(1000);
        }
    }

    delay(10);

    digitalWrite(dir, LOW);
    delay(5);
    SINGLE_THREADED_BLOCK() {
        for (i = 0; i < 200; i++) {
            digitalWrite(step, HIGH);
            delayMicroseconds(1000);
            digitalWrite(step, LOW);
            delayMicroseconds(1000);
        }
    }

    delay(10);
}
