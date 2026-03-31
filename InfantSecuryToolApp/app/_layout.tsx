import { useEffect } from 'react';
import { Stack } from 'expo-router';
import { BLEProvider } from '../context/BLEContext';
import { requestNotificationPermissions } from '../hooks/useNotifications';

export default function RootLayout() {
  useEffect(() => {
    requestNotificationPermissions();
  }, []);

  return (
    <BLEProvider>
      <Stack screenOptions={{ headerShown: false }} />
    </BLEProvider>
  );
}
