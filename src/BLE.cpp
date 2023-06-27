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
const uint8_t sdp_bluetooth_base_uuid[] = { 0x00, 0x00, 0x00, 0x00, /* - */ 0x00, 0x00, /* - */ 0x10, 0x00, /* - */
    0x80, 0x00, /* - */ 0x00, 0x80, 0x5F, 0x9B, 0x34, 0xFB };

/*
 * BLE Manager
 */
BLEManager::BLEManager(void) {
}

bool BLEManager::begin(int role) {
    bool ok = EspAtDrv.bleInit(role);
    return ok;
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

/// UUID class
UUID::UUID(void){
    memset(uuid, 0, 16);
}

UUID::UUID(const uint8_t uuid[16]){
    memcpy(this->uuid, uuid, 16);
}

UUID::UUID(const char * uuidStr){
    memset(uuid, 0, 16);
    int len = strlen(uuidStr);
    Serial.printf("Input length %d\r\n", len);
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
        Serial.println(data, HEX);
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
        sprintf(uuid16_buffer, "%04x", (uint16_t) READ_NET_32(uuid, 0));
        return uuid16_buffer;
    }  else {
        // TODO: fix uuid128_to_str
        return uuid128_to_str((uint8_t*)uuid);
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

BLEManager BLE;