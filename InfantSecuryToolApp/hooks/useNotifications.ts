import { useEffect, useRef } from 'react';
import * as Notifications from 'expo-notifications';
import { SensorData, THRESHOLDS } from '../constants/ble';

Notifications.setNotificationHandler({
  handleNotification: async () => ({
    shouldShowAlert: true,
    shouldPlaySound: true,
    shouldSetBadge: false,
    shouldShowBanner: true,
    shouldShowList: true,
  }),
});

export async function requestNotificationPermissions(): Promise<boolean> {
  const { status } = await Notifications.requestPermissionsAsync();
  return status === 'granted';
}

async function sendAlert(title: string, body: string) {
  await Notifications.scheduleNotificationAsync({
    content: { title, body, sound: true },
    trigger: null, // immediate
  });
}

/**
 * Watches sensorData and fires local notifications when thresholds are exceeded.
 * Cooldown: one notification per sensor per 60 seconds to avoid spamming.
 */
export function useNotifications(sensorData: SensorData) {
  const lastFiredAt = useRef<Record<string, number>>({});

  const canFire = (key: string): boolean => {
    const now = Date.now();
    if (!lastFiredAt.current[key] || now - lastFiredAt.current[key] > 60_000) {
      lastFiredAt.current[key] = now;
      return true;
    }
    return false;
  };

  useEffect(() => {
    const { temperature, humidity, motion, pulse, sound } = sensorData;

    if (temperature !== null) {
      if (temperature > THRESHOLDS.temperature.max && canFire('temp_high')) {
        sendAlert('🌡️ High Temperature Alert', `Temperature is ${temperature.toFixed(1)}°C — too hot!`);
      } else if (temperature < THRESHOLDS.temperature.min && canFire('temp_low')) {
        sendAlert('🌡️ Low Temperature Alert', `Temperature is ${temperature.toFixed(1)}°C — too cold!`);
      }
    }

    if (humidity !== null) {
      if (humidity > THRESHOLDS.humidity.max && canFire('hum_high')) {
        sendAlert('💧 High Humidity Alert', `Humidity is ${humidity.toFixed(0)}% — too humid!`);
      } else if (humidity < THRESHOLDS.humidity.min && canFire('hum_low')) {
        sendAlert('💧 Low Humidity Alert', `Humidity is ${humidity.toFixed(0)}% — too dry!`);
      }
    }

    if (motion !== null) {
      if (motion > THRESHOLDS.motion.noMotionSeconds && canFire('motion')) {
        sendAlert('🏃 No Motion Detected', `No movement detected for ${motion}s. Please check on the baby.`);
      }
    }

    if (pulse !== null) {
      if (pulse < THRESHOLDS.pulse.min && canFire('pulse_low')) {
        sendAlert('❤️ Low Heart Rate Alert', `Heart rate is ${pulse} bpm — below normal!`);
      } else if (pulse > THRESHOLDS.pulse.max && canFire('pulse_high')) {
        sendAlert('❤️ High Heart Rate Alert', `Heart rate is ${pulse} bpm — above normal!`);
      }
    }

    if (sound !== null) {
      if (sound > THRESHOLDS.sound.max && canFire('sound')) {
        sendAlert('🔊 Loud Sound Detected', `Sound level is ${sound.toFixed(0)} dB — possible crying detected!`);
      }
    }
  }, [sensorData]);
}
