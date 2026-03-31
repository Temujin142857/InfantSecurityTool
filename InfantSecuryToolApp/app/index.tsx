import React, { useEffect } from 'react';
import {
  View,
  Text,
  FlatList,
  TouchableOpacity,
  ActivityIndicator,
  SafeAreaView,
  StyleSheet,
} from 'react-native';
import { useRouter } from 'expo-router';
import { Device } from 'react-native-ble-plx';
import { useBLEContext } from '../context/BLEContext';
import { useBLE } from '../hooks/useBLE';

export default function SetupScreen() {
  const router = useRouter();
  const { scanning, discoveredDevices, startScan, stopScan } = useBLEContext();
  const { connectToDevice } = useBLE();

  // If already connected, go straight to dashboard
  const { connectedDevice } = useBLEContext();
  useEffect(() => {
    if (connectedDevice) router.replace('/dashboard');
  }, [connectedDevice]);

  const handleConnect = async (device: Device) => {
    try {
      await connectToDevice(device);
      router.replace('/dashboard');
    } catch (e) {
      console.warn('Connection failed:', e);
    }
  };

  return (
    <SafeAreaView style={styles.safe}>
      <View style={styles.container}>
        {/* Header */}
        <View style={styles.header}>
          <Text style={styles.title}>Baby Saver</Text>
          <Text style={styles.subtitle}>Connect to your ESP32 sensor device</Text>
        </View>

        {/* Scan button */}
        <TouchableOpacity
          style={[styles.scanButton, scanning && styles.scanButtonActive]}
          onPress={scanning ? stopScan : startScan}
        >
          {scanning && <ActivityIndicator color="#fff" style={styles.spinner} />}
          <Text style={styles.scanButtonText}>
            {scanning ? 'Scanning… (tap to stop)' : 'Scan for Devices'}
          </Text>
        </TouchableOpacity>

        {/* Device list */}
        {discoveredDevices.length === 0 && !scanning && (
          <View style={styles.empty}>
            <Text style={styles.emptyText}>No devices found. Tap scan to search.</Text>
          </View>
        )}

        <FlatList
          data={discoveredDevices}
          keyExtractor={item => item.id}
          contentContainerStyle={styles.list}
          renderItem={({ item }) => (
            <TouchableOpacity style={styles.deviceRow} onPress={() => handleConnect(item)}>
              <View>
                <Text style={styles.deviceName}>{item.name ?? 'Unknown Device'}</Text>
                <Text style={styles.deviceId}>{item.id}</Text>
              </View>
              <Text style={styles.connectText}>Connect →</Text>
            </TouchableOpacity>
          )}
        />
      </View>
    </SafeAreaView>
  );
}

const styles = StyleSheet.create({
  safe: {
    flex: 1,
    backgroundColor: '#f9fafb',
  },
  container: {
    flex: 1,
    padding: 24,
  },
  header: {
    marginTop: 24,
    marginBottom: 32,
  },
  title: {
    fontSize: 32,
    fontWeight: '800',
    color: '#111827',
  },
  subtitle: {
    fontSize: 15,
    color: '#6b7280',
    marginTop: 4,
  },
  scanButton: {
    flexDirection: 'row',
    alignItems: 'center',
    justifyContent: 'center',
    backgroundColor: '#3b82f6',
    paddingVertical: 16,
    borderRadius: 14,
    marginBottom: 24,
    gap: 8,
  },
  scanButtonActive: {
    backgroundColor: '#6b7280',
  },
  scanButtonText: {
    color: '#fff',
    fontSize: 16,
    fontWeight: '700',
  },
  spinner: {
    marginRight: 4,
  },
  empty: {
    alignItems: 'center',
    marginTop: 48,
  },
  emptyText: {
    color: '#9ca3af',
    fontSize: 15,
  },
  list: {
    gap: 10,
  },
  deviceRow: {
    flexDirection: 'row',
    alignItems: 'center',
    justifyContent: 'space-between',
    backgroundColor: '#fff',
    padding: 16,
    borderRadius: 12,
    borderWidth: 1,
    borderColor: '#e5e7eb',
  },
  deviceName: {
    fontSize: 16,
    fontWeight: '600',
    color: '#111827',
  },
  deviceId: {
    fontSize: 11,
    color: '#9ca3af',
    marginTop: 2,
  },
  connectText: {
    fontSize: 14,
    color: '#3b82f6',
    fontWeight: '600',
  },
});
