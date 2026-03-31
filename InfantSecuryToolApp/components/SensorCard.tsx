import React from 'react';
import { View, Text, StyleSheet } from 'react-native';
import { SensorKey, SENSOR_CONFIG, THRESHOLDS } from '../constants/ble';

interface Props {
  sensorKey: SensorKey;
  value: number | null;
}

function getStatus(key: SensorKey, value: number | null): 'normal' | 'warning' | 'alert' {
  if (value === null) return 'normal';
  switch (key) {
    case 'temperature':
      if (value > THRESHOLDS.temperature.max || value < THRESHOLDS.temperature.min) return 'alert';
      return 'normal';
    case 'humidity':
      if (value > THRESHOLDS.humidity.max || value < THRESHOLDS.humidity.min) return 'alert';
      return 'normal';
    case 'motion':
      if (value > THRESHOLDS.motion.noMotionSeconds) return 'alert';
      if (value > THRESHOLDS.motion.noMotionSeconds * 0.6) return 'warning';
      return 'normal';
    case 'pulse':
      if (value < THRESHOLDS.pulse.min || value > THRESHOLDS.pulse.max) return 'alert';
      return 'normal';
    case 'sound':
      if (value > THRESHOLDS.sound.max) return 'alert';
      if (value > THRESHOLDS.sound.max * 0.8) return 'warning';
      return 'normal';
    default:
      return 'normal';
  }
}

const STATUS_COLORS = {
  normal:  { bg: '#f0fdf4', border: '#bbf7d0', text: '#15803d' },
  warning: { bg: '#fffbeb', border: '#fde68a', text: '#b45309' },
  alert:   { bg: '#fef2f2', border: '#fecaca', text: '#b91c1c' },
};

export function SensorCard({ sensorKey, value }: Props) {
  const config = SENSOR_CONFIG[sensorKey];
  const status = getStatus(sensorKey, value);
  const colors = STATUS_COLORS[status];

  const displayValue = value !== null ? `${value.toFixed(1)} ${config.unit}` : '—';

  return (
    <View style={[styles.card, { backgroundColor: colors.bg, borderColor: colors.border }]}>
      <Text style={styles.icon}>{config.icon}</Text>
      <View style={styles.info}>
        <Text style={styles.label}>{config.label}</Text>
        <Text style={[styles.value, { color: colors.text }]}>{displayValue}</Text>
      </View>
      {status !== 'normal' && (
        <View style={[styles.statusPill, { backgroundColor: colors.border }]}>
          <Text style={[styles.statusText, { color: colors.text }]}>
            {status === 'alert' ? 'ALERT' : 'WARN'}
          </Text>
        </View>
      )}
    </View>
  );
}

const styles = StyleSheet.create({
  card: {
    flexDirection: 'row',
    alignItems: 'center',
    padding: 16,
    borderRadius: 12,
    borderWidth: 1.5,
    marginBottom: 10,
    gap: 12,
  },
  icon: {
    fontSize: 28,
  },
  info: {
    flex: 1,
  },
  label: {
    fontSize: 13,
    color: '#6b7280',
    fontWeight: '500',
  },
  value: {
    fontSize: 22,
    fontWeight: '700',
    marginTop: 2,
  },
  statusPill: {
    paddingHorizontal: 8,
    paddingVertical: 3,
    borderRadius: 8,
  },
  statusText: {
    fontSize: 11,
    fontWeight: '700',
    letterSpacing: 0.5,
  },
});
