import React, { createContext, useContext, useState, useRef, useCallback } from 'react';
import { BleManager, Device } from 'react-native-ble-plx';
import { DEFAULT_SENSOR_DATA, SensorData } from '../constants/ble';

interface BLEContextValue {
  manager: BleManager;
  scanning: boolean;
  connectedDevice: Device | null;
  sensorData: SensorData;
  discoveredDevices: Device[];
  startScan: () => void;
  stopScan: () => void;
  connect: (device: Device) => Promise<void>;
  disconnect: () => Promise<void>;
  setSensorData: React.Dispatch<React.SetStateAction<SensorData>>;
}

const BLEContext = createContext<BLEContextValue | null>(null);

const bleManager = new BleManager();

export function BLEProvider({ children }: { children: React.ReactNode }) {
  const [scanning, setScanning] = useState(false);
  const [connectedDevice, setConnectedDevice] = useState<Device | null>(null);
  const [sensorData, setSensorData] = useState<SensorData>(DEFAULT_SENSOR_DATA);
  const [discoveredDevices, setDiscoveredDevices] = useState<Device[]>([]);

  const startScan = useCallback(() => {
    setDiscoveredDevices([]);
    setScanning(true);
    bleManager.startDeviceScan(null, { allowDuplicates: false }, (error, device) => {
      if (error) {
        console.warn('BLE scan error:', error);
        setScanning(false);
        return;
      }
      if (device && device.name) {
        setDiscoveredDevices(prev => {
          if (prev.find(d => d.id === device.id)) return prev;
          return [...prev, device];
        });
      }
    });

    // Auto-stop scan after 15 seconds
    setTimeout(() => {
      bleManager.stopDeviceScan();
      setScanning(false);
    }, 15000);
  }, []);

  const stopScan = useCallback(() => {
    bleManager.stopDeviceScan();
    setScanning(false);
  }, []);

  const connect = useCallback(async (device: Device) => {
    bleManager.stopDeviceScan();
    setScanning(false);
    const connected = await device.connect();
    await connected.discoverAllServicesAndCharacteristics();
    setConnectedDevice(connected);
    setSensorData(DEFAULT_SENSOR_DATA);
  }, []);

  const disconnect = useCallback(async () => {
    if (connectedDevice) {
      await connectedDevice.cancelConnection();
    }
    setConnectedDevice(null);
    setSensorData(DEFAULT_SENSOR_DATA);
  }, [connectedDevice]);

  return (
    <BLEContext.Provider value={{
      manager: bleManager,
      scanning,
      connectedDevice,
      sensorData,
      discoveredDevices,
      startScan,
      stopScan,
      connect,
      disconnect,
      setSensorData,
    }}>
      {children}
    </BLEContext.Provider>
  );
}

export function useBLEContext(): BLEContextValue {
  const ctx = useContext(BLEContext);
  if (!ctx) throw new Error('useBLEContext must be used within BLEProvider');
  return ctx;
}
