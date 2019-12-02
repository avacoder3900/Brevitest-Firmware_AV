// /*
//  *    Example source code for an Arduino to show
//  *    how to communicate with an Allegro ALS31300
//  *
//  *    Written by K. Robert Bate, Allegro MicroSystems, LLC.
//  *
//  *    ALS31300Demo is distributed in the hope that it will be useful,
//  *    but WITHOUT ANY WARRANTY; without even the implied warranty of
//  *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//  */

// Return values of endTransmission in the Wire library
#define kNOERROR 0
#define kDATATOOLONGERROR 1
#define kRECEIVEDNACKONADDRESSERROR 2
#define kRECEIVEDNACKONDATAERROR 3
#define kOTHERERROR 4

// led blinking support
bool ledState = false;
int ledPin = D7;
unsigned long nextTime;

//int deviceAddress = 0x60; // Address of the ALS31300
int ninetysix = 0x60;
// int ninetyseven=0x61;
// int ninetyeight=0x62;
// int ninetynine=0x63;
// int hundred=0x64;
// int hundredone=0x65;
// int hundredtwo=0x66;
// int hundredthree=0x67;
// int hundredfour=0x68;
// int hundredfive=0x69;
// int hundredsix=0x6A;
// int hundredseven=0x6B;
// SCL Pin = 19
// SDA Pin = 18

//
// setup
//
// Initializes the Wire library for I2C communications,
// Serial for displaying the results and error messages,
// the hardware and variables to blink the LED,
// and sets the ALS31300 into customer access mode.
//
void setup()
{
    // Initialize the I2C communication library
    Wire.setClock(CLOCK_SPEED_100KHZ);    // 1 MHz
    Wire.begin();

    // Initialize the serial port
    Serial.begin(115200);
    // If using a Arduino with USB built in, uncomment the next line,
    // this allows the errors in Setup to be seen
    // while (!Serial);

    // Setup hardware and variables for code which blinks the LED
    nextTime = millis();
    pinMode(ledPin, OUTPUT);
    digitalWrite(ledPin, LOW);
    
    // Enter customer access mode on the ALS31300
    // uint16_t error = write(deviceAddress, 0x24, 0x2C413534);
    
    // if (error != kNOERROR)
    // {
    //     Serial.print("Error while trying to enter customer access mode. error = ");
    //     Serial.println(error);
    // }
}

// loop
//
// Every half second, read the ADCs of the ALS31300 and display
// the values and toggle the state of the LED.
//
void loop()
{
    // only perform the reading of the ALS31300 and the
    // toggling the state of the LED every half second
    delay(500);
    if (nextTime < millis())
    {
        nextTime = millis() + 500L;    

        // Uncomment which reading style is desired
        readALS31300ADC_1(ninetysix);
        // readALS31300ADC_2(ninetyseven);
        // readALS31300ADC_3(ninetyeight);
        // readALS31300ADC_4(ninetynine);
        // readALS31300ADC_5(hundred);
        // readALS31300ADC_6(hundredone);
        // readALS31300ADC_7(hundredtwo);
        // readALS31300ADC_8(hundredthree);
        // readALS31300ADC_9(hundredfour);
        // readALS31300ADC_10(hundredfive);
        // readALS31300ADC_11(hundredsix);
        // readALS31300ADC_12(hundredseven);
        //readALS31300ADC_FastLoop_1(ninetysix);
        //readALS31300ADC_FastLoop_2(ninetyseven);
        //readALS31300ADC_FastLoop_3(ninetyeight);
        //readALS31300ADC_FastLoop_4(ninetynine);
        //readALS31300ADC_FastLoop_5(hundred);
        //readALS31300ADC_FastLoop_6(hundredone);
        //readALS31300ADC_FastLoop_7(hundredtwo);
        //readALS31300ADC_FastLoop_8(hundredthree);
        //readALS31300ADC_FastLoop_9(hundredfour);
        //readALS31300ADC_FastLoop_10(hundredfive);
        //readALS31300ADC_FastLoop_11(hundredsix);
        //readALS31300ADC_FastLoop_12(hundredseven);
        //readALS31300ADC_FullLoop(deviceAddress);

        // Blink the LED
        ledState = !ledState;
        digitalWrite(ledPin, ledState);
    }
}

