/*
  This file is part of the iLabsEspAT library for iLabs Challenger
  products: https://github.com/PontusO/iLabs_EspAT
  Copyright 2023 Pontus Oldberg.

  Parts of this library was forked from  https://github.com/jandrassy/WiFiEspAT
  which is Copyright 2019 Juraj Andrassy.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public License
  along with this library.  If not, see <https://www.gnu.org/licenses/>.
*/

#include "BLE.h"
#include "utility/EspAtDrv.h"

// Bluetooth Base UUID: 00000000-0000-1000-8000- 00805F9B34FB
static const uint8_t sdp_bluetooth_base_uuid[] = { 0x00, 0x00, 0x00, 0x00, /* - */ 0x00, 0x00, /* - */ 0x10, 0x00, /* - */
    0x80, 0x00, /* - */ 0x00, 0x80, 0x5F, 0x9B, 0x34, 0xFB };

static const uint8_t iBeaconAdvertisement01[] = { 0x02, 0x01, 0x06 };
static const uint8_t iBeaconAdvertisement38[] = { 0x1a, 0xff, 0x4c, 0x00, 0x02, 0x15 };

static void (*bleScanResultCallback)(BLEScanResult &bleScanResult) = NULL;
static void (*bleScanDoneCallback)(void) = NULL;
static void (*bleConnectionCallback)(int conn_index) = NULL;
static void (*bleDisconnectCallback)(int conn_index) = NULL;
static void (*bleParameterUpdateCallback)(int conn_index) = NULL;
static void (*bleMtuUpdateCallback)(int conn_index) = NULL;
static void (*blePhyUpdateCallback)(int conn_index) = NULL;
static void (*bleGATTServiceDiscoveredCallback)(int service_index) = NULL;

static bool _isScanning = false;
static char blescanResponse[256];
static int scanline = 0;
static BLEScanResult scanResult;
static BLEConnection *bleConnections[MAX_BLE_CONNECTIONS] = { NULL };

std::list<BLEPrimaryGattService> primaryGattServices;
std::list<BLEGattIncludedService> includedGattServices;
std::list<BLEGattCharacteristics> gattCharacteristics;

