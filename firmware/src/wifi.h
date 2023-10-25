#ifndef WIFI_H
#define WIFI_H

/////////////////////////////////////////////////////////////
//                                                         //
//                        CONSTAANTS                       //
//                                                         //
/////////////////////////////////////////////////////////////
#define CREDENTIAL_DELIM ","    // Delimiter used to separate sent credentials.

#define GET_CREDENTIALS_DELAY 1000  // Delay between credential checks.
#define WIFI_CONNECT_DELAY 1000     // Delay between WiFi connection attempts.

#define CRED_RECV_RESPONSE "RECV"   // Sent when the SPU receives credentials.

#define BLE_CONNECTION_INTERVAL 30000       // The amount of time the SPU will broadcast the credentials service when connected to WiFi.
#define BLE_CONNECTION_CHECK_INTERVAL 1000  // The amount of time between checks for a dropped BLE connection.

#define BLE_AUTH_KEY "e0a21657-4c92-4700-b3b2-035ce274312d" // Key used to authenticate the device that sends credentials.

// Service and Characteristic UUIDs.
#define CREDENTIALS_SERVICE_UUID "0a280af2-975f-4a79-a5d1-e71c986d1e9a" 
#define CREDENTIALS_CHARACTERISTIC_UUID "d99cf743-a4b8-4ef0-b4e8-b4eb445692e1"
#define RESPONSE_CHARACTERISTIC_UUID "2fb441e2-29a2-4142-8cc3-88d0e353d452"

// Names of the characteristics.
#define CREDENTIALS_CHARACTERISTIC_NAME "wifi-credentials"
#define RESPONSE_CHARACTERISTIC_NAME "wifi-response"


/////////////////////////////////////////////////////////////
//                                                         //
//                       PROTOTYPES                        //
//                                                         //
/////////////////////////////////////////////////////////////
void setup_credentials_ble();
void connect_to_wifi();
void onReceiveCredentials(const uint8_t* data, size_t len, const BlePeerDevice& peer, void* context);

#endif // WIFI_H