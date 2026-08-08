import { onValue, push, ref, remove, set, update } from 'firebase/database'
import { database } from '@/lib/firebase'
import type { LedDevice } from '@/lib/types'

export function subscribeToDevices(
  userUid: string,
  onDevices: (devices: LedDevice[]) => void,
  onError: (error: Error) => void
) {
  if (!database) return () => undefined
  return onValue(
    ref(database, `users/${userUid}/devices`),
    (snapshot) => {
      const value = snapshot.val() ?? {}
      const devices: LedDevice[] = Object.entries(value).map(([id, device]) => {
        const raw = device as Partial<Omit<LedDevice, 'id'>>
        return {
          id,
          uid: raw.uid ?? id,
          name: raw.name ?? 'Unnamed Light',
          power: raw.power ?? false,
          brightness: raw.brightness ?? 72,
          effect: raw.effect ?? 'Aurora',
          speed: raw.speed ?? 48,
          color: raw.color ?? '#70e8ff',
          colorTemp: raw.colorTemp,
          timerMinutes: raw.timerMinutes ?? null,
          timerEnd: raw.timerEnd ?? null,
          createdAt: raw.createdAt,
        }
      })
      onDevices(devices)
    },
    onError
  )
}

export async function addDevice(userUid: string, deviceUid: string, name: string) {
  if (!database) throw new Error('Firebase is not configured.')
  const sanitizedId = (
    deviceUid.trim() || push(ref(database, `users/${userUid}/devices`)).key || crypto.randomUUID()
  ).replace(/[\.#\$\[\]\/]/g, '_')

  const deviceRef = ref(database, `users/${userUid}/devices/${sanitizedId}`)
  await set(deviceRef, {
    name: name.trim(),
    uid: deviceUid.trim() || sanitizedId,
    power: true,
    brightness: 72,
    effect: 'Aurora',
    speed: 48,
    color: '#70e8ff',
    createdAt: Date.now(),
  })
  return sanitizedId
}

export async function removeDevice(userUid: string, deviceId: string) {
  if (!database) throw new Error('Firebase is not configured.')
  await remove(ref(database, `users/${userUid}/devices/${deviceId}`))
}

export async function updateDevice(userUid: string, deviceId: string, patch: Partial<LedDevice>) {
  if (!database) throw new Error('Firebase is not configured.')
  const { id: _id, ...safePatch } = patch as Partial<LedDevice> & { id?: string }
  await update(ref(database, `users/${userUid}/devices/${deviceId}`), safePatch)
}