bool BLEManager::unsolicitedMessage(char *buffer) {
    bool result = false;
    // BLESCAN results are sent as 2 rows of data to us (line 0 and line 1)
    // So we repackage the data to one line here.
    if (scanline == 1) {
        int len = 256 - strlen(blescanResponse);
        strncat(blescanResponse, buffer, len);
        scanline = 0;
        Serial.println(blescanResponse);
        // Unpack data
        scanResult.bd_addr.setAddress(blescanResponse+10);

        const char s[2] = ",";
        char *iPtr = blescanResponse;
        char *token;

        // Parse out RSSI
        token = strsep(&iPtr, s);
        token = strsep(&iPtr, s);
        scanResult.rssi = strtol(token, NULL, 10);

        // Advertisement data
        token = strsep(&iPtr, s);
        int pos = 0;
        while(pos < 32 && *token != 0) {
            scanResult.adv_data[pos++] = *token++;
        }
        scanResult.adv_data[pos] = 0;

        // rsp data
        token = strsep(&iPtr, s);
        if (token) {
            while(pos < 32 && *token != 0) {
                scanResult.scan_rsp_data[pos++] = *token++;
            }
            scanResult.scan_rsp_data[pos] = 0;
        }

        // Remote advertised address type
        token = strsep(&iPtr, s);
        BD_ADDR_TYPE address_type = (BD_ADDR_TYPE)strtol(token, NULL, 10);
        scanResult.bd_addr.setAddressType(address_type);

        // Call the callback method if any has been defined.
        if (bleScanResultCallback)
            bleScanResultCallback(scanResult);

        result = true;    
    }
    if (strstr(buffer, "+BLESCAN:")) {
        scanline = 1;       // Next line
        strncpy(blescanResponse, buffer, 256);
        result = true;
    } else if (strstr(buffer, "+BLESCANDONE")) {
        _isScanning = false;
        result = true;

        if (bleScanDoneCallback)
            bleScanDoneCallback();
    } else if (strstr(buffer, "+BLECONN:")) {
        char *ptr = strchr(buffer, ':');
        if (ptr) {
            // Create a  new connection object.
            // When not needed anymore the user must delete it
            BLEConnection *newCon = new BLEConnection();
            newCon->conn_index = strtol(ptr+1, NULL, 10);
            ptr = strchr(buffer, ',');
            newCon->remote_address.setAddress(ptr+2);  // +2 to skip both , and " signs

            // Insert new connection into table.
            bleConnections[newCon->conn_index] = newCon;

            // Call user to inform on new connection.
            if (bleConnectionCallback)
                bleConnectionCallback(newCon->conn_index);
            result = true;
        } else {
            // Fail silently ?
            Serial.println("Didn't find response terminator !");
        }
    } else if (strstr(buffer, "+BLECONNPARAM:")) {
        char *ptr = strchr(buffer, ':');
        if (ptr) {                
            // Tokenize
            const char s[2] = ",";
            char *token;
            ptr++;
            token = strsep(&ptr, s);
            int index = strtol(token, NULL, 10);
            Serial.printf("bleConnection: %d\r\n", index);
            if (index < MAX_BLE_CONNECTIONS && bleConnections[index]) {
                token = strsep(&ptr, s);
                bleConnections[index]->min_interval = strtol(token, NULL, 10);
                token = strsep(&ptr, s);
                bleConnections[index]->max_interval = strtol(token, NULL, 10);
                token = strsep(&ptr, s);
                bleConnections[index]->cur_interval = strtol(token, NULL, 10);
                token = strsep(&ptr, s);
                bleConnections[index]->latency = strtol(token, NULL, 10);
                token = strsep(&ptr, s);
                bleConnections[index]->timeout = strtol(token, NULL, 10);

                if (bleParameterUpdateCallback)
                    bleParameterUpdateCallback(index);

                result = true;
            }
        } else {
            // Fail silently ?
            Serial.println("Didn't find response terminator !");
        }
    } else if (strstr(buffer, "+BLESETPHY:")) {
        // OK i am not entirely sure how this should work but here's my initial go at it.
        // The response (+BLESETPHY:) does not come with a connection handle, not sure why.
        // Instead we get the remote BD_ADDR which we can use to look up the corresponding
        // connection handle.
        // My implementation will search through the connection entries looking for the
        // remote address and then add the PHY values there. The index is then used in
        // the callback to simplify the API.
        char *ptr = strchr(buffer, ':');
        if (ptr) {
            // Tokenize
            const char s[2] = ",";
            char *token;
            ptr++;
            token = strsep(&ptr, s);
            // According to espressif documentation this response can come back with either
            // a BD_ADDR enclosed in citationmarks or s simple -1 without citation marks.
            if (*token == '-') {
                // OK, so this is probably from a set command so we know that it failed.
                // No need to do anything here for now.
                Serial.println("Attempt to set PHY failed !");
            } else {
                int i;
                // A proper address !
                token += 1;   // Skip the initial citation mark.
                BD_ADDR addr(token);
                // Check for the proper connection entry.
                for (i=0; i<MAX_BLE_CONNECTIONS; i++) {
                    if (bleConnections[i]->remote_address.matches(&addr))
                        break;
                }

                if (i < MAX_BLE_CONNECTIONS) {
                    token = strsep(&ptr, s);
                    bleConnections[i]->tx_PHY = (ble_advertisement_phy_t)strtol(token, NULL, 10);
                    token = strsep(&ptr, s);
                    bleConnections[i]->rx_PHY = (ble_advertisement_phy_t)strtol(token, NULL, 10);

                    if (blePhyUpdateCallback)
                        blePhyUpdateCallback(i);
                } else {
                    Serial.printf("Failed to find connection related to %s\r\n", addr.getAddressString());
                }
                result = true;
            }
        } else  {
            // Fail silently ?
            Serial.println("Didn't find response terminator !");
        }
    } else if (strstr(buffer, "+BLEDISCONN:")) {
        char *ptr = strchr(buffer, ':');
        if (ptr) {
            // Create a new connection object.
            // User must delete this and the already created connection object.
            BLEConnection *disCon = new BLEConnection();
            disCon->conn_index = strtol(ptr+1, NULL, 10);
            ptr = strchr(buffer, ',');
            disCon->remote_address.setAddress(ptr+2);  // +2 to skip both , and " signs

            if (bleConnections[disCon->conn_index]) {
                delete bleConnections[disCon->conn_index];
                bleConnections[disCon->conn_index] = NULL;
            }

            if (bleDisconnectCallback)
                bleDisconnectCallback(disCon->conn_index);

            delete disCon;
            result = true;
        } else {
            // Fail silently ?
            Serial.println("Didn't find response terminator !");
        }
    } else if (strstr(buffer, "+BLECFGMTU:")) {
        char *ptr = strchr(buffer, ':');
        if (ptr) {                
            // Tokenize
            const char s[2] = ",";
            char *token;
            ptr++;
            token = strsep(&ptr, s);
            int index = strtol(token, NULL, 10);
            Serial.printf("bleConnection: %d\r\n", index);
            if (index < MAX_BLE_CONNECTIONS && bleConnections[index]) {
                token = strsep(&ptr, s);
                bleConnections[index]->mtu_size = strtol(token, NULL, 10);

                if (bleMtuUpdateCallback)
                    bleMtuUpdateCallback(index);

                result = true;
            }
        } else {
            // Fail silently ?
            Serial.println("Didn't find response terminator !");
        }
    } else if (strstr(buffer, "+BLEGATTCPRIMSRV:")) {
        // +BLEGATTCPRIMSRV:0,1,0x1800,1
        result = true;
        char *ptr = strchr(buffer, ':');
        if (ptr) {                
            // Tokenize
            const char s[2] = ",";
            char *token;
            ptr++;
            token = strsep(&ptr, s);
            int index = strtol(token, NULL, 10);
            if (index >= 0 && index < MAX_BLE_CONNECTIONS) {
                BLEPrimaryGattService srvc;
                srvc.conn_index = index;
                token = strsep(&ptr, s);
                srvc.srv_index = strtol(token, NULL, 10);
                // UUID is expressed as a hex number here, prepended with 0x.
                token = strsep(&ptr, s);
                token = strchr(token, 'x') + 1;
                srvc.srv_uuid.setUUID(token);
                token = strsep(&ptr, s);
                srvc.srv_type = (BLEServiceType_t)strtol(token, NULL, 10);
                // Add to list of primary services.
                Serial.println("Adding service to list !");
                primaryGattServices.push_back(srvc);
            }
        }
    } else if (strstr(buffer, "+BLEGATTCCHAR")) {
        result = true;
        char *ptr = strchr(buffer, ':');
        if (ptr) {
            BLEGattCharacteristics chrctr;
            if (strstr(buffer, "char")) {
                chrctr.gatt_char_type = BLE_GATT_CHARACTERISTIC;
                ptr = strchr(buffer, ',') + 1;
                // Tokenize
                const char s[2] = ",";
                char *token;
                token = strsep(&ptr, s);
                chrctr.char_index = strtol(token, NULL, 10);
                token = strsep(&ptr, s);
                chrctr.srv_index = strtol(token, NULL, 10);
                token = strsep(&ptr, s);
                chrctr.char_index = strtol(token, NULL, 10);
                // UUID is expressed as a hex number here, prepended with 0x.
                token = strsep(&ptr, s);
                token = strchr(token, 'x') + 1;
                chrctr.char_uuid.setUUID(token);
                // characteristics property is a hex number here, prepended with 0x.
                token = strsep(&ptr, s);
                token = strchr(token, 'x') + 1;
                chrctr.char_prop = strtol(token, NULL, 16);
            } else {
                chrctr.gatt_char_type = BLE_GATT_DESCRIPTOR;
                ptr = strchr(buffer, ',') + 1;
                // Tokenize
                const char s[2] = ",";
                char *token;
                token = strsep(&ptr, s);
                chrctr.char_index = strtol(token, NULL, 10);
                token = strsep(&ptr, s);
                chrctr.srv_index = strtol(token, NULL, 10);
                token = strsep(&ptr, s);
                chrctr.char_index = strtol(token, NULL, 10);
                token = strsep(&ptr, s);
                chrctr.desc_index = strtol(token, NULL, 10);
                // UUID is expressed as a hex number here, prepended with 0x.
                token = strsep(&ptr, s);
                token = strchr(token, 'x') + 1;
                chrctr.desc_uuid.setUUID(token);
            }
            Serial.println("Adding characteristic to list !");
            gattCharacteristics.push_back(chrctr);
        }
    }  else if (strstr(buffer, "+BLEGATTCINCLSRV")) {
        result = true;
        char *ptr = strchr(buffer, ':');
        if (ptr) {
            BLEGattIncludedService gis;
            // Tokenize
            const char s[2] = ",";
            char *token;
            ptr++;
            token = strsep(&ptr, s);
            gis.conn_index = strtol(token, NULL, 10);
            token = strsep(&ptr, s);
            gis.srv_index = strtol(token, NULL, 10);
            // UUID is expressed as a hex number here, prepended with 0x.
            token = strsep(&ptr, s);
            token = strchr(token, 'x') + 1;
            gis.srv_uuid.setUUID(token);
            token = strsep(&ptr, s);
            gis.srv_type = (BLEServiceType_t)strtol(token, NULL, 10);
            // UUID is expressed as a hex number here, prepended with 0x.
            token = strsep(&ptr, s);
            token = strchr(token, 'x') + 1;
            gis.included_srv_uuid.setUUID(token);
            token = strsep(&ptr, s);
            gis.included_srv_type = (BLEServiceType_t)strtol(token, NULL, 10);
            Serial.println("Adding included service to list !");
            includedGattServices.push_back(gis);
        }
    }
    return result;
}