//
// readALS31300ADC
//
// Read the X, Y, Z values from Register 0x28 and 0x29
// eight times. No loop mode is used.
//
void readALS31300ADC_1(int busAddress_1)
{    
   

    for (int count1 = 0; count1 < 2; ++count1)
    {
        // Write the address that is going to be read from the ALS31300
        Wire.beginTransmission(busAddress_1);
        Wire.write(0x28);
        uint16_t error = Wire.endTransmission(false);

        // The ALS31300 accepted the address
        if (error == kNOERROR)
        {
            // Start the read and request 8 bytes
            // which are the contents of register 0x28 and 0x29
            Wire.requestFrom(busAddress_1, 8);
            
            // Read the first 4 bytes which are the contents of register 0x28
            uint32_t value0x28 = Wire.read() << 24;
            value0x28 += Wire.read() << 16;
            value0x28 += Wire.read() << 8;
            value0x28 += Wire.read();

            // Read the next 4 bytes which are the contents of register 0x29
            uint32_t value0x29 = Wire.read() << 24;
            value0x29 += Wire.read() << 16;
            value0x29 += Wire.read() << 8;
            value0x29 += Wire.read();

            // Take the most significant byte of each axis from register 0x28 and combine it with the least
            // significant 4 bits of each axis from register 0x29, then sign extend the 12th bit.
            int x1 = SignExtendBitfield(((value0x28 >> 20) & 0x0FF0) | ((value0x29 >> 16) & 0x0F), 12);
            int y1 = SignExtendBitfield(((value0x28 >> 12) & 0x0FF0) | ((value0x29 >> 12) & 0x0F), 12);
            int z1 = SignExtendBitfield(((value0x28 >> 4) & 0x0FF0) | ((value0x29 >> 8) & 0x0F), 12);

            // Display the values of x, y and z
           // Serial.print("Count2, X2, Y2, Z2 = ");
           // Serial.print(count2);
            //Serial.print(", ");
            //Serial.print(x2);
            //Serial.print(", ");
            //Serial.print(y2);
           // Serial.print(", ");
            //Serial.println(z2);

            // Look at the datasheet for the sensitivity of the part used.
            // In this case, full scale range is 500 gauss, other sensitivities
            // are 1000 gauss and 2000 gauss
            float mx1 = (float)x1 / 1.0;
            float my1 = (float)y1 / 1.0;
            float mz1 = (float)z1 / 0.25;

            Serial.print("MX1, MY1, MZ1 = ");
            Serial.print(mx1);
            Serial.print(", ");
            Serial.print(my1);
            Serial.print(", ");
            Serial.print(mz1);
            Serial.println(" Gauss");

            
        }
        else
        {
            Serial.print("4 - Unable to read the ALS31300. error = ");
            Serial.println(error);
            break;
        }
    }
}

void readALS31300ADC_2(int busAddress_2)
{    
   

    for (int count2 = 0; count2 < 2; ++count2)
    {
        // Write the address that is going to be read from the ALS31300
        Wire.beginTransmission(busAddress_2);
        Wire.write(0x28);
        uint16_t error = Wire.endTransmission(false);

        // The ALS31300 accepted the address
        if (error == kNOERROR)
        {
            // Start the read and request 8 bytes
            // which are the contents of register 0x28 and 0x29
            Wire.requestFrom(busAddress_2, 8);
            
            // Read the first 4 bytes which are the contents of register 0x28
            uint32_t value0x28 = Wire.read() << 24;
            value0x28 += Wire.read() << 16;
            value0x28 += Wire.read() << 8;
            value0x28 += Wire.read();

            // Read the next 4 bytes which are the contents of register 0x29
            uint32_t value0x29 = Wire.read() << 24;
            value0x29 += Wire.read() << 16;
            value0x29 += Wire.read() << 8;
            value0x29 += Wire.read();

            // Take the most significant byte of each axis from register 0x28 and combine it with the least
            // significant 4 bits of each axis from register 0x29, then sign extend the 12th bit.
            int x2 = SignExtendBitfield(((value0x28 >> 20) & 0x0FF0) | ((value0x29 >> 16) & 0x0F), 12);
            int y2 = SignExtendBitfield(((value0x28 >> 12) & 0x0FF0) | ((value0x29 >> 12) & 0x0F), 12);
            int z2 = SignExtendBitfield(((value0x28 >> 4) & 0x0FF0) | ((value0x29 >> 8) & 0x0F), 12);

            // Display the values of x, y and z
           // Serial.print("Count2, X2, Y2, Z2 = ");
           // Serial.print(count2);
            //Serial.print(", ");
            //Serial.print(x2);
            //Serial.print(", ");
            //Serial.print(y2);
           // Serial.print(", ");
            //Serial.println(z2);

            // Look at the datasheet for the sensitivity of the part used.
            // In this case, full scale range is 500 gauss, other sensitivities
            // are 1000 gauss and 2000 gauss
            float mx2 = (float)x2 / 1.0;
            float my2 = (float)y2 / 1.0;
            float mz2 = (float)z2 / 0.25;

            Serial.print("MX2, MY2, MZ2 = ");
            Serial.print(mx2);
            Serial.print(", ");
            Serial.print(my2);
            Serial.print(", ");
            Serial.print(mz2);
            Serial.println(" Gauss");

            
        }
        else
        {
            Serial.print("4 - Unable to read the ALS31300. error = ");
            Serial.println(error);
            break;
        }
    }
}

