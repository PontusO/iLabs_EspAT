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
#ifndef __ILABS_BLE_H
#define __ILABS_BLE_H

#include <utility/EspAtDrvTypes.h>
#include <list>

#if defined __cplusplus
extern "C" {
#endif

#include <stdint.h>

#define LE_ADVERTISING_DATA_SIZE    31
#define BD_ADDR_LEN                 6

#define READ_NET_32(buffer, pos)    (((uint32_t) buffer[(pos)+3]) | (((uint32_t)buffer[(pos)+2]) << 8) | (((uint32_t)buffer[(pos)+1]) << 16) | (((uint32_t) buffer[pos])) << 24)

typedef uint8_t bd_addr_t[BD_ADDR_LEN];

/**
 * This is the maximum number of simultanous connections that the default
 * ESP-AT stack supports.
 */
#define MAX_BLE_CONNECTIONS         3

typedef enum BLERole {
    BLE_ROLE_DEINIT = 0,
    BLE_ROLE_CLIENT = 1,
    BLE_ROLE_SERVER = 2        
} BLERole;

typedef enum BD_ADDR_TYPE {
    PUBLIC_ADDRESS = 0,
    PRIVAT_ADDRESS = 1,
    RPA_PUBLIC_ADDRESS = 2,
    RPA_PRIVAT_ADDRESS = 3,
} BD_ADDR_TYPE;

class BD_ADDR {
private:
    uint8_t address[6];
    char address_string[19];
    BD_ADDR_TYPE address_type;
public:
    BD_ADDR();
    BD_ADDR(const char * address_string, BD_ADDR_TYPE address_type = PUBLIC_ADDRESS);
    BD_ADDR(const uint8_t address[6], BD_ADDR_TYPE address_type = PUBLIC_ADDRESS);
    void setAddress(const char * address_string, BD_ADDR_TYPE address_type = PUBLIC_ADDRESS);
    void setAddressType(BD_ADDR_TYPE type);
    const uint8_t * getAddress();
    const char * getAddressString();
    BD_ADDR_TYPE getAddressType();
    bool matches(BD_ADDR *other);
};

class UUID {
private:
    uint8_t uuid[16];
public:
    UUID();
    UUID(const uint8_t uuid[16]);
    UUID(const char * uuidStr);

    void setUUID(const char *uuidStr);
    const char * getUuidString()    const;
    const char * getUuidStringLE()  const;
    const char * getUuid128String() const;
    const uint8_t * getUuid(void)   const;
    bool matches(UUID *uuid)        const;
};

typedef enum BLEAdvertisementType {
    ADV_TYPE_IND = 0,
    ADV_TYPE_DIRECT_IND_HIGH = 1,
    ADV_TYPE_SCAN_IND = 2,
    ADV_TYPE_NONCONN_IND = 3,
    ADV_TYPE_DIRECT_IND_LOW = 4,
    ADV_TYPE_EXT_NOSCANNABLE_IND = 5,
    ADV_TYPE_EXT_CONNECTABLE_IND = 6,
    ADV_TYPE_EXT_SCANNABLE_IND = 7
} ble_advertisement_type_t;

typedef enum BLEAdvertisementChannel {
    ADV_CHNL_37 = 1,
    ADV_CHNL_38 = 2,
    ADV_CHNL_39 = 4,
    ADV_CHNL_ALL = 7
} ble_advertisement_channel_t;

typedef enum BLEAdvertisementFilterPolicy {
    ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY = 0,
    ADV_FILTER_ALLOW_SCAN_WLST_CON_ANY = 1,
    ADV_FILTER_ALLOW_SCAN_ANY_CON_WLST = 2,
    ADV_FILTER_ALLOW_SCAN_WLST_CON_WLST = 3
} ble_advertisement_filter_polict_t;

typedef enum BLEAdvertisementPHY {
    BLE_PHY_1M = 1, 
    BLE_PHY_2M = 2,
    BLE_PHY_CODED = 3
} ble_advertisement_phy_t;

class BLEAdvertisement {
    friend class BLEManager;
private:
    uint8_t adv_data[119];
    uint8_t mfg_data[119];
    size_t adv_data_len = 0;
    uint16_t adv_int_min;
    uint16_t adv_int_max;
    ble_advertisement_type_t adv_type;
    BD_ADDR_TYPE own_addr_type;
    ble_advertisement_channel_t channel_map;
    ble_advertisement_filter_polict_t adv_filter_policy;
    BD_ADDR_TYPE peer_addr_type;
    BD_ADDR peer_addr;
    ble_advertisement_phy_t primary_phy;
    ble_advertisement_phy_t secondary_phy;
    char dev_name[32];
    UUID uuid;
    bool include_power = false;
    uint8_t hex2int(char ch)
    {
        if (ch >= '0' && ch <= '9')
            return ch - '0';
        if (ch >= 'A' && ch <= 'F')
            return ch - 'A' + 10;
        if (ch >= 'a' && ch <= 'f')
            return ch - 'a' + 10;
        return 0xff;
    }
public:
    BLEAdvertisement(uint16_t adv_int_min = 100, uint16_t adv_int_max = 100, ble_advertisement_type_t adv_type = ADV_TYPE_IND,
                     BD_ADDR_TYPE own_addr_type = PUBLIC_ADDRESS, ble_advertisement_channel_t channel_map = ADV_CHNL_ALL,
                     ble_advertisement_filter_polict_t adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
                     BD_ADDR_TYPE peer_addr_type = PUBLIC_ADDRESS, BD_ADDR peer_addr = BD_ADDR(),
                     ble_advertisement_phy_t primary_phy = BLE_PHY_1M, ble_advertisement_phy_t secondary_phy = BLE_PHY_2M) :
                     adv_int_min(adv_int_min), adv_int_max(adv_int_max), adv_type(adv_type),
                     own_addr_type(own_addr_type), channel_map(channel_map),
                     adv_filter_policy(adv_filter_policy),
                     peer_addr_type(peer_addr_type), peer_addr(peer_addr),
                     primary_phy(primary_phy), secondary_phy(secondary_phy) { }
    BLEAdvertisement(const char *adv_data_str,
                     uint16_t adv_int_min = 100, uint16_t adv_int_max = 100, ble_advertisement_type_t adv_type = ADV_TYPE_IND,
                     BD_ADDR_TYPE own_addr_type = PUBLIC_ADDRESS, ble_advertisement_channel_t channel_map = ADV_CHNL_ALL,
                     ble_advertisement_filter_polict_t adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
                     BD_ADDR_TYPE peer_addr_type = PUBLIC_ADDRESS, BD_ADDR peer_addr = BD_ADDR(),
                     ble_advertisement_phy_t primary_phy = BLE_PHY_1M, ble_advertisement_phy_t secondary_phy = BLE_PHY_2M) :
                     adv_int_min(adv_int_min), adv_int_max(adv_int_max), adv_type(adv_type),
                     own_addr_type(own_addr_type), channel_map(channel_map),
                     adv_filter_policy(adv_filter_policy),
                     peer_addr_type(peer_addr_type), peer_addr(peer_addr),
                     primary_phy(primary_phy), secondary_phy(secondary_phy) { 
                        setAdvDataString(adv_data_str);
                     }
    BLEAdvertisement(uint8_t adv_data[119], size_t len,
                     uint16_t adv_int_min = 100, uint16_t adv_int_max = 100, ble_advertisement_type_t adv_type = ADV_TYPE_IND,
                     BD_ADDR_TYPE own_addr_type = PUBLIC_ADDRESS, ble_advertisement_channel_t channel_map = ADV_CHNL_ALL,
                     ble_advertisement_filter_polict_t adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
                     BD_ADDR_TYPE peer_addr_type = PUBLIC_ADDRESS, BD_ADDR peer_addr = BD_ADDR(),
                     ble_advertisement_phy_t primary_phy = BLE_PHY_1M, ble_advertisement_phy_t secondary_phy = BLE_PHY_2M) :
                     adv_int_min(adv_int_min), adv_int_max(adv_int_max), adv_type(adv_type),
                     own_addr_type(own_addr_type), channel_map(channel_map),
                     adv_filter_policy(adv_filter_policy),
                     peer_addr_type(peer_addr_type), peer_addr(peer_addr),
                     primary_phy(primary_phy), secondary_phy(secondary_phy) { 
                        if (len < 119) {
                            adv_data_len = len;
                            memcpy(adv_data, adv_data, 119);
                        }
                     }

    ~BLEAdvertisement() {  }

    char *getAdvDataString(char *string, int len);
    int setAdvDataString(const char *string);
    int setMfgDataString(const char *string);
};

typedef enum BLEScanFilterPolicy {
    BLE_SCAN_FILTER_ALLOW_ALL = 0,
    BLE_SCAN_FILTER_ALLOW_ONLY_WLST = 1,
    BLE_SCAN_FILTER_ALLOW_UND_RPA_DIR = 2,
    BLE_SCAN_FILTER_ALLOW_WLIST_PRA_DIR = 3
} ble_scan_filter_policy_t;

typedef enum BLEScanFilter {
    BLE_SCAN_NO_FILTER = 0,
    BLE_SCAN_MAC_FILTER = 1,
    BLE_SCAN_NAME_FILTER = 2
} ble_scan_filter_t;

class BLEScan {
public:
    uint8_t scan_type;
    uint8_t own_addr_type;
    uint8_t filter_policy;
    uint8_t scan_interval;
    uint8_t scan_window;
    ble_scan_filter_t scan_filter;
    char filter_param[64];

    BLEScan(uint8_t scan_type = 0, uint8_t own_addr_type = 0, uint8_t filter_policy = 0, uint8_t scan_interval = 100, uint8_t scan_window = 50) :
        scan_type(scan_type), own_addr_type(own_addr_type), filter_policy(filter_policy), scan_interval(scan_interval), scan_window(scan_window) { }

    void setFilter(ble_scan_filter_t filter, const char *param) {
        scan_filter = filter;
        strncpy(filter_param, param, 64);
    }
};

class BLEScanResult {
public:
    BD_ADDR bd_addr;
    int rssi;
    char adv_data[32];
    char scan_rsp_data[32];
};

class BLEConnection {
public:
    int conn_index;
    BD_ADDR remote_address;

    int min_interval;
    int max_interval;
    int cur_interval;
    int latency;
    int timeout;

    int mtu_size;

    ble_advertisement_phy_t tx_PHY;
    ble_advertisement_phy_t rx_PHY;
};

typedef enum BLEServiceType {
    NOT_PRIMARY_SERVICE = 0,
    PRIMARY_SERVICE = 1
} BLEServiceType_t;

class BLEPrimaryGattService {
public:
    int conn_index;
    int srv_index;
    UUID srv_uuid;
    BLEServiceType_t srv_type;
};

typedef enum BLEGattCharType_e {
    BLE_GATT_CHARACTERISTIC = 0,
    BLE_GATT_DESCRIPTOR,
} BLEGattCharType_e;

class BLEGattCharacteristics {
public:
    BLEGattCharType_e gatt_char_type;
    int conn_index;
    int srv_index;
    int char_index;
    UUID char_uuid;
    int char_prop;
    int desc_index;
    UUID desc_uuid;
    BLEServiceType_t srv_type;
};

class BLEGattIncludedService {
public:
    int conn_index;
    int srv_index;
    UUID srv_uuid;
    BLEServiceType_t srv_type;
    UUID included_srv_uuid;
    BLEServiceType_t included_srv_type;
};

class BLEManager {
private:
    char deviceName[32];
    constexpr static const int BUFLEN = 512;
    char lBuf[BLEManager::BUFLEN];

    int nuConnections = 0;
    int _role = BLE_ROLE_DEINIT;
public:
    BLEManager();

    static bool unsolicitedMessage(char *buffer);

    void registerBleScanResultCallback(void (*callback)(BLEScanResult &bleScanResult));
    void registerBleScanDoneCallback(void (*callback)(void));
    void registerBleConnectionCallback(void (*callback)(int conn_index));
    void registerBleDisconnectCallback(void (*callback)(int conn_index));
    void registerBleParameterUpdateCallback(void (*callback)(int conn_index));
    void registerBleMtuUpdateCallback(void (*callback)(int conn_index));
    void registerBlePhyUpdateCallback(void (*callback)(int conn_index));

    void registerBleGATTServiceDiscoveredCallback(void (*callback)(int service_index));

    bool begin(int role);
    int role();

    // Adress
    bool setPublicBdAddr(bd_addr_t addr, BD_ADDR_TYPE addr_type = PRIVAT_ADDRESS);
    bool setPublicBdAddr(BD_ADDR addr);
    char *getPublicBdAddr();

    bool setDeviceName(const char *name);
    char *getDeviceName();

    bool startScan(const BLEScan &scan, uint32_t duration = 10, ble_scan_filter_t filter = BLE_SCAN_NO_FILTER, const char *filter_param = NULL);
    bool stopScan();
    bool isScanning();

    void iBeaconConfigure(BLEAdvertisement &adv, UUID *uuid, uint16_t major_id, uint16_t minor_id, uint8_t measured_power = 0xc3);
    bool startAdvertising(BLEAdvertisement &scan);
    bool startAdvertising(BLEAdvertisement &scan, const char *dev_name, UUID *uuid, bool include_power = false);
    bool stopAdvertising();

    bool bleConnect(int index, BD_ADDR address, BLEConnection *conn = NULL);
    bool updateConnection(int conn_index);
    static BLEConnection *getBleConnection(int index);
    bool mtu(int conn_index, int mtu_size);
    int mtu();

    bool discoverGATTServices(int conn_index);
    bool discoverCharacteristicsForService(int conn_index, int srv_index);
    bool discoverIncludedServices(int conn_index, int srv_index);

    void process();
    
    EspAtDrvError error();
};

extern BLEManager BLE;

#if defined __cplusplus
}
#endif

#endif // __ILABS_BLE_H