/*
 * BLE Manager
 */
BLEManager::BLEManager(void) {
}

void BLEManager::registerBleScanResultCallback(void (*callback)(BLEScanResult &bleScanResult)) {
    bleScanResultCallback = callback;
}

void BLEManager::registerBleScanDoneCallback(void (*callback)(void)) {
    bleScanDoneCallback = callback;
}

void BLEManager::registerBleConnectionCallback(void (*callback)(int conn_index)) {
    bleConnectionCallback = callback;
}

void BLEManager::registerBleDisconnectCallback(void (*callback)(int conn_index)) {
    bleDisconnectCallback = callback;
}

void BLEManager::registerBleParameterUpdateCallback(void (*callback)(int conn_index)) {
    bleParameterUpdateCallback = callback;
}

void BLEManager::registerBleMtuUpdateCallback(void (*callback)(int conn_index)) {
    bleMtuUpdateCallback = callback;
}

void BLEManager::registerBlePhyUpdateCallback(void (*callback)(int conn_index)) {
    blePhyUpdateCallback = callback;
}

void registerBleGATTServiceDiscoveredCallback(void (*callback)(int service_index)) {
    bleGATTServiceDiscoveredCallback = callback;
}

bool BLEManager::begin(int role) {
    EspAtDrv.setUnsolicitedMessageCallback(unsolicitedMessage);
    _role = role;
    bool ok = EspAtDrv.bleInit(role);
    return ok;
}