void readALS31300ADC_3(int busAddress_3)
{    
   

    for (int count3 = 0; count3 < 2; ++count3)
    {
        // Write the address that is going to be read from the ALS31300
        Wire.beginTransmission(busAddress_3);
        Wire.write(0x28);
        uint16_t error = Wire.endTransmission(false);

        // The ALS31300 accepted the address
        if (error == kNOERROR)
        {
            // Start the read and request 8 bytes
            // which are the contents of register 0x28 and 0x29
            Wire.requestFrom(busAddress_3, 8);
            
            // Read the first 4 bytes which are the contents of register 0x28
            uint32_t value0x28 = Wire.read() << 24;
            value0x28 += Wire.read() << 16;
            value0x28 += Wire.read() << 8;
            value0x28 += Wire.read();

            // Read the next 4 bytes which are the contents of register 0x29
            uint32_t value0x29 = Wire.read() << 24;
            value0x29 += Wire.read() << 16;
            value0x29 += Wire.read() << 8;
            value0x29 += Wire.read();

            // Take the most significant byte of each axis from register 0x28 and combine it with the least
            // significant 4 bits of each axis from register 0x29, then sign extend the 12th bit.
            int x3 = SignExtendBitfield(((value0x28 >> 20) & 0x0FF0) | ((value0x29 >> 16) & 0x0F), 12);
            int y3 = SignExtendBitfield(((value0x28 >> 12) & 0x0FF0) | ((value0x29 >> 12) & 0x0F), 12);
            int z3 = SignExtendBitfield(((value0x28 >> 4) & 0x0FF0) | ((value0x29 >> 8) & 0x0F), 12);

            // Display the values of x, y and z
           // Serial.print("Count2, X2, Y2, Z2 = ");
           // Serial.print(count2);
            //Serial.print(", ");
            //Serial.print(x2);
            //Serial.print(", ");
            //Serial.print(y2);
           // Serial.print(", ");
            //Serial.println(z2);

            // Look at the datasheet for the sensitivity of the part used.
            // In this case, full scale range is 500 gauss, other sensitivities
            // are 1000 gauss and 2000 gauss
            float mx3 = (float)x3 / 1.0;
            float my3 = (float)y3 / 1.0;
            float mz3 = (float)z3 / 0.25;

            Serial.print("MX3, MY3, MZ3 = ");
            Serial.print(mx3);
            Serial.print(", ");
            Serial.print(my3);
            Serial.print(", ");
            Serial.print(mz3);
            Serial.println(" Gauss");

            
        }
        else
        {
            Serial.print("4 - Unable to read the ALS31300. error = ");
            Serial.println(error);
            break;
        }
    }
}

void readALS31300ADC_4(int busAddress_4)
{    
   

    for (int count4 = 0; count4 < 2; ++count4)
    {
        // Write the address that is going to be read from the ALS31300
        Wire.beginTransmission(busAddress_4);
        Wire.write(0x28);
        uint16_t error = Wire.endTransmission(false);

        // The ALS31300 accepted the address
        if (error == kNOERROR)
        {
            // Start the read and request 8 bytes
            // which are the contents of register 0x28 and 0x29
            Wire.requestFrom(busAddress_4, 8);
            
            // Read the first 4 bytes which are the contents of register 0x28
            uint32_t value0x28 = Wire.read() << 24;
            value0x28 += Wire.read() << 16;
            value0x28 += Wire.read() << 8;
            value0x28 += Wire.read();

            // Read the next 4 bytes which are the contents of register 0x29
            uint32_t value0x29 = Wire.read() << 24;
            value0x29 += Wire.read() << 16;
            value0x29 += Wire.read() << 8;
            value0x29 += Wire.read();

            // Take the most significant byte of each axis from register 0x28 and combine it with the least
            // significant 4 bits of each axis from register 0x29, then sign extend the 12th bit.
            int x4 = SignExtendBitfield(((value0x28 >> 20) & 0x0FF0) | ((value0x29 >> 16) & 0x0F), 12);
            int y4 = SignExtendBitfield(((value0x28 >> 12) & 0x0FF0) | ((value0x29 >> 12) & 0x0F), 12);
            int z4 = SignExtendBitfield(((value0x28 >> 4) & 0x0FF0) | ((value0x29 >> 8) & 0x0F), 12);

            // Display the values of x, y and z
           // Serial.print("Count2, X2, Y2, Z2 = ");
           // Serial.print(count2);
            //Serial.print(", ");
            //Serial.print(x2);
            //Serial.print(", ");
            //Serial.print(y2);
           // Serial.print(", ");
            //Serial.println(z2);

            // Look at the datasheet for the sensitivity of the part used.
            // In this case, full scale range is 500 gauss, other sensitivities
            // are 1000 gauss and 2000 gauss
            float mx4 = (float)x4 / 1.0;
            float my4 = (float)y4 / 1.0;
            float mz4 = (float)z4 / 0.25;

            Serial.print("MX4, MY4, MZ4 = ");
            Serial.print(mx4);
            Serial.print(", ");
            Serial.print(my4);
            Serial.print(", ");
            Serial.print(mz4);
            Serial.println(" Gauss");

            
        }
        else
        {
            Serial.print("5 - Unable to read the ALS31300. error = ");
            Serial.println(error);
            break;
        }
    }
}

