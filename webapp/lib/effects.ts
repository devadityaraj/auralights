import type { Effect } from '@/lib/types'

export type EffectDefinition = {
  id: Effect
  label: string
  icon: string
  description: string
  color: string
}

export const effectDefinitions: EffectDefinition[] = [
  { id: 'Solid', label: 'Solid Color', icon: '🎨', description: 'Static single hue illumination', color: '#70e8ff' },
  { id: 'Rainbow', label: 'RGB Rainbow', icon: '🌈', description: 'Continuous spectrum wave transition', color: '#f43f5e' },
  { id: 'Breathe', label: 'Deep Breathe', icon: '🧘', description: 'Calming cool blue breathing cycle', color: '#60a5fa' },
  { id: 'Fire', label: 'Cozy Fire', icon: '🔥', description: 'Deep red and amber heat embers', color: '#f97316' },
  { id: 'Aurora', label: 'Aurora Borealis', icon: '🌌', description: 'Fluid atmospheric multi-color wave', color: '#67e8f9' },
  { id: 'Candle', label: 'Warm Candle', icon: '🕯️', description: 'Organic warm flame flicker', color: '#ffb347' },
  { id: 'Cyberwave', label: 'Cyberwave', icon: '⚡', description: 'Synthwave cyan & hot pink neon shift', color: '#ec4899' },
  { id: 'Strobe', label: 'Party Strobe', icon: '✨', description: 'High energy flash strobe rhythm', color: '#facc15' },
  { id: 'Pulse', label: 'Rhythm Pulse', icon: '💓', description: 'Electric violet rhythmic pulse', color: '#c084fc' },
  { id: 'Rain', label: 'Rain Drops', icon: '💧', description: 'Blue & cyan water drop falling streams', color: '#38bdf8' },
  { id: 'Meteor', label: 'Meteor Shower', icon: '☄️', description: 'Bright comet with fading trail', color: '#93c5fd' },
  { id: 'Twinkle', label: 'Twinkle Stars', icon: '⭐', description: 'Sparkling gold and white fairy sparkles', color: '#fde047' },
  { id: 'Chase', label: 'Color Chase', icon: '💫', description: 'Running neon pixel trail sequence', color: '#a855f7' },
  { id: 'Party', label: 'Party Lights', icon: '🎉', description: 'Vivid multi-color festival flash bursts', color: '#fb7185' },
  { id: 'Bounce', label: 'Color Bounce', icon: '🏓', description: 'Ping-pong bouncing spectrum pixel', color: '#4ade80' },
]

export const effects = effectDefinitions.map(({ id }) => id)
export type SupportedEffect = (typeof effects)[number]

export interface ColorPaletteItem {
  name: string
  color: string
  category: string
}

export const curatedPalettes: ColorPaletteItem[] = [
  { name: 'Aura Cyan', color: '#70e8ff', category: 'Signature' },
  { name: 'Neon Violet', color: '#b7a0ff', category: 'Signature' },
  { name: 'Cyber Pink', color: '#ff5ea7', category: 'Vibrant' },
  { name: 'Sunset Amber', color: '#ff9a3c', category: 'Warm' },
  { name: 'Candle Flame', color: '#ffb86c', category: 'Warm' },
  { name: 'Emerald Glow', color: '#50fa7b', category: 'Nature' },
  { name: 'Electric Lime', color: '#a3e635', category: 'Vibrant' },
  { name: 'Deep Indigo', color: '#6366f1', category: 'Deep' },
  { name: 'Royal Purple', color: '#9333ea', category: 'Deep' },
  { name: 'Ice Blizzard', color: '#cffafe', category: 'Cool' },
  { name: 'Warm White (2700K)', color: '#ffe4b5', category: 'White' },
  { name: 'Pure Daylight (5000K)', color: '#f8fafc', category: 'White' },
]
