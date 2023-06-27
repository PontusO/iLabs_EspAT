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