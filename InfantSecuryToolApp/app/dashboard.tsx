import React, { useEffect } from 'react';
import { View, Text, ScrollView, TouchableOpacity, SafeAreaView, StyleSheet } from 'react-native';
import { useRouter } from 'expo-router';
import { useBLEContext } from '../context/BLEContext';
import { useBLE } from '../hooks/useBLE';
import { useNotifications } from '../hooks/useNotifications';
import { ConnectionBadge } from '../components/ConnectionBadge';
import { SensorCard } from '../components/SensorCard';
import { SensorKey } from '../constants/ble';

const SENSOR_KEYS: SensorKey[] = ['temperature', 'humidity', 'motion', 'pulse', 'sound'];

export default function DashboardScreen() {
  const router = useRouter();
  const { connectedDevice, sensorData } = useBLEContext();
  const { disconnect } = useBLE();

  // Fire notifications when sensor data changes
  useNotifications(sensorData);

  // If disconnected, go back to setup
  useEffect(() => {
    if (!connectedDevice) router.replace('/');
  }, [connectedDevice]);

  const handleDisconnect = async () => {
    await disconnect();
    router.replace('/');
  };

  return (
    <SafeAreaView style={styles.safe}>
      <ScrollView contentContainerStyle={styles.container}>
        {/* Header */}
        <View style={styles.header}>
          <Text style={styles.title}>Baby Saver</Text>
          <ConnectionBadge
            connected={!!connectedDevice}
            deviceName={connectedDevice?.name}
          />
        </View>

        {/* Sensor cards */}
        <View style={styles.section}>
          <Text style={styles.sectionLabel}>LIVE SENSOR DATA</Text>
          {SENSOR_KEYS.map(key => (
            <SensorCard key={key} sensorKey={key} value={sensorData[key]} />
          ))}
        </View>

        {/* Disconnect */}
        <TouchableOpacity style={styles.disconnectButton} onPress={handleDisconnect}>
          <Text style={styles.disconnectText}>Disconnect</Text>
        </TouchableOpacity>
      </ScrollView>
    </SafeAreaView>
  );
}

const styles = StyleSheet.create({
  safe: {
    flex: 1,
    backgroundColor: '#f9fafb',
  },
  container: {
    padding: 24,
    paddingBottom: 48,
  },
  header: {
    marginTop: 24,
    marginBottom: 32,
    gap: 12,
  },
  title: {
    fontSize: 32,
    fontWeight: '800',
    color: '#111827',
  },
  section: {
    marginBottom: 32,
  },
  sectionLabel: {
    fontSize: 11,
    fontWeight: '700',
    color: '#9ca3af',
    letterSpacing: 1.2,
    marginBottom: 12,
  },
  disconnectButton: {
    alignItems: 'center',
    paddingVertical: 14,
    borderRadius: 12,
    borderWidth: 1.5,
    borderColor: '#ef4444',
  },
  disconnectText: {
    color: '#ef4444',
    fontWeight: '700',
    fontSize: 15,
  },
});
