#ifndef WIFI_H
#define WIFI_H

#define CREDENTIAL_DELIM ","

void setupWifiBLE();
void onWifiCredentialsWrite(const uint8_t* data, size_t len, const BlePeerDevice& peer, void* context);

// @todo confirm UUIDs are okay
BleUuid wifiCredentialsService("0a280af2-975f-4a79-a5d1-e71c986d1e9a");

BleUuid wifiCredentialsUuid("d99cf743-a4b8-4ef0-b4e8-b4eb445692e1");
BleUuid wifiResponseUuid("2fb441e2-29a2-4142-8cc3-88d0e353d452");

BleCharacteristic wifiCredentialsCharacteristic(
    "wifi-credentials", 
    BleCharacteristicProperty::WRITE,
    wifiCredentialsUuid,
    wifiCredentialsService,
    onReceiveCredentials,
    NULL
);

BleCharacteristic wifiResponseCharacterisic(
    "wifi-response", 
    BleCharacteristicProperty::READ,
    wifiResponseUuid,
    wifiCredentialsService,
    NULL,
    NULL
);

#endif // WIFI_H