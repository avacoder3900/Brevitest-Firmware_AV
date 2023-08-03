#include "wifi.h"

// ---------- Constants ---------- // 
#define CREDENTIAL_DELIM ","    // Delimiter used to separate sent credentials.

#define GET_CREDENTIALS_DELAY 1000  // Delay between credential checks.
#define WIFI_CONNECT_DELAY 1000     // Delay between WiFi connection attempts.

#define CRED_RECV_RESPONSE "Credentials Received"   // Sent when the SPU receives credentials.
#define WIFI_CONNECTED_RESPONSE "Connected to WiFi" // Sent when the SPU is connected to WiFi.

#define BLE_CONNECTION_INTERVAL 30000       // The amount of time the SPU will broadcast the credentials service when connected to WiFi.
#define BLE_CONNECTION_CHECK_INTERVAL 1000  // 

/////////////////////////////////////////////////////////////
//                                                         //
//                       CREDENTIALS                       //
//                                                         //
/////////////////////////////////////////////////////////////

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


/////////////////////////////////////////////////////////////
//                                                         //
//                         TIMER                           //
//                                                         //
/////////////////////////////////////////////////////////////

void ble_connection_timer_callback();
Timer ble_connection_timer(BLE_CONNECTION_INTERVAL, ble_connection_timer_callback, false);

bool connection_check = false;  // Flag used to determine if the SPU should check for a dropped BLE connection.

/**
 * Callback for the BLE connection timer.
 * 
 * Manages the interval allowed for BLE advertising when the SPU is connected to WiFi.
 * 
 * The timer is started after the SPU connects to WiFi, until the timer runs out and this 
 * function is called, the SPU will continue to advertise. After this function is called
 * for the first time advertising will stop if there is not a BLE connection.
 * 
 * If there is a BLE connection the funciton will periodically be called to check if the 
 * connection is still active, and to stop advertising if it is not.
 */
void ble_connection_timer_callback() 
{
    // Check if the connection interval is over, if so, start checking more frequently for dropped conenction.
    if (!connection_check) {
        connection_check = true;
        ble_connection_timer.changePeriod(BLE_CONNECTION_CHECK_INTERVAL);
    }

    // If the BLE connection is dropped, stop BLE, stop the timer.
    if (!BLE.connected()) {
        Log.info("WiFi: BLE no longer connected, stoppping advertising.");
        BLE.stopAdvertising();
        ble_connection_timer.stop();
    }
}

/////////////////////////////////////////////////////////////
//                                                         //
//                     WIFI CONNECTION                     //
//                                                         //
/////////////////////////////////////////////////////////////
/**
 * Setup the credentials BLE service and characteristics.
 */
void setup_credentials_ble()
{
    BLE.addCharacteristic(wifiCredentialsCharacteristic);
    BLE.addCharacteristic(wifiResponseCharacterisic);
    wifiAdvertisingData.appendServiceUUID(wifiCredentialsService);
}

/**
 * If the SPU does not currently have WiFi credentials, waits for the credentials to be received via bluetooth.
 * 
 * Upon reciving credentials, the SPU will restart.
 */
void get_credentials()
{
    if(!WiFi.hasCredentials()) {
        Log.info("WiFi: No Credentials, waiting for credentials...");

        // Wait for credentials to be received.
        while(!WiFi.hasCredentials())
        {
            delay(GET_CREDENTIALS_DELAY);
        }

        // The SPU has received credentials, the handler that sets them will restart the SPU.
    }
}

/**
 * Waits for the SPU to connect to WiFi.
 */
void wait_for_wifi_connect()
{
    Log.info("WiFi: Connnecting to WiFi...");
    // Try to connect to WiFi.
    WiFi.connect(WIFI_CONNECT_SKIP_LISTEN);
    delay(WIFI_CONNECT_DELAY);

    // Wait for WiFi the to connect.
    while(!WiFi.ready())
    {
        WiFi.connect(WIFI_CONNECT_SKIP_LISTEN);
        delay(WIFI_CONNECT_DELAY);
    }
    Log.info("WiFi: Connected to WiFi.");
}

/**
 * Connects the SPU to WiFi.
 * 
 * Turns on wifi credentials bluetooth service
 * 
 * If the SPU does not currently have WiFi credentials, waits for the credentials to be received via bluetooth.
 *
 * Will block until the 
 * 
 */
void connect_to_wifi() 
{
    Log.info("WiFi: Begin WiFi Connection process.");
    // Advertise the credentials service
    BLE.advertise(&wifiAdvertisingData);
    BLE.on();
    WiFi.on();

    // If the SPU does not currently have WiFi credentials, it will wiat for them then restart the SPU.
    get_credentials();
    // Waits for wifi to connect, will continue to accept new credentials, if new ones are received, restarts the SPU.
    wait_for_wifi_connect();

    // Inform the website that the SPU is connected to WiFi.
    wifiResponseCharacterisic.setValue(WIFI_CONNECTED_RESPONSE);

    ble_connection_timer.start();
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
    if(strcmp(ble_auth, bleAuthKey) != 0)
    {
        Log.info("WiFi: Invalid BLE Auth Key");
        return;
    }

    // Parse Credentials
    char* ssid = strtok_r(NULL, CREDENTIAL_DELIM, &save_ptr);
    char* password = strtok_r(NULL, CREDENTIAL_DELIM, &save_ptr);
    char* auth_str = strtok_r(NULL, CREDENTIAL_DELIM, &save_ptr);
    int auth = atoi(auth_str);

    Log.info("WiFi: Received Credentials: SSID: %s, PASSWORD: %s, AUTH: %d", ssid, password, auth);

    // Set WiFi Credentials
    WiFi.setCredentials(ssid, password, auth);

    // Notify the device that sent the credentials that the credentials were received.
    wifiResponseCharacterisic.setValue(CRED_RECV_RESPONSE);
    delay(100);

    Log.info("Wifi: Credentials Received and Set");
    System.reset();
}