int BLEManager::role() {
    return _role;
}

EspAtDrvError BLEManager::error() {
    return EspAtDrv.getLastErrorCode();
}

bool BLEManager::setPublicBdAddr(bd_addr_t addr, BD_ADDR_TYPE addr_type) {
    BD_ADDR t_addr(addr);
    return EspAtDrv.setPublicBdAddr(t_addr.getAddressString());
}

bool BLEManager::setPublicBdAddr(BD_ADDR addr) {
    return EspAtDrv.setPublicBdAddr(addr.getAddressString(), addr.getAddressType());
}

char * BLEManager::getPublicBdAddr() {
    return EspAtDrv.getPublicBdAddr() + 1;  // +1 to skip the first " character.
}

bool BLEManager::setDeviceName(const char *name) {
    if (strlen(name) > 32)
        return false;
    strncpy(deviceName, name, 32);
    return EspAtDrv.setName(name);
}

char * BLEManager::getDeviceName() {
    char *name = EspAtDrv.getName();
    strncpy(deviceName, name, 32);
    return name;
}

bool BLEManager::startScan(const BLEScan &scan, uint32_t duration, ble_scan_filter_t filter, const char *filter_param) {
    char scan_string[128];
    bool res;

    snprintf(scan_string, 128, "%d,%d,%d,%d,%d", scan.scan_type, scan.own_addr_type, scan.filter_policy,
        scan.scan_interval, scan.scan_window);
    res = EspAtDrv.setScanParams(scan_string);
    _isScanning = true;

    // Override filter ?
    if (filter != BLE_SCAN_NO_FILTER) {
        snprintf(scan_string, 128, "1,%d,%d,\"%s\"", duration, filter == 1 ? 1 : 2, filter_param);
        return EspAtDrv.startScan(scan_string);
    }

    if (scan.scan_filter != BLE_SCAN_NO_FILTER) {
        snprintf(scan_string, 128, "1,%d,%d,\"%s\"", duration, scan.scan_filter == 1 ? 1 : 2, scan.filter_param);
        return EspAtDrv.startScan(scan_string);
    }

    snprintf(scan_string, 128, "1,%d", duration);
    return EspAtDrv.startScan(scan_string);
}