void readALS31300ADC_5(int busAddress_5)
{    
   

    for (int count5 = 0; count5 < 2; ++count5)
    {
        // Write the address that is going to be read from the ALS31300
        Wire.beginTransmission(busAddress_5);
        Wire.write(0x28);
        uint16_t error = Wire.endTransmission(false);

        // The ALS31300 accepted the address
        if (error == kNOERROR)
        {
            // Start the read and request 8 bytes
            // which are the contents of register 0x28 and 0x29
            Wire.requestFrom(busAddress_5, 8);
            
            // Read the first 4 bytes which are the contents of register 0x28
            uint32_t value0x28 = Wire.read() << 24;
            value0x28 += Wire.read() << 16;
            value0x28 += Wire.read() << 8;
            value0x28 += Wire.read();

            // Read the next 4 bytes which are the contents of register 0x29
            uint32_t value0x29 = Wire.read() << 24;
            value0x29 += Wire.read() << 16;
            value0x29 += Wire.read() << 8;
            value0x29 += Wire.read();

            // Take the most significant byte of each axis from register 0x28 and combine it with the least
            // significant 4 bits of each axis from register 0x29, then sign extend the 12th bit.
            int x5 = SignExtendBitfield(((value0x28 >> 20) & 0x0FF0) | ((value0x29 >> 16) & 0x0F), 12);
            int y5 = SignExtendBitfield(((value0x28 >> 12) & 0x0FF0) | ((value0x29 >> 12) & 0x0F), 12);
            int z5 = SignExtendBitfield(((value0x28 >> 4) & 0x0FF0) | ((value0x29 >> 8) & 0x0F), 12);

            // Display the values of x, y and z
           // Serial.print("Count2, X2, Y2, Z2 = ");
           // Serial.print(count2);
            //Serial.print(", ");
            //Serial.print(x2);
            //Serial.print(", ");
            //Serial.print(y2);
           // Serial.print(", ");
            //Serial.println(z2);

            // Look at the datasheet for the sensitivity of the part used.
            // In this case, full scale range is 500 gauss, other sensitivities
            // are 1000 gauss and 2000 gauss
            float mx5 = (float)x5 / 1.0;
            float my5 = (float)y5 / 1.0;
            float mz5 = (float)z5 / 0.25;

            Serial.print("MX5, MY5, MZ5 = ");
            Serial.print(mx5);
            Serial.print(", ");
            Serial.print(my5);
            Serial.print(", ");
            Serial.print(mz5);
            Serial.println(" Gauss");

            
        }
        else
        {
            Serial.print("6 - Unable to read the ALS31300. error = ");
            Serial.println(error);
            break;
        }
    }
}


