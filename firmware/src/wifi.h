#ifndef WIFI_H
#define WIFI_H

void setup_credentials_ble();
void connect_to_wifi();
void onReceiveCredentials(const uint8_t* data, size_t len, const BlePeerDevice& peer, void* context);

#endif // WIFI_H