bool BLEManager::stopScan() {
    char stop_scan[32];
    snprintf(stop_scan, 32, "0");
    return EspAtDrv.startScan(stop_scan);
}

bool BLEManager::isScanning() {
    return _isScanning;
}

int BLEAdvertisement::setAdvDataString(const char *string) {
    size_t len = strlen(string);
    int ix = 0;
    if (len && !(len & 1)) {
        for (int i=0;i<len;i+=2) {
            adv_data[ix] = hex2int(string[i]) << 4;
            adv_data[ix++] |= hex2int(string[i+1]);
        }
        adv_data_len = ix;
    } else {
        adv_data_len = 0;
    }
    return ix;
}

int BLEAdvertisement::setMfgDataString(const char *string) {
    // For now we are using adv_data as a buffer for the manufacturer data
    // This may change in the future.
    return setAdvDataString(string);
}

char *BLEAdvertisement::getAdvDataString(char *string, int len) {
    char *ptr = string;
    memset(string, 0, len);
    if (adv_data_len) {
        for (int i=0;i<adv_data_len;i++) {
            ptr += snprintf(ptr, 3, "%02x", adv_data[i]);
        }
    }
    return string;
}

/**
 * Advertisement parameters need to come in the following order
 * <adv_int_min>,<adv_int_max>,<adv_type>,<own_addr_type>,<channel_map>
 * [,<adv_filter_policy>][,<peer_addr_type>,<peer_addr>] - optional
 * [,<primary_phy>,<secondary_phy>] - optional
 */
bool BLEManager::startAdvertising(BLEAdvertisement &adv) {
    bool res;

    snprintf(lBuf, BLEManager::BUFLEN, "%d,%d,%d,%d,%d", adv.adv_int_min, adv.adv_int_max, adv.adv_type,
        adv.own_addr_type, adv.channel_map);

    res = EspAtDrv.setAdvertisementParams(lBuf);
    if (!res) goto exit;

    if (adv.adv_data_len) {
        res = EspAtDrv.setAdvData((const char *)adv.getAdvDataString(lBuf, BLEManager::BUFLEN));
        if (!res) goto exit;
    }

    res = EspAtDrv.startAdvertising();
exit:
    return res;
}

