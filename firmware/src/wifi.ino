#include "wifi.h"

#define CREDENTIAL_DELIM ","

// ---------- BLE Service ---------- // 
// Key used to authenticate the device that sends credentials.
char bleAuthKey[] = "79b45686-f959-49b2-9d1b-fbafeaaf0293";
// Service and Characteristic UUIDs
BleUuid wifiCredentialsService("0a280af2-975f-4a79-a5d1-e71c986d1e9a"); 
BleUuid wifiCredentialsUuid("d99cf743-a4b8-4ef0-b4e8-b4eb445692e1");
BleUuid wifiResponseUuid("2fb441e2-29a2-4142-8cc3-88d0e353d452");

// Characteristic for receiving credentials.
BleCharacteristic wifiCredentialsCharacteristic(
    "wifi-credentials", 
    BleCharacteristicProperty::WRITE,
    wifiCredentialsUuid,
    wifiCredentialsService,
    onReceiveCredentials,
    NULL
);

// Characteristic for sending a response to the device that sent credentials.
BleCharacteristic wifiResponseCharacterisic(
    "wifi-response", 
    BleCharacteristicProperty::READ,
    wifiResponseUuid,
    wifiCredentialsService,
    NULL,
    NULL
);

BleAdvertisingData wifiAdvertisingData;

// ---------- BLE Setup ---------- // 
/**
 * Setup the credentials BLE service and characteristics.
 */
void setup_wifi_ble()
{
    BLE.addCharacteristic(wifiCredentialsCharacteristic);
    BLE.addCharacteristic(wifiResponseCharacterisic);
    wifiAdvertisingData.appendServiceUUID(wifiCredentialsService);
}

/**
 * Start listening for credentials from a bluetooth device.
 */
void start_listen_for_credentials()
{
    BLE.advertise(&wifiAdvertisingData);
}

/**
 * Stop listening for credentials from a bluetooth device.
 */
void stop_listen_for_credentials()
{
    BLE.stopAdvertising();
}

/**
 * Handler that is called whenever a bluetooth device sends credentials to the SPU.
 * 
 * Authenticates the device then stores the given credentials.
 * 
 * @param data The data that was sent.
 * @param len The length of the data.
 * @param peer The peer device that sent the data.
 * @param context The context of the handler.
 */
void onReceiveCredentials(const uint8_t* data, size_t len, const BlePeerDevice& peer, void* context)
{
    char* credentials = (char*) data;
    char* save_ptr = credentials;

    // Check if the device that sent the credentials has the correct auth key.
    char* ble_auth = strtok_r(credentials, CREDENTIAL_DELIM, &save_ptr);
    if(ble_auth != bleAuthKey) 
    {
        Log.info("Invalid BLE Auth Key");
        return;
    }

    // Parse Credentials
    char* ssid = strtok_r(NULL, CREDENTIAL_DELIM, &save_ptr);
    char* password = strtok_r(NULL, CREDENTIAL_DELIM, &save_ptr);
    char* auth_str = strtok_r(NULL, CREDENTIAL_DELIM, &save_ptr);
    int auth = atoi(auth_str);

    // Set WiFi Credentials
    WiFi.setCredentials(ssid, password, auth);

    // Notify the device that sent the credentials that the credentials were received.
    wifiResponseCharacterisic.setValue("Credentials Received");
}