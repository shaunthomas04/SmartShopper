import { Buffer } from "buffer"
import { PermissionsAndroid, Platform } from "react-native"
import { BleManager, Device, Subscription } from "react-native-ble-plx"

const SERVICE_UUID        = "12345678-1234-1234-1234-1234567890ab"
const CHARACTERISTIC_UUID = "abcdef01-1234-1234-1234-1234567890ab"

class BLEManager {

  private manager:      BleManager
  private device:       Device | null = null
  private subscription: Subscription | null = null
  private buffer:       string = ""

  constructor() {
    this.manager = new BleManager()
  }

  async requestPermissions() {
    if (Platform.OS === "android") {
      await PermissionsAndroid.requestMultiple([
        PermissionsAndroid.PERMISSIONS.BLUETOOTH_SCAN,
        PermissionsAndroid.PERMISSIONS.BLUETOOTH_CONNECT,
        PermissionsAndroid.PERMISSIONS.ACCESS_FINE_LOCATION,
      ])
    }
  }

  scanForDevice(deviceName: string, onFound: (device: Device) => void) {
    console.log(`Scanning for "${deviceName}"...`)
    this.manager.startDeviceScan(null, null, (error, device) => {
      if (error) { console.log("Scan error:", error); return }
      if (!device) return
      const nameMatch = device.name === deviceName || device.localName === deviceName
      if (nameMatch) {
        console.log("Found:", device.name)
        this.manager.stopDeviceScan()
        this.device = device
        onFound(device)
      }
    })
  }

  stopScan() {
    this.manager.stopDeviceScan()
  }

  async connect(device: Device) {
    this.device = await device.connect()

    // Request a larger MTU so each BLE packet carries more data.
    // 512 is the BLE spec maximum; the actual negotiated value may be lower
    // depending on the peripheral (M5's stack typically lands at 517 or 512).
    try {
      await this.device.requestMTU(512)
      console.log("MTU negotiated")
    } catch (e) {
      console.log("MTU negotiation failed (non-fatal):", e)
    }

    await this.device.discoverAllServicesAndCharacteristics()
    console.log("Connected and services discovered")
    return this.device
  }

  // ---------------------------------------------------------------------------
  // Subscribe to notifications from the M5.
  //
  // The M5 sends the full shopping-list JSON in 500-byte chunks.  Each BLE
  // notification arrives as one chunk.  We accumulate chunks in `this.buffer`
  // and attempt JSON.parse after every chunk.  The moment it succeeds we know
  // we have a complete, valid payload — we call onMessage and reset the buffer.
  //
  // If a chunk is corrupted or the connection drops mid-transfer the buffer is
  // simply discarded on the next successful parse or on disconnect.
  // ---------------------------------------------------------------------------
  subscribeToNotifications(onMessage: (message: string) => void) {
    if (!this.device) return;

    this.subscription?.remove();
    this.buffer = "";

    this.subscription = this.device.monitorCharacteristicForService(
      SERVICE_UUID,
      CHARACTERISTIC_UUID,
      (error, characteristic) => {
        if (error) { console.log("Notify error:", error); return; }
        if (!characteristic?.value) return;

        const chunk = Buffer.from(characteristic.value, "base64").toString("utf-8");
        console.log("[BLE] Chunk received:", chunk);

        // Only reset buffer if this chunk starts a new JSON object
        // AND the current buffer is either empty or already complete
        if (chunk.startsWith("{") && (this.buffer === "" || this.isCompleteJson(this.buffer))) {
          this.buffer = chunk;
        } else {
          this.buffer += chunk;
        }

        console.log("[BLE] Buffer so far:", this.buffer);

        try {
          const parsed = JSON.parse(this.buffer);
          this.buffer = "";
          onMessage(JSON.stringify(parsed));
        } catch {
          // Still accumulating chunks
        }
      }
    );
}

// Helper to check if buffer is already valid JSON
private isCompleteJson(str: string): boolean {
    try { JSON.parse(str); return true; } catch { return false; }
}

  unsubscribe() {
    this.subscription?.remove()
    this.subscription = null
    this.buffer = ""
  }

  async sendMessage(message: string) {
    if (!this.device) { console.log("No device connected"); return }
    const encoded = Buffer.from(message).toString("base64")
    await this.device.writeCharacteristicWithResponseForService(
      SERVICE_UUID,
      CHARACTERISTIC_UUID,
      encoded
    )
    console.log("Sent:", message)
  }

  async disconnect() {
    this.unsubscribe()
    if (!this.device) return
    await this.device.cancelConnection()
    this.device = null
    console.log("Disconnected")
  }
}

export default new BLEManager()