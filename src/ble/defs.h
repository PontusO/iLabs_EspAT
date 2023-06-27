#define LE_ADVERTISING_DATA_SIZE    31
/**
 * @brief Length of a bluetooth device address.
 */
#define BD_ADDR_LEN 6

typedef struct {
    uint16_t start_handle;
    uint16_t value_handle;
    uint16_t end_handle;
    uint16_t properties;
    uint16_t uuid16;
    uint8_t  uuid128[16];
} gatt_client_characteristic_t;

typedef struct {
    uint16_t start_group_handle;
    uint16_t end_group_handle;
    uint16_t uuid16;
    uint8_t  uuid128[16];
} gatt_client_service_t;

typedef struct le_characteristic{
    uint16_t start_handle;
    uint16_t value_handle;
    uint16_t end_handle;
    uint16_t properties;
    uint16_t uuid16;
    uint8_t  uuid128[16];
} le_characteristic_t;

typedef struct le_service{
    uint16_t start_group_handle;
    uint16_t end_group_handle;
    uint16_t uuid16;
    uint8_t  uuid128[16];
} le_service_t;

/**
 * @brief hci connection handle type
 */
typedef uint16_t hci_con_handle_t;

/**
 * @brief Bluetooth address
 */
typedef uint8_t bd_addr_t[BD_ADDR_LEN];

typedef enum BLERole {
    BLE_ROLE_DEINIT = 0,
    BLE_ROLE_CLIENT = 1,
    BLE_ROLE_SERVER = 2        
} BLERole;
