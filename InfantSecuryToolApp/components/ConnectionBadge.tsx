import React from 'react';
import { View, Text, StyleSheet } from 'react-native';

interface Props {
  connected: boolean;
  deviceName?: string | null;
}

export function ConnectionBadge({ connected, deviceName }: Props) {
  return (
    <View style={[styles.badge, connected ? styles.connected : styles.disconnected]}>
      <View style={[styles.dot, connected ? styles.dotConnected : styles.dotDisconnected]} />
      <Text style={styles.text}>
        {connected ? `Connected${deviceName ? ` · ${deviceName}` : ''}` : 'Disconnected'}
      </Text>
    </View>
  );
}

const styles = StyleSheet.create({
  badge: {
    flexDirection: 'row',
    alignItems: 'center',
    paddingHorizontal: 16,
    paddingVertical: 8,
    borderRadius: 20,
    gap: 8,
  },
  connected: {
    backgroundColor: '#d1fae5',
  },
  disconnected: {
    backgroundColor: '#fee2e2',
  },
  dot: {
    width: 8,
    height: 8,
    borderRadius: 4,
  },
  dotConnected: {
    backgroundColor: '#10b981',
  },
  dotDisconnected: {
    backgroundColor: '#ef4444',
  },
  text: {
    fontSize: 14,
    fontWeight: '600',
    color: '#1f2937',
  },
});
