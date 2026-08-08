export type Effect =
  | 'Solid'
  | 'Rainbow'
  | 'Breathe'
  | 'Fire'
  | 'Aurora'
  | 'Candle'
  | 'Cyberwave'
  | 'Strobe'
  | 'Pulse'
  | 'Rain'
  | 'Meteor'
  | 'Twinkle'
  | 'Chase'
  | 'Party'
  | 'Bounce'

export type LedDevice = {
  id: string
  name: string
  uid: string
  power: boolean
  brightness: number
  effect: Effect
  speed: number
  color: string
  colorTemp?: number
  timerMinutes?: number | null
  timerEnd?: number | null
  createdAt?: number
}

export type UserProfile = {
  username: string
  email: string
}
