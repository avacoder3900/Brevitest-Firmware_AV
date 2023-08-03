#ifndef WIFI_H
#define WIFI_H

#define CREDENTIALS_NO_TIMOEUT -1

void setup_wifi_ble();
void connect_to_wifi();
void onReceiveCredentials(const uint8_t* data, size_t len, const BlePeerDevice& peer, void* context);
void fast_connect();

#endif // WIFI_H