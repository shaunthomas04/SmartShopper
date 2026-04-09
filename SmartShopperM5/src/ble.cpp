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
        if (!shoppingList.empty()) {
            bleNotifySingleItem(selectedIndex); 
        }
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
        BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY
    );

    bleCharacteristic->addDescriptor(new BLE2902());
    
    // Set initial value to empty object instead of array
    bleCharacteristic->setValue("{}"); 

    bleService->start();

    BLEAdvertising *adv = BLEDevice::getAdvertising();
    adv->addServiceUUID(SHOPPER_SERVICE_UUID);
    adv->setScanResponse(true);
    BLEDevice::startAdvertising();

    Serial.printf("[BLE] Advertising started as %s\n", USER_ID.c_str());

    // NEW: Notify the first item immediately if available
    if (!shoppingList.empty()) {
        bleNotifySingleItem(selectedIndex); 
    }
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

    Serial.printf("[JSON OUT] %s\n", payload.c_str());
    bleCharacteristic->setValue(payload.c_str());

    if (bleClientConnected) {
        bleCharacteristic->notify();
        Serial.printf("[BLE] Notified client: %s\n", payload.c_str());
    } else {
        Serial.println("[BLE] Value updated (no client connected)");
    }
}

void bleStop() {
    if (!bleServer) return;

    BLEDevice::stopAdvertising();

    if (bleService) {
        bleService->stop();
    }

    bleServer       = nullptr;
    bleService      = nullptr;
    bleCharacteristic = nullptr;
    bleClientConnected = false;

    BLEDevice::deinit(true);
    Serial.println("[BLE] Stopped and deinitialized");
}

void bleNotifySingleItem(int index) {
    if (!bleCharacteristic || shoppingList.empty()) return;
    
    // Safety check for index
    if (index < 0 || index >= (int)shoppingList.size()) return;

    const auto& s = shoppingList[index];

    // Create JSON for a SINGLE object instead of an array
    JsonDocument doc;
    doc["name"]     = s.name;
    doc["price"]    = s.price;
    doc["cost"]     = s.cost;
    doc["rating"]   = s.rating;
    doc["reviews"]  = s.reviews;
    doc["store"]    = s.store;
    doc["distance"] = s.distance;
    doc["link"]     = s.link;
    doc["image"]    = s.image;
    doc["index"]    = index; // Useful for the client to know which item this is

    String payload;
    serializeJson(doc, payload);

    bleCharacteristic->setValue(payload.c_str());

    if (bleClientConnected) {
        bleCharacteristic->notify();
        Serial.printf("[BLE] Notified single item (%d): %s\n", index, s.name.c_str());
    } else {
        Serial.printf("[BLE] Updated value for item: %s\n", s.name.c_str());
    }
}