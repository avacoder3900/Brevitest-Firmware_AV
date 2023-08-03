#include "wifi.h"

// ---------- Constants ---------- // 
#define CREDENTIAL_DELIM ","    // Delimiter used to separate sent credentials.

#define GET_CREDENTIALS_DELAY 1000  // Delay between credential checks.
#define WIFI_CONNECT_DELAY 1000     // Delay between WiFi connection attempts.

// Response codes
#define CRED_RECV_RESPONSE "Credentials Received"   // Sent when the SPU receives credentials.
#define WIFI_CONNECTED_RESPONSE "Connected to WiFi" // Sent when the SPU is connected to WiFi.

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
    BleCharacteristicProperty::WRITE_WO_RSP,
    wifiCredentialsUuid,
    wifiCredentialsService,
    onReceiveCredentials,
    NULL
);

// Characteristic for sending a response to the device that sent credentials.
BleCharacteristic wifiResponseCharacterisic(
    "wifi-response",
    BleCharacteristicProperty::NOTIFY,
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

void send_byte(uint8_t byte)
{
    wifiResponseCharacterisic.setValue(&byte);
}

void get_credentials()
{
    Log.info("=== Getting credentials...");

    // Advertise the credentials service
    BLE.advertise(&wifiAdvertisingData);
    BLE.on();
    WiFi.on();

    // Wait for credentials to be received or until timoeut.
    while(!WiFi.hasCredentials())
    {
        delay(GET_CREDENTIALS_DELAY);
    }

    Log.info("=== Credentials received");
}

void connect_to_wifi()
{
    // Get wifi credentials.
    get_credentials();

    // Try to connect to WiFi.
    Log.info("=== Connecting to WiFi...");
    WiFi.connect(WIFI_CONNECT_SKIP_LISTEN);
    delay(WIFI_CONNECT_DELAY);

    // Wait for WiFi to connect.
    while(!WiFi.ready())
    {
        WiFi.connect(WIFI_CONNECT_SKIP_LISTEN);
        delay(WIFI_CONNECT_DELAY);
    }
    Log.info("=== Connected to WiFi");

    // Inform the website that the SPU is connected to WiFi.
    wifiResponseCharacterisic.setValue(WIFI_CONNECTED_RESPONSE);

    // Stop advertising the credentials service.
    BLE.stopAdvertising();
}

void fast_connect()
{
    Log.info("=== Connecting to WiFi...");
    WiFi.connect(WIFI_CONNECT_SKIP_LISTEN);
    delay(WIFI_CONNECT_DELAY);

    // Wait for WiFi to connect.
    while(!WiFi.ready())
    {
        WiFi.connect(WIFI_CONNECT_SKIP_LISTEN);
        delay(WIFI_CONNECT_DELAY);
    }
    Log.info("=== Connected to WiFi");
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
    Log.info(credentials);

    char* save_ptr = credentials;

    // Check if the device that sent the credentials has the correct auth key.
    char* ble_auth = strtok_r(credentials, CREDENTIAL_DELIM, &save_ptr);
    Log.info("BLE Auth Key: %s", bleAuthKey);
    if(strcmp(ble_auth, bleAuthKey) != 0)
    {
        Log.info("Invalid BLE Auth Key");
        return;
    }

    // Parse Credentials
    char* ssid = strtok_r(NULL, CREDENTIAL_DELIM, &save_ptr);
    Log.info("SSID: %s", ssid);
    char* password = strtok_r(NULL, CREDENTIAL_DELIM, &save_ptr);
    Log.info("Password: %s", password);
    char* auth_str = strtok_r(NULL, CREDENTIAL_DELIM, &save_ptr);
    Log.info("Auth: %s", auth_str);
    int auth = atoi(auth_str);

    // Set WiFi Credentials
    WiFi.clearCredentials();
    WiFi.setCredentials(ssid, password, auth);

    // Notify the device that sent the credentials that the credentials were received.
    wifiResponseCharacterisic.setValue(CRED_RECV_RESPONSE);
}