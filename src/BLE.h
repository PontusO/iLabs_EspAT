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

#if defined __cplusplus
extern "C" {
#endif

#include <stdint.h>

#define LE_ADVERTISING_DATA_SIZE    31
#define BD_ADDR_LEN                 6

#define READ_NET_32(buffer, pos)    (((uint32_t) buffer[(pos)+3]) | (((uint32_t)buffer[(pos)+2]) << 8) | (((uint32_t)buffer[(pos)+1]) << 16) | (((uint32_t) buffer[pos])) << 24)

typedef uint8_t bd_addr_t[BD_ADDR_LEN];

typedef enum BLERole {
    BLE_ROLE_DEINIT = 0,
    BLE_ROLE_CLIENT = 1,
    BLE_ROLE_SERVER = 2        
} BLERole;

typedef enum BD_ADDR_TYPE {
    PUBLIC_ADDRESS = 0,
    PRIVAT_ADDRESS
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
    const uint8_t * getAddress();
    const char * getAddressString();
    BD_ADDR_TYPE getAddressType();
};

class UUID {
private:
    uint8_t uuid[16];
public:
    UUID();
    UUID(const uint8_t uuid[16]);
    UUID(const char * uuidStr);
    const char * getUuidString()    const;
    const char * getUuid128String() const;
    const uint8_t * getUuid(void)   const;
    bool matches(UUID *uuid)        const;
};

class BLEAdvertisement {
private:
    uint8_t advertising_event_type;
    uint8_t rssi;
    uint8_t data_length;
    uint8_t data[10 + LE_ADVERTISING_DATA_SIZE];
    BD_ADDR  bd_addr;
    UUID * iBeacon_UUID;
public:
    BLEAdvertisement(uint8_t * event_packet);
    ~BLEAdvertisement();
    BD_ADDR * getBdAddr();
    BD_ADDR_TYPE getBdAddrType();
    int getRssi();
    bool containsService(UUID * service);
    bool nameHasPrefix(const char * namePrefix);
    const uint8_t * getAdvData();
    bool isIBeacon();
    const UUID * getIBeaconUUID();
    uint16_t     getIBeaconMajorID();
    uint16_t     getIBecaonMinorID();
    uint8_t      getiBeaconMeasuredPower();
};

class BLEManager {
public:
    BLEManager();

    bool begin(int role);

    // Adress
    bool setPublicBdAddr(bd_addr_t addr, BD_ADDR_TYPE addr_type = PRIVAT_ADDRESS);
    bool setPublicBdAddr(BD_ADDR addr);
    char *getPublicBdAddr();
    
    void setAdvData(uint16_t size, const uint8_t * data);

    EspAtDrvError error();
};

extern BLEManager BLE;

#if defined __cplusplus
}
#endif

#endif // __ILABS_BLE_H