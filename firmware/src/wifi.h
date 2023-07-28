#ifndef WIFI_H
#define WIFI_H

void setup_wifi_ble();
void onReceiveCredentials(const uint8_t* data, size_t len, const BlePeerDevice& peer, void* context);
void start_listen_for_credentials();
void stop_listen_for_credentials();

#endif // WIFI_H