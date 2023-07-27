#include "wifi.h"

void setupWifiBLE()
{
    BLE.addCharacteristic(wifiCredentialsCharacteristic);
}

/**
 * Called when the 
*/
void onReceiveCredentials(const uint8_t* data, size_t len, const BlePeerDevice& peer, void* context)
{
    char* credentials = (char*) data;

    // Parse Credentials
    char* ssid = strtok(credentials, CREDENTIAL_DELIM);
    char* password = strtok(credentials, CREDENTIAL_DELIM);
    char* auth_str = strtok(credentials, CREDENTIAL_DELIM);
    int auth = atoi(auth_str);

    WiFi.setCredentials(ssid, password, auth);
}