void readALS31300ADC_6(int busAddress_6)
{    
   

    for (int count6 = 0; count6 < 2; ++count6)
    {
        // Write the address that is going to be read from the ALS31300
        Wire.beginTransmission(busAddress_6);
        Wire.write(0x28);
        uint16_t error = Wire.endTransmission(false);

        // The ALS31300 accepted the address
        if (error == kNOERROR)
        {
            // Start the read and request 8 bytes
            // which are the contents of register 0x28 and 0x29
            Wire.requestFrom(busAddress_6, 8);
            
            // Read the first 4 bytes which are the contents of register 0x28
            uint32_t value0x28 = Wire.read() << 24;
            value0x28 += Wire.read() << 16;
            value0x28 += Wire.read() << 8;
            value0x28 += Wire.read();

            // Read the next 4 bytes which are the contents of register 0x29
            uint32_t value0x29 = Wire.read() << 24;
            value0x29 += Wire.read() << 16;
            value0x29 += Wire.read() << 8;
            value0x29 += Wire.read();

            // Take the most significant byte of each axis from register 0x28 and combine it with the least
            // significant 4 bits of each axis from register 0x29, then sign extend the 12th bit.
            int x6 = SignExtendBitfield(((value0x28 >> 20) & 0x0FF0) | ((value0x29 >> 16) & 0x0F), 12);
            int y6 = SignExtendBitfield(((value0x28 >> 12) & 0x0FF0) | ((value0x29 >> 12) & 0x0F), 12);
            int z6 = SignExtendBitfield(((value0x28 >> 4) & 0x0FF0) | ((value0x29 >> 8) & 0x0F), 12);

            // Display the values of x, y and z
           // Serial.print("Count2, X2, Y2, Z2 = ");
           // Serial.print(count2);
            //Serial.print(", ");
            //Serial.print(x2);
            //Serial.print(", ");
            //Serial.print(y2);
           // Serial.print(", ");
            //Serial.println(z2);

            // Look at the datasheet for the sensitivity of the part used.
            // In this case, full scale range is 500 gauss, other sensitivities
            // are 1000 gauss and 2000 gauss
            float mx6 = (float)x6 / 1.0;
            float my6 = (float)y6 / 1.0;
            float mz6 = (float)z6 / 0.25;

            Serial.print("MX6, MY6, MZ6 = ");
            Serial.print(mx6);
            Serial.print(", ");
            Serial.print(my6);
            Serial.print(", ");
            Serial.print(mz6);
            Serial.println(" Gauss");

            
        }
        else
        {
            Serial.print("7 - Unable to read the ALS31300. error = ");
            Serial.println(error);
            break;
        }
    }
}


void readALS31300ADC_7(int busAddress_7)
{    
   

    for (int count7 = 0; count7 < 2; ++count7)
    {
        // Write the address that is going to be read from the ALS31300
        Wire.beginTransmission(busAddress_7);
        Wire.write(0x28);
        uint16_t error = Wire.endTransmission(false);

        // The ALS31300 accepted the address
        if (error == kNOERROR)
        {
            // Start the read and request 8 bytes
            // which are the contents of register 0x28 and 0x29
            Wire.requestFrom(busAddress_7, 8);
            
            // Read the first 4 bytes which are the contents of register 0x28
            uint32_t value0x28 = Wire.read() << 24;
            value0x28 += Wire.read() << 16;
            value0x28 += Wire.read() << 8;
            value0x28 += Wire.read();

            // Read the next 4 bytes which are the contents of register 0x29
            uint32_t value0x29 = Wire.read() << 24;
            value0x29 += Wire.read() << 16;
            value0x29 += Wire.read() << 8;
            value0x29 += Wire.read();

            // Take the most significant byte of each axis from register 0x28 and combine it with the least
            // significant 4 bits of each axis from register 0x29, then sign extend the 12th bit.
            int x7 = SignExtendBitfield(((value0x28 >> 20) & 0x0FF0) | ((value0x29 >> 16) & 0x0F), 12);
            int y7 = SignExtendBitfield(((value0x28 >> 12) & 0x0FF0) | ((value0x29 >> 12) & 0x0F), 12);
            int z7 = SignExtendBitfield(((value0x28 >> 4) & 0x0FF0) | ((value0x29 >> 8) & 0x0F), 12);

            // Display the values of x, y and z
           // Serial.print("Count2, X2, Y2, Z2 = ");
           // Serial.print(count2);
            //Serial.print(", ");
            //Serial.print(x2);
            //Serial.print(", ");
            //Serial.print(y2);
           // Serial.print(", ");
            //Serial.println(z2);

            // Look at the datasheet for the sensitivity of the part used.
            // In this case, full scale range is 500 gauss, other sensitivities
            // are 1000 gauss and 2000 gauss
            float mx7 = (float)x7 / 1.0;
            float my7 = (float)y7 / 1.0;
            float mz7 = (float)z7 / 0.25;

            Serial.print("MX7, MY7, MZ7 = ");
            Serial.print(mx7);
            Serial.print(", ");
            Serial.print(my7);
            Serial.print(", ");
            Serial.print(mz7);
            Serial.println(" Gauss");

            
        }
        else
        {
            Serial.print("7 - Unable to read the ALS31300. error = ");
            Serial.println(error);
            break;
        }
    }
}