bool BLEManager::startAdvertising(BLEAdvertisement &adv, const char *dev_name, UUID *uuid, bool include_power)  {
    bool res;

    if (!dev_name || !uuid) {
        return false;
    }

    snprintf(lBuf, BLEManager::BUFLEN, "%d,%d,%d,%d,%d", adv.adv_int_min, adv.adv_int_max, adv.adv_type,
        adv.own_addr_type, adv.channel_map);

    res = EspAtDrv.setAdvertisementParams(lBuf);
    if (!res) goto exit;

    if (dev_name != NULL) {
        char mfg[241];

        snprintf(lBuf, BLEManager::BUFLEN, "\"%s\",\"%s\",\"%s\",%d", dev_name, uuid->getUuidStringLE(), 
            adv.getAdvDataString(mfg, 241), include_power);

        res = EspAtDrv.startAdvertisingEx(lBuf);
        if (!res) goto exit;
        
        res = EspAtDrv.startAdvertising();
    } else {
        res = EspAtDrv.setAdvData((const char *)adv.getAdvDataString(lBuf, BLEManager::BUFLEN));
        if (res) goto exit;
        res = EspAtDrv.startAdvertising();
    }
exit:
    return res;
}

void big_endian_store_16(uint8_t * buffer, uint16_t position, uint16_t value){
    uint16_t pos = position;
    buffer[pos++] = (uint8_t)(value >> 8);
    buffer[pos++] = (uint8_t)(value);
}

void reverse_bytes(const uint8_t * src, uint8_t * dest, int len){
    int i;
    for (i = 0; i < len; i++)
        dest[len - 1 - i] = src[i];
}

void BLEManager::iBeaconConfigure(BLEAdvertisement &adv, UUID *uuid, uint16_t major_id, uint16_t minor_id, uint8_t measured_power) {
    memcpy(adv.adv_data, iBeaconAdvertisement01,  sizeof(iBeaconAdvertisement01));
    memcpy(&adv.adv_data[3], iBeaconAdvertisement38, sizeof(iBeaconAdvertisement38));
    memcpy(&adv.adv_data[9], uuid->getUuid(), 16);
    big_endian_store_16(adv.adv_data, 25, major_id);
    big_endian_store_16(adv.adv_data, 27, minor_id);
    adv.adv_data[29] = measured_power;
    adv.adv_data_len = 30;
}


bool BLEManager::stopAdvertising() {
    return EspAtDrv.stopAdvertising();
}

void BLEManager::process() {
    EspAtDrv.process();
}

/**
 * 
 * Connection related functions
 * 
*/
bool BLEManager::bleConnect(int index, BD_ADDR address, BLEConnection *conn) {
    if (!bleConnections[index]) {
        bleConnections[index] = new BLEConnection();
    }
    snprintf(lBuf, BLEManager::BUFLEN, "%d,\"%s\"", index, address.getAddressString());
    return EspAtDrv.bleConnect(lBuf);
}

BLEConnection *BLEManager::getBleConnection(int index) {
    return bleConnections[index];
}

bool BLEManager::updateConnection(int conn_index) {
    BLEConnection *conn = getBleConnection(conn_index);
    snprintf(lBuf, BLEManager::BUFLEN, "%d,%d,%d,%d,%d", 
        conn_index, conn->min_interval, conn->max_interval, conn->latency, conn->timeout);
    return EspAtDrv.updateConnParams(lBuf);
}

bool BLEManager::mtu(int conn_index, int mtu_size) {
    BLEConnection *conn = getBleConnection(conn_index);
    conn->mtu_size = mtu_size;
    snprintf(lBuf, BLEManager::BUFLEN, "%d,%d", 
        conn_index, conn->mtu_size);
    return EspAtDrv.updateMtuSize(lBuf);
}

