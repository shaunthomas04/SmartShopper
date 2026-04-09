import React, { useState, useEffect } from 'react';
import {
  SafeAreaView,
  View,
  Text,
  Image,
  StyleSheet,
  TouchableOpacity,
  Linking,
  TextInput,
  ActivityIndicator,
  Alert,
  ScrollView,
} from 'react-native';
import BLEManager from '../ble/BLEManager';

// Define the shape of a single shopping item
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
  index?: number; // Added to track position from M5
};

type BLEStatus = 'idle' | 'scanning' | 'connecting' | 'connected' | 'error';

export default function IndexScreen() {
  const [bleName, setBleName] = useState('');
  const [status, setStatus] = useState<BLEStatus>('idle');
  const [statusMessage, setStatusMessage] = useState('');
  
  // Changed from items[] to a single currentItem
  const [currentItem, setCurrentItem] = useState<ShoppingItem | null>(null);

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
              
              // Handle single object or array fallback
              if (parsed && !Array.isArray(parsed)) {
                setCurrentItem(parsed);
              } else if (Array.isArray(parsed) && parsed.length > 0) {
                setCurrentItem(parsed[0]);
              }
            } catch (err) {
              console.log('Non-JSON or malformed message received:', message);
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
    setCurrentItem(null);
  };

  const isConnected = status === 'connected';

  return (
    <SafeAreaView style={styles.container}>
      {/* BLE Input + Connection Controls */}
      <View style={styles.inputContainer}>
        <Text style={styles.inputLabel}>BLE Device Name</Text>
        <View style={styles.inputRow}>
          <TextInput
            style={[styles.input, { flex: 1 }]}
            placeholder="Enter device name (e.g. shaun2026)"
            placeholderTextColor="#888"
            value={bleName}
            onChangeText={setBleName}
            autoCapitalize="none"
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

        {/* Status Indicator */}
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

      <ScrollView contentContainerStyle={styles.mainContent}>
        {currentItem ? (
          <View style={styles.selectionContainer}>
            <Text style={styles.liveLabel}>● LIVE SELECTION</Text>
            
            <TouchableOpacity
              style={styles.card}
              activeOpacity={0.9}
              onPress={() => currentItem.link && Linking.openURL(currentItem.link)}
            >
              <Image 
                source={{ uri: currentItem.image || 'https://m.media-amazon.com/images/I/51ergTbpNlL.jpg' }} 
                style={styles.image} 
                resizeMode="cover"
              />
              <View style={styles.info}>
                <Text style={styles.name}>{currentItem.name}</Text>
                <Text style={styles.price}>{currentItem.price}</Text>
                
                <View style={styles.detailsRow}>
                  {currentItem.store && <Text style={styles.meta}>📍 {currentItem.store}</Text>}
                  {currentItem.distance && <Text style={styles.meta}> • {currentItem.distance}</Text>}
                </View>

                {currentItem.rating != null && (
                  <Text style={styles.meta}>
                    ⭐ {currentItem.rating} ({currentItem.reviews ?? 0} reviews)
                  </Text>
                )}
              </View>
            </TouchableOpacity>

            <Text style={styles.helperText}>Scroll on your M5Stack to update this view</Text>
          </View>
        ) : isConnected ? (
          <View style={styles.emptyState}>
            <Text style={styles.emptyText}>Connected! Waiting for item data...</Text>
            <ActivityIndicator color="#4cd964" style={{ marginTop: 12 }} />
          </View>
        ) : (
          <View style={styles.emptyState}>
            <Text style={styles.emptyText}>Connect to your M5Stack to view items</Text>
          </View>
        )}
      </ScrollView>
    </SafeAreaView>
  );
}

const styles = StyleSheet.create({
  container: { 
    flex: 1, 
    backgroundColor: '#000' 
  },
  inputContainer: { 
    padding: 16, 
    backgroundColor: '#111',
    borderBottomWidth: 1,
    borderColor: '#222'
  },
  inputLabel: { 
    color: '#888', 
    marginBottom: 8, 
    fontSize: 12,
    fontWeight: '600'
  },
  inputRow: { 
    flexDirection: 'row', 
    gap: 10, 
    alignItems: 'center' 
  },
  input: {
    backgroundColor: '#1c1c1e',
    color: 'white',
    padding: 12,
    borderRadius: 10,
    borderWidth: 1,
    borderColor: '#333',
  },
  button: {
    paddingHorizontal: 16,
    paddingVertical: 12,
    borderRadius: 10,
    justifyContent: 'center',
  },
  buttonConnect: { 
    backgroundColor: '#4cd964' 
  },
  buttonDisconnect: { 
    backgroundColor: '#ff3b30' 
  },
  buttonText: { 
    color: '#000', 
    fontWeight: 'bold', 
    fontSize: 14 
  },
  statusRow: { 
    flexDirection: 'row', 
    alignItems: 'center', 
    marginTop: 10 
  },
  statusText: { 
    color: '#aaa', 
    fontSize: 13 
  },
  mainContent: {
    flexGrow: 1,
    paddingVertical: 30,
    alignItems: 'center',
  },
  selectionContainer: {
    width: '100%',
    alignItems: 'center',
  },
  liveLabel: {
    color: '#4cd964',
    fontSize: 12,
    fontWeight: '900',
    marginBottom: 15,
    letterSpacing: 2,
  },
  card: {
  backgroundColor: '#1c1c1e',
  width: '90%',
  borderRadius: 24,
  // Remove overflow: 'hidden' temporarily to see if the image is just being clipped
  borderWidth: 1,
  borderColor: '#333',
  minHeight: 350, // Force a minimum height for the whole card
},
image: { 
  width: '100%', 
  height: 250,
  backgroundColor: '#333', // This will show a grey box if the image fails to load
  borderTopLeftRadius: 24,
  borderTopRightRadius: 24,
},
  info: { 
    padding: 20 
  },
  name: { 
    color: 'white', 
    fontSize: 20, 
    fontWeight: 'bold', 
    marginBottom: 8 
  },
  price: { 
    color: '#4cd964', 
    fontSize: 22, 
    fontWeight: '800', 
    marginBottom: 12 
  },
  detailsRow: {
    flexDirection: 'row',
    marginBottom: 6,
    flexWrap: 'wrap',
  },
  meta: { 
    color: '#aaa', 
    fontSize: 14,
    marginBottom: 4,
  },
  helperText: { 
    color: '#444', 
    fontSize: 13, 
    marginTop: 20,
    fontStyle: 'italic'
  },
  emptyState: { 
    alignItems: 'center', 
    marginTop: 60,
    paddingHorizontal: 40 
  },
  emptyText: { 
    color: '#555', 
    fontSize: 16, 
    textAlign: 'center',
    lineHeight: 24
  },
});