void readALS31300ADC_8(int busAddress_8)
{    
   

    for (int count8 = 0; count8 < 2; ++count8)
    {
        // Write the address that is going to be read from the ALS31300
        Wire.beginTransmission(busAddress_8);
        Wire.write(0x28);
        uint16_t error = Wire.endTransmission(false);

        // The ALS31300 accepted the address
        if (error == kNOERROR)
        {
            // Start the read and request 8 bytes
            // which are the contents of register 0x28 and 0x29
            Wire.requestFrom(busAddress_8, 8);
            
            // Read the first 4 bytes which are the contents of register 0x28
            uint32_t value0x28 = Wire.read() << 24;
            value0x28 += Wire.read() << 16;
            value0x28 += Wire.read() << 8;
            value0x28 += Wire.read();

            // Read the next 4 bytes which are the contents of register 0x29
            uint32_t value0x29 = Wire.read() << 24;
            value0x29 += Wire.read() << 16;
            value0x29 += Wire.read() << 8;
            value0x29 += Wire.read();

            // Take the most significant byte of each axis from register 0x28 and combine it with the least
            // significant 4 bits of each axis from register 0x29, then sign extend the 12th bit.
            int x8 = SignExtendBitfield(((value0x28 >> 20) & 0x0FF0) | ((value0x29 >> 16) & 0x0F), 12);
            int y8 = SignExtendBitfield(((value0x28 >> 12) & 0x0FF0) | ((value0x29 >> 12) & 0x0F), 12);
            int z8 = SignExtendBitfield(((value0x28 >> 4) & 0x0FF0) | ((value0x29 >> 8) & 0x0F), 12);

            // Display the values of x, y and z
           // Serial.print("Count2, X2, Y2, Z2 = ");
           // Serial.print(count2);
            //Serial.print(", ");
            //Serial.print(x2);
            //Serial.print(", ");
            //Serial.print(y2);
           // Serial.print(", ");
            //Serial.println(z2);

            // Look at the datasheet for the sensitivity of the part used.
            // In this case, full scale range is 500 gauss, other sensitivities
            // are 1000 gauss and 2000 gauss
            float mx8 = (float)x8 / 1.0;
            float my8 = (float)y8 / 1.0;
            float mz8 = (float)z8 / 0.25;

            Serial.print("MX8, MY8, MZ8 = ");
            Serial.print(mx8);
            Serial.print(", ");
            Serial.print(my8);
            Serial.print(", ");
            Serial.print(mz8);
            Serial.println(" Gauss");

            
        }
        else
        {
            Serial.print("7 - Unable to read the ALS31300. error = ");
            Serial.println(error);
            break;
        }
    }
}

void readALS31300ADC_9(int busAddress_9)
{    
   

    for (int count9 = 0; count9 < 2; ++count9)
    {
        // Write the address that is going to be read from the ALS31300
        Wire.beginTransmission(busAddress_9);
        Wire.write(0x28);
        uint16_t error = Wire.endTransmission(false);

        // The ALS31300 accepted the address
        if (error == kNOERROR)
        {
            // Start the read and request 8 bytes
            // which are the contents of register 0x28 and 0x29
            Wire.requestFrom(busAddress_9, 8);
            
            // Read the first 4 bytes which are the contents of register 0x28
            uint32_t value0x28 = Wire.read() << 24;
            value0x28 += Wire.read() << 16;
            value0x28 += Wire.read() << 8;
            value0x28 += Wire.read();

            // Read the next 4 bytes which are the contents of register 0x29
            uint32_t value0x29 = Wire.read() << 24;
            value0x29 += Wire.read() << 16;
            value0x29 += Wire.read() << 8;
            value0x29 += Wire.read();

            // Take the most significant byte of each axis from register 0x28 and combine it with the least
            // significant 4 bits of each axis from register 0x29, then sign extend the 12th bit.
            int x9 = SignExtendBitfield(((value0x28 >> 20) & 0x0FF0) | ((value0x29 >> 16) & 0x0F), 12);
            int y9 = SignExtendBitfield(((value0x28 >> 12) & 0x0FF0) | ((value0x29 >> 12) & 0x0F), 12);
            int z9 = SignExtendBitfield(((value0x28 >> 4) & 0x0FF0) | ((value0x29 >> 8) & 0x0F), 12);

            // Display the values of x, y and z
           // Serial.print("Count2, X2, Y2, Z2 = ");
           // Serial.print(count2);
            //Serial.print(", ");
            //Serial.print(x2);
            //Serial.print(", ");
            //Serial.print(y2);
           // Serial.print(", ");
            //Serial.println(z2);

            // Look at the datasheet for the sensitivity of the part used.
            // In this case, full scale range is 500 gauss, other sensitivities
            // are 1000 gauss and 2000 gauss
            float mx9 = (float)x9 / 1.0;
            float my9 = (float)y9 / 1.0;
            float mz9 = (float)z9 / 0.25;

            Serial.print("MX9, MY9, MZ9 = ");
            Serial.print(mx9);
            Serial.print(", ");
            Serial.print(my9);
            Serial.print(", ");
            Serial.print(mz9);
            Serial.println(" Gauss");

            
        }
        else
        {
            Serial.print("7 - Unable to read the ALS31300. error = ");
            Serial.println(error);
            break;
        }
    }
}

