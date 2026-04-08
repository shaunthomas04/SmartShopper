import React, { useState, useEffect } from 'react';
import {
  SafeAreaView,
  View,
  Text,
  Image,
  FlatList,
  StyleSheet,
  TouchableOpacity,
  Linking,
  TextInput,
  ActivityIndicator,
  Alert,
} from 'react-native';
import BLEManager from '../ble/BLEManager';

type ShoppingItem = {
  name: string;
  link: string;
  image: string;
  price: string;
  cost: number;
  rating: number | null;
  reviews: number | null;
  store: string | null;
  distance: string | null;
};

type BLEStatus = 'idle' | 'scanning' | 'connecting' | 'connected' | 'error';

export default function IndexScreen() {
  const [bleName, setBleName] = useState('');
  const [status, setStatus] = useState<BLEStatus>('idle');
  const [statusMessage, setStatusMessage] = useState('');
  const [items, setItems] = useState<ShoppingItem[]>([]);

  // Cleanup on unmount
  useEffect(() => {
    return () => {
      BLEManager.unsubscribe();
      BLEManager.disconnect();
    };
  }, []);

  const handleConnect = async () => {
    if (!bleName.trim()) {
      Alert.alert('Enter a device name first');
      return;
    }

    try {
      // 1. Request permissions (Android)
      await BLEManager.requestPermissions();

      // 2. Scan
      setStatus('scanning');
      setStatusMessage(`Scanning for "${bleName}"...`);

      BLEManager.scanForDevice(bleName.trim(), async (device) => {
        try {
          // 3. Connect
          setStatus('connecting');
          setStatusMessage('Connecting...');
          await BLEManager.connect(device);

          // 4. Subscribe to notifications
          setStatus('connected');
          setStatusMessage(`Connected to ${bleName}`);

          BLEManager.subscribeToNotifications((message) => {
            try {
              const parsed = JSON.parse(message);
              // Handle both array and { shoppingResults: [...] } formats
              const results: ShoppingItem[] = Array.isArray(parsed)
                ? parsed
                : parsed.shoppingResults ?? [];
              setItems(results);
            } catch {
              console.log('Non-JSON message received:', message);
            }
          });
        } catch (err) {
          setStatus('error');
          setStatusMessage('Failed to connect');
          console.log('Connect error:', err);
        }
      });
    } catch (err) {
      setStatus('error');
      setStatusMessage('Permission or scan error');
      console.log(err);
    }
  };

  const handleDisconnect = async () => {
    BLEManager.stopScan();
    await BLEManager.disconnect();
    setStatus('idle');
    setStatusMessage('');
    setItems([]);
  };

  const renderItem = ({ item }: { item: ShoppingItem }) => (
    <TouchableOpacity
      style={styles.card}
      onPress={() => Linking.openURL(item.link)}
    >
      <Image source={{ uri: item.image }} style={styles.image} />
      <View style={styles.info}>
        <Text style={styles.name}>{item.name}</Text>
        <Text style={styles.price}>{item.price}</Text>
        {item.store && <Text style={styles.meta}>Store: {item.store}</Text>}
        {item.rating != null && (
          <Text style={styles.meta}>
            ⭐ {item.rating} ({item.reviews ?? 0} reviews)
          </Text>
        )}
        {item.distance && <Text style={styles.meta}>{item.distance}</Text>}
      </View>
    </TouchableOpacity>
  );

  const isConnected = status === 'connected';

  return (
    <SafeAreaView style={styles.container}>

      {/* BLE Input + Button */}
      <View style={styles.inputContainer}>
        <Text style={styles.inputLabel}>BLE Device Name</Text>
        <View style={styles.inputRow}>
          <TextInput
            style={[styles.input, { flex: 1 }]}
            placeholder="Enter device name..."
            placeholderTextColor="#888"
            value={bleName}
            onChangeText={setBleName}
            editable={!isConnected && status !== 'scanning'}
          />
          <TouchableOpacity
            style={[styles.button, isConnected ? styles.buttonDisconnect : styles.buttonConnect]}
            onPress={isConnected ? handleDisconnect : handleConnect}
            disabled={status === 'scanning' || status === 'connecting'}
          >
            <Text style={styles.buttonText}>
              {isConnected ? 'Disconnect' : 'Connect'}
            </Text>
          </TouchableOpacity>
        </View>

        {/* Status row */}
        {(status !== 'idle') && (
          <View style={styles.statusRow}>
            {(status === 'scanning' || status === 'connecting') && (
              <ActivityIndicator size="small" color="#4cd964" style={{ marginRight: 6 }} />
            )}
            <Text style={[
              styles.statusText,
              status === 'connected' && { color: '#4cd964' },
              status === 'error' && { color: '#ff3b30' },
            ]}>
              {statusMessage}
            </Text>
          </View>
        )}
      </View>

      {/* Results */}
      {items.length === 0 && isConnected && (
        <View style={styles.emptyState}>
          <Text style={styles.emptyText}>Waiting for data from device...</Text>
          <ActivityIndicator color="#4cd964" style={{ marginTop: 10 }} />
        </View>
      )}

      <FlatList
        data={items}
        keyExtractor={(_, index) => index.toString()}
        renderItem={renderItem}
        contentContainerStyle={{ paddingBottom: 20 }}
      />

    </SafeAreaView>
  );
}

const styles = StyleSheet.create({
  container:      { flex: 1, backgroundColor: '#111' },
  inputContainer: { padding: 12 },
  inputLabel:     { color: '#aaa', marginBottom: 6, fontSize: 12 },
  inputRow:       { flexDirection: 'row', gap: 8, alignItems: 'center' },
  input: {
    backgroundColor: '#1c1c1e',
    color: 'white',
    padding: 10,
    borderRadius: 8,
    borderWidth: 1,
    borderColor: '#333',
  },
  button: {
    paddingHorizontal: 14,
    paddingVertical: 10,
    borderRadius: 8,
  },
  buttonConnect:    { backgroundColor: '#4cd964' },
  buttonDisconnect: { backgroundColor: '#ff3b30' },
  buttonText:       { color: '#000', fontWeight: '700', fontSize: 14 },
  statusRow:        { flexDirection: 'row', alignItems: 'center', marginTop: 8 },
  statusText:       { color: '#aaa', fontSize: 12 },
  emptyState:       { alignItems: 'center', marginTop: 40 },
  emptyText:        { color: '#555', fontSize: 14 },
  card: {
    backgroundColor: '#1c1c1e',
    marginHorizontal: 12,
    marginBottom: 12,
    borderRadius: 12,
    overflow: 'hidden',
  },
  image:  { width: '100%', height: 200 },
  info:   { padding: 12 },
  name:   { color: 'white', fontSize: 16, fontWeight: '600', marginBottom: 6 },
  price:  { color: '#4cd964', fontSize: 16, fontWeight: 'bold', marginBottom: 6 },
  meta:   { color: '#aaa', fontSize: 13 },
});