int BLEManager::mtu() {
    // Get the mtu size from the esp device.
    char *resp = EspAtDrv.getMtuSize();
    Serial.printf("Returned '%s'\r\n", resp);
    // Parse out connection index and mtu size.
    const char s[2] = ",";
    char *token;
    token = strsep(&resp, s);
    // Connection index
    int index = strtol(token, NULL, 10);
    token = strsep(&resp, s);
    int mtu_size = strtol(token, NULL, 10);
    Serial.printf("index=%d, mtu=%d\r\n", index, mtu_size);

    if (index >= 0 && index < MAX_BLE_CONNECTIONS) {
        BLEConnection *conn = getBleConnection(index);
        conn->mtu_size = mtu_size;
        return mtu_size;
    }
    return -1;
}

bool BLEManager::discoverGATTServices(int conn_index) {
    bool result = false;

    if (_role == BLE_ROLE_CLIENT) {
        snprintf(lBuf, BLEManager::BUFLEN, "%d", conn_index);
        result = EspAtDrv.discoverCGATTServices(lBuf);
    } else if (_role == BLE_ROLE_SERVER) {
        Serial.println("Server gatt discovery !");
        result = false;
    }
    return result;
}

bool BLEManager::discoverCharacteristicsForService(int conn_index, int srv_index) {
    bool result = false;

    if (_role == BLE_ROLE_CLIENT) {
        snprintf(lBuf, BLEManager::BUFLEN, "%d,%d", conn_index, srv_index);
        result = EspAtDrv.discoverCGATTServicesCharacteristics(lBuf);
    } else if (_role == BLE_ROLE_SERVER) {
        Serial.println("Server gatt discovery !");
        result = false;
    }
    return result;
}

bool BLEManager::discoverIncludedServices(int conn_index, int srv_index) {
    bool result = false;

    if (_role == BLE_ROLE_CLIENT) {
        snprintf(lBuf, BLEManager::BUFLEN, "%d,%d", conn_index, srv_index);
        result = EspAtDrv.discoverCGATTIncludedServices(lBuf);
    } else if (_role == BLE_ROLE_SERVER) {
        Serial.println("Server gatt discovery !");
        result = false;
    }
    return result;
}

/**
  * UUID class
  * 
  * UUID numbers are stored natural numbers (Big Endian) in the 128-bit array.
  * When sending UUID's to the ESP-AT interpreter they need to be LE reversed and if
  * it is a 128-bit number it can not have any dashes in the string.
  */
static int nibble_for_char(const char c){
    if ('0' <= c && c <= '9') return c - '0';
    if ('a' <= c && c <= 'f') return c - 'a' + 10;
    if ('A' <= c && c <= 'F') return c - 'A' + 10;
    return 0;
}

static char uuid128_to_str_buffer[32+4+1];
char * uuid128_to_str(uint8_t * uuid){
    sprintf(uuid128_to_str_buffer, "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
           uuid[0], uuid[1], uuid[2], uuid[3], uuid[4], uuid[5], uuid[6], uuid[7],
           uuid[8], uuid[9], uuid[10], uuid[11], uuid[12], uuid[13], uuid[14], uuid[15]);
    return uuid128_to_str_buffer;
}

char * uuid128_to_str_LE(uint8_t * uuid){
    sprintf(uuid128_to_str_buffer, "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
           uuid[15], uuid[14], uuid[13], uuid[12], uuid[11], uuid[10], uuid[9], uuid[8],
           uuid[7], uuid[6], uuid[5], uuid[4], uuid[3], uuid[2], uuid[1], uuid[0]);
    return uuid128_to_str_buffer;
}

UUID::UUID(void){
    memset(uuid, 0, 16);
}

UUID::UUID(const uint8_t uuid[16]){
    memcpy(this->uuid, uuid, 16);
}

UUID::UUID(const char * uuidStr){
    setUUID(uuidStr);
}