void readALS31300ADC_10(int busAddress_10)
{    
   

    for (int count10 = 0; count10 < 2; ++count10)
    {
        // Write the address that is going to be read from the ALS31300
        Wire.beginTransmission(busAddress_10);
        Wire.write(0x28);
        uint16_t error = Wire.endTransmission(false);

        // The ALS31300 accepted the address
        if (error == kNOERROR)
        {
            // Start the read and request 8 bytes
            // which are the contents of register 0x28 and 0x29
            Wire.requestFrom(busAddress_10, 8);
            
            // Read the first 4 bytes which are the contents of register 0x28
            uint32_t value0x28 = Wire.read() << 24;
            value0x28 += Wire.read() << 16;
            value0x28 += Wire.read() << 8;
            value0x28 += Wire.read();

            // Read the next 4 bytes which are the contents of register 0x29
            uint32_t value0x29 = Wire.read() << 24;
            value0x29 += Wire.read() << 16;
            value0x29 += Wire.read() << 8;
            value0x29 += Wire.read();

            // Take the most significant byte of each axis from register 0x28 and combine it with the least
            // significant 4 bits of each axis from register 0x29, then sign extend the 12th bit.
            int x10 = SignExtendBitfield(((value0x28 >> 20) & 0x0FF0) | ((value0x29 >> 16) & 0x0F), 12);
            int y10 = SignExtendBitfield(((value0x28 >> 12) & 0x0FF0) | ((value0x29 >> 12) & 0x0F), 12);
            int z10 = SignExtendBitfield(((value0x28 >> 4) & 0x0FF0) | ((value0x29 >> 8) & 0x0F), 12);

            // Display the values of x, y and z
           // Serial.print("Count2, X2, Y2, Z2 = ");
           // Serial.print(count2);
            //Serial.print(", ");
            //Serial.print(x2);
            //Serial.print(", ");
            //Serial.print(y2);
           // Serial.print(", ");
            //Serial.println(z2);

            // Look at the datasheet for the sensitivity of the part used.
            // In this case, full scale range is 500 gauss, other sensitivities
            // are 1000 gauss and 2000 gauss
            float mx10 = (float)x10 / 1.0;
            float my10 = (float)y10 / 1.0;
            float mz10 = (float)z10 / 0.25;

            Serial.print("MX10, MY10, MZ10 = ");
            Serial.print(mx10);
            Serial.print(", ");
            Serial.print(my10);
            Serial.print(", ");
            Serial.print(mz10);
            Serial.println(" Gauss");

            
        }
        else
        {
            Serial.print("7 - Unable to read the ALS31300. error = ");
            Serial.println(error);
            break;
        }
    }
}

