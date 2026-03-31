import { useEffect, useCallback } from 'react';
import { Platform, PermissionsAndroid } from 'react-native';
import { useBLEContext } from '../context/BLEContext';
import { ESP32_SERVICE_UUID, CHARACTERISTIC_UUIDS, SensorKey } from '../constants/ble';

/**
 * Requests Android BLE permissions (Android 12+).
 */
async function requestAndroidPermissions(): Promise<boolean> {
  if (Platform.OS !== 'android') return true;
  const granted = await PermissionsAndroid.requestMultiple([
    PermissionsAndroid.PERMISSIONS.BLUETOOTH_SCAN,
    PermissionsAndroid.PERMISSIONS.BLUETOOTH_CONNECT,
    PermissionsAndroid.PERMISSIONS.ACCESS_FINE_LOCATION,
  ]);
  return Object.values(granted).every(r => r === PermissionsAndroid.RESULTS.GRANTED);
}

/**
 * Decodes a base64 BLE characteristic value to a float.
 * The ESP32 firmware must encode values as a 4-byte little-endian float.
 */
function decodeFloat(base64: string): number {
  const binary = atob(base64);
  const bytes = new Uint8Array(4);
  for (let i = 0; i < 4; i++) bytes[i] = binary.charCodeAt(i);
  return new DataView(bytes.buffer).getFloat32(0, true);
}

export function useBLE() {
  const { manager, connectedDevice, connect: ctxConnect, disconnect, setSensorData } = useBLEContext();

  // Request permissions on mount (Android)
  useEffect(() => {
    requestAndroidPermissions();
  }, []);

  // Subscribe to all sensor characteristics once device is connected
  useEffect(() => {
    if (!connectedDevice) return;

    const subscriptions = (Object.keys(CHARACTERISTIC_UUIDS) as SensorKey[]).map(key => {
      return connectedDevice.monitorCharacteristicForService(
        ESP32_SERVICE_UUID,
        CHARACTERISTIC_UUIDS[key],
        (error, characteristic) => {
          if (error) {
            console.warn(`BLE characteristic error [${key}]:`, error.message);
            return;
          }
          if (characteristic?.value) {
            const value = decodeFloat(characteristic.value);
            setSensorData(prev => ({ ...prev, [key]: value }));
          }
        }
      );
    });

    return () => {
      subscriptions.forEach(sub => sub.remove());
    };
  }, [connectedDevice]);

  // Monitor unexpected disconnections
  useEffect(() => {
    if (!connectedDevice) return;
    const sub = manager.onDeviceDisconnected(connectedDevice.id, () => {
      disconnect();
    });
    return () => sub.remove();
  }, [connectedDevice]);

  const connectToDevice = useCallback(async (device: Parameters<typeof ctxConnect>[0]) => {
    await ctxConnect(device);
  }, [ctxConnect]);

  return { connectToDevice, disconnect };
}
