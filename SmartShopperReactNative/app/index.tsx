import React, { useState } from 'react';
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
} from 'react-native';

const DATA = {
  shoppingResults: [
    {
      name: "Fortnum & Mason Celebration Blend Loose Leaf Tea Tin",
      link: "https://www.google.com",
      image: "https://encrypted-tbn2.gstatic.com/shopping?q=tbn:ANd9GcT8i1c_lbxc6VI_QIJzgz7xXbIWOcTJPUaDM5AK63JjAYXfBXcimHC5vF8enDL2GyT2Hdw_MGx6zBE",
      price: "$39.95",
      cost: 39.95,
      rating: null,
      reviews: null,
      store: "Williams-Sonoma",
      distance: "Nearby, 14 mi"
    },
    {
      name: "Teabloom Exceptional Loose Leaf Tea Chest",
      link: "https://www.google.com",
      image: "https://encrypted-tbn0.gstatic.com/shopping?q=tbn:ANd9GcThqlKK1ISaqiZlDE__kD4CDJ68zuNu3PEf8Gy7gp4ch9SRafnDhSyvOIN_7A",
      price: "$69.95",
      cost: 69.95,
      rating: 5,
      reviews: 70,
      store: "Teabloom",
      distance: null
    },
    {
      name: "Lovery Home Spa Gift Basket Honey & Almond Luxury Set",
      link: "https://www.google.com",
      image: "https://encrypted-tbn0.gstatic.com/shopping?q=tbn:ANd9GcRrZ5XF8kIGPL63naHzNegB3GfdnLANdQaW69EkC6iht5p1o7A530X1HSXGpMJ0EIwEHQUpLjEbcMenOELpAk0Yzqzs9fP7Hw",
      price: "$39.99",
      cost: 39.99,
      rating: 3.8,
      reviews: 112,
      store: "Target",
      distance: null
    }
  ]
};

export default function IndexScreen() {
  const [bleName, setBleName] = useState('');

  const renderItem = ({ item }) => (
    <TouchableOpacity
      style={styles.card}
      onPress={() => Linking.openURL(item.link)}
    >
      <Image source={{ uri: item.image }} style={styles.image} />

      <View style={styles.info}>
        <Text style={styles.name}>{item.name}</Text>

        <Text style={styles.price}>{item.price}</Text>

        {item.store && (
          <Text style={styles.meta}>Store: {item.store}</Text>
        )}

        {item.rating && (
          <Text style={styles.meta}>
            ⭐ {item.rating} ({item.reviews || 0} reviews)
          </Text>
        )}

        {item.distance && (
          <Text style={styles.meta}>{item.distance}</Text>
        )}
      </View>
    </TouchableOpacity>
  );

  return (
    <SafeAreaView style={styles.container}>
      
      {/* BLE Device Input */}
      <View style={styles.inputContainer}>
        <Text style={styles.inputLabel}>BLE Device Name</Text>
        <TextInput
          style={styles.input}
          placeholder="Enter device name..."
          placeholderTextColor="#888"
          value={bleName}
          onChangeText={setBleName}
        />
      </View>

      <FlatList
        data={DATA.shoppingResults}
        keyExtractor={(item, index) => index.toString()}
        renderItem={renderItem}
        contentContainerStyle={{ paddingBottom: 20 }}
      />
    </SafeAreaView>
  );
}

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: '#111',
  },

  inputContainer: {
    padding: 12,
  },
  inputLabel: {
    color: '#aaa',
    marginBottom: 6,
    fontSize: 12,
  },
  input: {
    backgroundColor: '#1c1c1e',
    color: 'white',
    padding: 10,
    borderRadius: 8,
    borderWidth: 1,
    borderColor: '#333',
  },

  card: {
    backgroundColor: '#1c1c1e',
    marginHorizontal: 12,
    marginBottom: 12,
    borderRadius: 12,
    overflow: 'hidden',
  },
  image: {
    width: '100%',
    height: 200,
  },
  info: {
    padding: 12,
  },
  name: {
    color: 'white',
    fontSize: 16,
    fontWeight: '600',
    marginBottom: 6,
  },
  price: {
    color: '#4cd964',
    fontSize: 16,
    fontWeight: 'bold',
    marginBottom: 6,
  },
  meta: {
    color: '#aaa',
    fontSize: 13,
  },
});