void readALS31300ADC_11(int busAddress_11)
{    
   

    for (int count11 = 0; count11 < 2; ++count11)
    {
        // Write the address that is going to be read from the ALS31300
        Wire.beginTransmission(busAddress_11);
        Wire.write(0x28);
        uint16_t error = Wire.endTransmission(false);

        // The ALS31300 accepted the address
        if (error == kNOERROR)
        {
            // Start the read and request 8 bytes
            // which are the contents of register 0x28 and 0x29
            Wire.requestFrom(busAddress_11, 8);
            
            // Read the first 4 bytes which are the contents of register 0x28
            uint32_t value0x28 = Wire.read() << 24;
            value0x28 += Wire.read() << 16;
            value0x28 += Wire.read() << 8;
            value0x28 += Wire.read();

            // Read the next 4 bytes which are the contents of register 0x29
            uint32_t value0x29 = Wire.read() << 24;
            value0x29 += Wire.read() << 16;
            value0x29 += Wire.read() << 8;
            value0x29 += Wire.read();

            // Take the most significant byte of each axis from register 0x28 and combine it with the least
            // significant 4 bits of each axis from register 0x29, then sign extend the 12th bit.
            int x11 = SignExtendBitfield(((value0x28 >> 20) & 0x0FF0) | ((value0x29 >> 16) & 0x0F), 12);
            int y11 = SignExtendBitfield(((value0x28 >> 12) & 0x0FF0) | ((value0x29 >> 12) & 0x0F), 12);
            int z11 = SignExtendBitfield(((value0x28 >> 4) & 0x0FF0) | ((value0x29 >> 8) & 0x0F), 12);

            // Display the values of x, y and z
           // Serial.print("Count2, X2, Y2, Z2 = ");
           // Serial.print(count2);
            //Serial.print(", ");
            //Serial.print(x2);
            //Serial.print(", ");
            //Serial.print(y2);
           // Serial.print(", ");
            //Serial.println(z2);

            // Look at the datasheet for the sensitivity of the part used.
            // In this case, full scale range is 500 gauss, other sensitivities
            // are 1000 gauss and 2000 gauss
            float mx11 = (float)x11 / 1.0;
            float my11 = (float)y11 / 1.0;
            float mz11 = (float)z11 / 0.25;

            Serial.print("MX11, MY11, MZ11 = ");
            Serial.print(mx11);
            Serial.print(", ");
            Serial.print(my11);
            Serial.print(", ");
            Serial.print(mz11);
            Serial.println(" Gauss");

            
        }
        else
        {
            Serial.print("7 - Unable to read the ALS31300. error = ");
            Serial.println(error);
            break;
        }
    }
}

void readALS31300ADC_12(int busAddress_12)
{    
   

    for (int count12 = 0; count12 < 2; ++count12)
    {
        // Write the address that is going to be read from the ALS31300
        Wire.beginTransmission(busAddress_12);
        Wire.write(0x28);
        uint16_t error = Wire.endTransmission(false);

        // The ALS31300 accepted the address
        if (error == kNOERROR)
        {
            // Start the read and request 8 bytes
            // which are the contents of register 0x28 and 0x29
            Wire.requestFrom(busAddress_12, 8);
            
            // Read the first 4 bytes which are the contents of register 0x28
            uint32_t value0x28 = Wire.read() << 24;
            value0x28 += Wire.read() << 16;
            value0x28 += Wire.read() << 8;
            value0x28 += Wire.read();

            // Read the next 4 bytes which are the contents of register 0x29
            uint32_t value0x29 = Wire.read() << 24;
            value0x29 += Wire.read() << 16;
            value0x29 += Wire.read() << 8;
            value0x29 += Wire.read();

            // Take the most significant byte of each axis from register 0x28 and combine it with the least
            // significant 4 bits of each axis from register 0x29, then sign extend the 12th bit.
            int x12 = SignExtendBitfield(((value0x28 >> 20) & 0x0FF0) | ((value0x29 >> 16) & 0x0F), 12);
            int y12 = SignExtendBitfield(((value0x28 >> 12) & 0x0FF0) | ((value0x29 >> 12) & 0x0F), 12);
            int z12 = SignExtendBitfield(((value0x28 >> 4) & 0x0FF0) | ((value0x29 >> 8) & 0x0F), 12);

            // Display the values of x, y and z
           // Serial.print("Count2, X2, Y2, Z2 = ");
           // Serial.print(count2);
            //Serial.print(", ");
            //Serial.print(x2);
            //Serial.print(", ");
            //Serial.print(y2);
           // Serial.print(", ");
            //Serial.println(z2);

            // Look at the datasheet for the sensitivity of the part used.
            // In this case, full scale range is 500 gauss, other sensitivities
            // are 1000 gauss and 2000 gauss
            float mx12 = (float)x12 / 1.0;
            float my12 = (float)y12 / 1.0;
            float mz12 = (float)z12 / 0.25;

            Serial.print("MX12, MY12, MZ12 = ");
            Serial.print(mx12);
            Serial.print(", ");
            Serial.print(my12);
            Serial.print(", ");
            Serial.print(mz12);
            Serial.println(" Gauss");

            
        }
        else
        {
            Serial.print("7 - Unable to read the ALS31300. error = ");
            Serial.println(error);
            break;
        }
    }
}


long SignExtendBitfield(uint32_t data, int width)
{
    long x = (long)data;
    long mask = 1L << (width - 1);

    if (width < 32)
    {
        x = x & ((1 << width) - 1); // make sure the upper bits are zero
    }

    return (long)((x ^ mask) - mask);
}