void UUID::setUUID(const char *uuidStr) {
    memset(uuid, 0, 16);
    int len = strlen(uuidStr);
    if (len <= 4){
        // Handle 4 Bytes HEX
        int result = strtol((const char *)uuidStr, NULL, 16);
        if (result >= 0 && result <= 65535) {
            // Normalize UUID to bluetooth base uuid
            memcpy(uuid, sdp_bluetooth_base_uuid, 16);
            uuid[2] = (result >> 8) & 0xff;
            uuid[3] = result & 0xff;
        }
        return;
    }
    
    // quick UUID parser, ignoring dashes
    // Parse and store Big Endian.
    int i = 0;
    uint8_t data = 0;
    bool have_nibble = false;
    while(*uuidStr && i < 16) {
        const char c = *uuidStr++;
        if (c == '-') continue;
        data = data << 4 | nibble_for_char(c);
        if (!have_nibble) {
            have_nibble = true;
            continue;
        }
        uuid[i++] = data;
        data = 0;
        have_nibble = false;
    }
    if (have_nibble)
        uuid[i] = data << 4;
}

const uint8_t * UUID::getUuid(void) const {
    return uuid;
}

static char uuid16_buffer[5];
const char * UUID::getUuidString() const {
    if (memcmp(&uuid[4], &sdp_bluetooth_base_uuid[4], 12) == 0) {
        sprintf(uuid16_buffer, "%04x", (uint16_t)READ_NET_32(uuid, 0));
        return uuid16_buffer;
    } else {
        // TODO: fix uuid128_to_str
        return uuid128_to_str((uint8_t*)uuid);
    }
}

const char * UUID::getUuidStringLE() const {
    if (memcmp(&uuid[4], &sdp_bluetooth_base_uuid[4], 12) == 0) {
        sprintf(uuid16_buffer, "%04x", (uint16_t)*(uint16_t *)&uuid[2]);
        return uuid16_buffer;
    } else {
        // TODO: fix uuid128_to_str
        return uuid128_to_str_LE((uint8_t*)uuid);
    }
}

const char * UUID::getUuid128String() const {
    return uuid128_to_str((uint8_t*)uuid);
}

bool UUID::matches(UUID *other)        const {
    return memcmp(this->uuid, other->uuid, 16) == 0;
}

/*
 * BD_ADDR
 */ 
BD_ADDR::BD_ADDR(void) {
}

BD_ADDR::BD_ADDR(const char * address_string, BD_ADDR_TYPE address_type) : address_type(address_type) {
    int processed = sscanf(address_string, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx", address, address + 1,
                           address + 2, address + 3, address + 4, address + 5);
    if (processed != 6) { // Set address to zeroes if we did not get six bytes back.
        for (int i = 0; i < 6; i++) {
            address[i] = 0;
        }
    }
}

void BD_ADDR::setAddress(const char * address_string, BD_ADDR_TYPE address_type) {
    address_type = address_type;
    int processed = sscanf(address_string, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx", address, address + 1,
                           address + 2, address + 3, address + 4, address + 5);
    if (processed != 6) { // Set address to zeroes if we did not get six bytes back.
        for (int i = 0; i < 6; i++) {
            address[i] = 0;
        }
    }
}

void BD_ADDR::setAddressType(BD_ADDR_TYPE type) {
    address_type = type;
}

BD_ADDR::BD_ADDR(const uint8_t address[6], BD_ADDR_TYPE address_type) : address_type(address_type) {
    memcpy(this->address, address, 6);
}

const uint8_t * BD_ADDR::getAddress(void) {
    return address;
}

const char * BD_ADDR::getAddressString(void) {
    snprintf(address_string, 18, "%02x:%02x:%02x:%02x:%02x:%02x", address[0], address[1],
                           address[2], address[3], address[4], address[5]);
    return address_string;
}

BD_ADDR_TYPE BD_ADDR::getAddressType(void) {
    return address_type;
}

bool BD_ADDR::matches(BD_ADDR *other) {
    return memcmp(this->address, other->address, 6) == 0;
}

BLEManager BLE;