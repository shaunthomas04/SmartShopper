#include "ble.h"
#include "globals.h"
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLE2902.h>
#include <ArduinoJson.h>

// ----------- UUIDs -----------
#define SHOPPER_SERVICE_UUID        "12345678-1234-1234-1234-1234567890ab"
#define SHOPPER_CHARACTERISTIC_UUID "abcdef01-1234-1234-1234-1234567890ab"

// ----------- BLE broadcast name -----------
// Broadcast name = user ID so clients find this device by scanning for it

// ----------- BLE objects -----------
static BLEServer         *bleServer         = nullptr;
static BLEService        *bleService        = nullptr;
static BLECharacteristic *bleCharacteristic = nullptr;

static bool bleClientConnected = false;

// ----------- Server callbacks -----------
class ShopperServerCallbacks : public BLEServerCallbacks {
    void onConnect(BLEServer *pServer) override {
        bleClientConnected = true;
        Serial.println("[BLE] Client connected");
        // Push current list immediately on connect
        bleNotifyShoppingList();
    }
    void onDisconnect(BLEServer *pServer) override {
        bleClientConnected = false;
        Serial.println("[BLE] Client disconnected — restarting advertising");
        BLEDevice::startAdvertising();
    }
};

// ----------- Setup -----------
void bleSetup() {
    BLEDevice::init(USER_ID.c_str());

    bleServer = BLEDevice::createServer();
    bleServer->setCallbacks(new ShopperServerCallbacks());

    bleService = bleServer->createService(SHOPPER_SERVICE_UUID);

    bleCharacteristic = bleService->createCharacteristic(
        SHOPPER_CHARACTERISTIC_UUID,
        BLECharacteristic::PROPERTY_READ   |
        BLECharacteristic::PROPERTY_NOTIFY
    );

    bleCharacteristic->addDescriptor(new BLE2902());
    bleCharacteristic->setValue("[]");  // empty list on start

    bleService->start();

    BLEAdvertising *adv = BLEDevice::getAdvertising();
    adv->addServiceUUID(SHOPPER_SERVICE_UUID);
    adv->setScanResponse(true);
    adv->setMinPreferred(0x12);
    BLEDevice::startAdvertising();

    Serial.printf("[BLE] Advertising started as %s\n", USER_ID.c_str());
}

// ----------- Serialize shoppingList → JSON and notify -----------
// Payload format (array of objects):
// [
//   {"name":"...","price":"...","cost":0.0,"rating":0.0,"reviews":0,"store":"...","distance":"..."},
//   ...
// ]
void bleNotifyShoppingList() {
    if (!bleCharacteristic) return;

    // Build JSON — one object per shopping list entry
    // Each entry is ~120 chars; 10 items ≈ 1200 bytes, well within BLE MTU after negotiation.
    // For very long lists the payload is truncated at 512 bytes (safe BLE default MTU - overhead).
    JsonDocument doc;
    JsonArray arr = doc.to<JsonArray>();

    for (const auto& s : shoppingList) {
        JsonObject obj = arr.add<JsonObject>();
        obj["name"]     = s.name;
        obj["price"]    = s.price;
        obj["cost"]     = s.cost;
        obj["rating"]   = s.rating;
        obj["reviews"]  = s.reviews;
        obj["store"]    = s.store;
        obj["distance"] = s.distance;
        obj["link"]     = s.link;
        obj["image"]    = s.image;
    }

    String payload;
    serializeJson(doc, payload);

    // BLE characteristic value is limited; truncate if needed
    if (payload.length() > 512) {
        Serial.printf("[BLE] Warning: payload %d bytes, truncating to 512\n", payload.length());
        payload = payload.substring(0, 512);
    }

    bleCharacteristic->setValue(payload.c_str());

    if (bleClientConnected) {
        bleCharacteristic->notify();
        Serial.printf("[BLE] Notified client: %s\n", payload.c_str());
    } else {
        Serial.println("[BLE] Value updated (no client connected)");
    }
}