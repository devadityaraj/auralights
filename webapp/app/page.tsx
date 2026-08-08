'use client'

import { useEffect, useMemo, useState } from 'react'
import { onAuthStateChanged, type User } from 'firebase/auth'
import {
  ChevronRight,
  Clock,
  Eye,
  EyeOff,
  KeyRound,
  Lightbulb,
  LogOut,
  Palette,
  Plus,
  Power,
  RefreshCw,
  Sun,
  Trash2,
  UserRound,
  Wand2,
} from 'lucide-react'
import { auth, firebaseConfigured } from '@/lib/firebase'
import {
  clearSavedCredentials,
  extractUserId,
  formatAuthError,
  getSavedCredentials,
  isAutoLoginSuppressed,
  loginWithCredentials,
  logout,
  registerWithCredentials,
  saveSavedCredentials,
  type SavedCredentials,
} from '@/lib/services/auth'
import {
  addDevice,
  removeDevice,
  subscribeToDevices,
  updateDevice,
} from '@/lib/services/devices'
import type { LedDevice } from '@/lib/types'
import {
  curatedPalettes,
  effectDefinitions,
} from '@/lib/effects'

export default function Page() {
  const [user, setUser] = useState<User | null>(null)
  const [checkingAuth, setCheckingAuth] = useState(true)
  const [splashStatus, setSplashStatus] = useState('Checking session...')
  const [devices, setDevices] = useState<LedDevice[]>([])
  const [selectedId, setSelectedId] = useState<string>('')

  const [activeTray, setActiveTray] = useState<'color' | 'effects' | 'timer'>('effects')

  const [authMode, setAuthMode] = useState<'login' | 'register'>('login')
  const [form, setForm] = useState({ userId: '', password: '' })
  const [savePassword, setSavePassword] = useState(true)
  const [autoLogin, setAutoLogin] = useState(true)
  const [showPassword, setShowPassword] = useState(false)
  const [savedCreds, setSavedCreds] = useState<SavedCredentials | null>(null)

  const [message, setMessage] = useState('')
  const [busy, setBusy] = useState(false)
  const [showAdd, setShowAdd] = useState(false)
  const [showAccount, setShowAccount] = useState(false)
  const [newDevice, setNewDevice] = useState({ name: '', uid: '' })

  const [timerRemaining, setTimerRemaining] = useState<string | null>(null)

  useEffect(() => {
    const saved = getSavedCredentials()
    if (saved) {
      setSavedCreds(saved)
      setForm({ userId: saved.userId, password: saved.password })
      setSavePassword(saved.savePassword)
      setAutoLogin(saved.autoLogin)
    }

    if (!auth || !firebaseConfigured) {
      setCheckingAuth(false)
      return
    }

    let isMounted = true

    const unsubscribe = onAuthStateChanged(auth, async (currentUser) => {
      if (!isMounted) return

      if (currentUser) {
        setUser(currentUser)
        setCheckingAuth(false)
        return
      }

      const suppressed = isAutoLoginSuppressed()
      if (saved && saved.autoLogin && saved.userId && saved.password && !suppressed) {
        setSplashStatus(`Auto-logging in as ${saved.userId}...`)
        try {
          await loginWithCredentials(saved.userId, saved.password, true)
          return
        } catch (err) {
          console.warn('Auto-login attempt failed:', err)
          if (isMounted) {
            setMessage('Saved credentials expired. Please sign in.')
            setCheckingAuth(false)
          }
        }
      } else {
        if (isMounted) {
          setCheckingAuth(false)
        }
      }
    })

    return () => {
      isMounted = false
      unsubscribe()
    }
  }, [])

  useEffect(() => {
    if (!user) return
    return subscribeToDevices(
      user.uid,
      (next) => {
        setDevices(next)
        if (next.length > 0) {
          setSelectedId((curr) => (next.some((d) => d.id === curr) ? curr : next[0].id))
        } else {
          setSelectedId('')
        }
      },
      (error) => {
        setMessage(error.message)
      }
    )
  }, [user])

  const selected = useMemo(
    () => devices.find((device) => device.id === selectedId) ?? devices[0] ?? null,
    [devices, selectedId]
  )

  const activeDisplayColor = useMemo(() => {
    if (!selected) return '#70e8ff'
    if (selected.effect === 'Solid') return selected.color
    const eff = effectDefinitions.find((e) => e.id === selected.effect)
    return eff?.color || selected.color || '#70e8ff'
  }, [selected])

  useEffect(() => {
    if (!selected?.timerEnd) {
      setTimerRemaining(null)
      return
    }

    const interval = setInterval(() => {
      const remainingMs = (selected.timerEnd || 0) - Date.now()
      if (remainingMs <= 0) {
        setTimerRemaining(null)
        patchDevice({ power: false, timerEnd: null, timerMinutes: null })
        clearInterval(interval)
      } else {
        const mins = Math.floor(remainingMs / 60000)
        const secs = Math.floor((remainingMs % 60000) / 1000)
        setTimerRemaining(`${mins}m ${secs.toString().padStart(2, '0')}s`)
      }
    }, 1000)

    return () => clearInterval(interval)
  }, [selected?.timerEnd])

  async function submitAuth(event: React.FormEvent) {
    event.preventDefault()
    setMessage('')

    if (!firebaseConfigured) {
      setMessage('Add Firebase configuration to .env.local')
      return
    }

    const userId = form.userId.trim()
    const password = form.password

    if (!userId) {
      setMessage('Please enter a User ID.')
      return
    }

    if (password.length < 6) {
      setMessage('Password must be at least 6 characters.')
      return
    }

    setBusy(true)
    try {
      if (authMode === 'register') {
        await registerWithCredentials(userId, password)
      } else {
        await loginWithCredentials(userId, password, savePassword)
      }

      if (savePassword) {
        saveSavedCredentials(userId, password, savePassword, autoLogin)
        setSavedCreds(getSavedCredentials())
      } else {
        clearSavedCredentials()
        setSavedCreds(null)
      }
    } catch (error) {
      setMessage(formatAuthError(error))
    } finally {
      setBusy(false)
    }
  }

  async function handleLogout() {
    setBusy(true)
    setShowAccount(false)
    try {
      await logout(true)
      setUser(null)
      setDevices([])
      setMessage('Signed out successfully.')
    } catch (err) {
      console.error(err)
    } finally {
      setBusy(false)
    }
  }

  function handleClearSavedCredentials() {
    clearSavedCredentials()
    setSavedCreds(null)
    setForm({ userId: '', password: '' })
    setMessage('Saved credentials removed.')
  }

  async function patchDevice(patch: Partial<LedDevice>) {
    if (!selected) return
    setDevices((current) =>
      current.map((device) => (device.id === selected.id ? { ...device, ...patch } : device))
    )
    if (user) {
      try {
        await updateDevice(user.uid, selected.id, patch)
      } catch (error) {
        setMessage(error instanceof Error ? error.message : 'Could not update.')
      }
    }
  }

  async function handleAddLight(name: string, deviceUid: string) {
    const trimmedName = name.trim()
    const trimmedUid = deviceUid.trim()

    if (!trimmedName || !trimmedUid) {
      setMessage('Please provide both light name and device UID.')
      return
    }

    setBusy(true)
    setMessage('')
    try {
      if (user) {
        const generatedId = await addDevice(user.uid, trimmedUid, trimmedName)
        setSelectedId(generatedId)
      } else {
        const newLight: LedDevice = {
          id: trimmedUid,
          uid: trimmedUid,
          name: trimmedName,
          power: true,
          brightness: 75,
          effect: 'Aurora',
          speed: 48,
          color: '#70e8ff',
        }
        setDevices((current) => [...current, newLight])
        setSelectedId(trimmedUid)
      }
      setNewDevice({ name: '', uid: '' })
      setShowAdd(false)
    } catch (error) {
      setMessage(error instanceof Error ? error.message : 'Could not add light.')
    } finally {
      setBusy(false)
    }
  }

  if (checkingAuth && firebaseConfigured) {
    return (
      <div className="app-viewport">
        <div className="app-frame items-center justify-center text-center">
          <div className="orb-center-node mb-3 text-cyan-400" style={{ color: '#70e8ff' }}>
            <Lightbulb size={24} />
          </div>
          <p className="text-[11px] font-mono font-bold text-cyan-400 tracking-widest uppercase">
            Aura Lights
          </p>
          <p className="text-sm text-slate-300 font-medium">{splashStatus}</p>
          <button
            className="text-[11px] text-slate-500 hover:text-white mt-3 underline"
            onClick={() => setCheckingAuth(false)}
          >
            Cancel auto-login
          </button>
        </div>
      </div>
    )
  }

  if (!user && firebaseConfigured) {
    return (
      <div className="app-viewport">
        <form className="app-frame glass-panel p-5 justify-between" onSubmit={submitAuth}>
          <div className="flex items-center justify-between">
            <div className="flex items-center gap-2">
              <div className="brand-dot">
                <Lightbulb size={16} />
              </div>
              <span className="font-bold text-sm text-white">Aura Lights</span>
            </div>
            <div className="flex bg-black/40 p-1 rounded-lg border border-white/10 text-xs">
              <button
                type="button"
                className={`px-3 py-1 rounded-md transition ${authMode === 'login' ? 'bg-white/15 text-white font-semibold' : 'text-slate-400'}`}
                onClick={() => setAuthMode('login')}
              >
                Sign In
              </button>
              <button
                type="button"
                className={`px-3 py-1 rounded-md transition ${authMode === 'register' ? 'bg-white/15 text-white font-semibold' : 'text-slate-400'}`}
                onClick={() => setAuthMode('register')}
              >
                Register
              </button>
            </div>
          </div>

          <div className="flex flex-col gap-2 my-auto">
            {savedCreds && authMode === 'login' && (
              <div className="flex items-center justify-between p-2 rounded-lg bg-cyan-950/30 border border-cyan-500/20 text-xs text-cyan-300">
                <span className="flex items-center gap-1">
                  <KeyRound size={13} /> Stored: <strong>{savedCreds.userId}</strong>
                </span>
                <button type="button" className="underline text-[10px]" onClick={handleClearSavedCredentials}>
                  Clear
                </button>
              </div>
            )}

            <label className="field-label">
              User ID
              <input
                type="text"
                autoComplete="username"
                value={form.userId}
                onChange={(e) => setForm({ ...form, userId: e.target.value })}
                placeholder="e.g. alex, room_admin"
                required
                autoFocus
              />
            </label>

            <label className="field-label">
              Password
              <div className="relative">
                <input
                  type={showPassword ? 'text' : 'password'}
                  autoComplete={authMode === 'login' ? 'current-password' : 'new-password'}
                  value={form.password}
                  onChange={(e) => setForm({ ...form, password: e.target.value })}
                  placeholder="••••••••"
                  minLength={6}
                  required
                />
                <button
                  type="button"
                  className="absolute right-3 top-1/2 -translate-y-1/2 text-slate-500"
                  onClick={() => setShowPassword(!showPassword)}
                  tabIndex={-1}
                >
                  {showPassword ? <EyeOff size={15} /> : <Eye size={15} />}
                </button>
              </div>
            </label>

            <div className="flex items-center justify-between text-xs text-slate-400 mt-1 px-1">
              <label className="flex items-center gap-2 cursor-pointer">
                <input
                  type="checkbox"
                  checked={savePassword}
                  onChange={(e) => {
                    setSavePassword(e.target.checked)
                    if (!e.target.checked) setAutoLogin(false)
                  }}
                  className="accent-cyan-400"
                />
                <span>Save credentials</span>
              </label>

              <label className="flex items-center gap-2 cursor-pointer">
                <input
                  type="checkbox"
                  checked={autoLogin}
                  disabled={!savePassword}
                  onChange={(e) => setAutoLogin(e.target.checked)}
                  className="accent-cyan-400"
                />
                <span>Auto-login</span>
              </label>
            </div>
          </div>

          <div>
            <button className="primary-action-btn" disabled={busy}>
              {busy ? (
                <RefreshCw size={15} className="animate-spin" />
              ) : (
                <>
                  <span>{authMode === 'login' ? 'Enter App' : 'Create Account'}</span>
                  <ChevronRight size={15} />
                </>
              )}
            </button>
            {message && <p className="text-xs text-rose-400 text-center mt-2">{message}</p>}
          </div>
        </form>
      </div>
    )
  }

  const currentUserId = user ? extractUserId(user) : 'Guest'

  return (
    <div className="app-viewport">
      {devices.length > 0 && selected ? (
        <div className="app-frame">
          <header className="app-header glass-panel">
            <div className="header-left">
              <div className="brand-dot">
                <Lightbulb size={16} className="text-amber-300" />
              </div>
              <div className="header-meta">
                <div className="app-title-text">
                  <span>Aura Lights</span>
                </div>
                <span className="header-sub-text">
                  {devices.length} {devices.length === 1 ? 'Device' : 'Devices'} · Firebase Synced
                </span>
              </div>
            </div>

            <div className="header-actions">
              <button
                className="icon-btn"
                title="Account"
                onClick={() => setShowAccount(true)}
              >
                <UserRound size={15} />
              </button>
            </div>
          </header>

          <div className="device-select-bar">
            {devices.map((device) => {
              const isSelected = device.id === selected.id
              return (
                <button
                  key={device.id}
                  className={`device-pill ${isSelected ? 'selected' : ''}`}
                  onClick={() => setSelectedId(device.id)}
                >
                  <span
                    className="device-indicator-dot"
                    style={{
                      backgroundColor: device.power ? device.color : '#64748b',
                      boxShadow: device.power ? `0 0 8px ${device.color}` : 'none',
                    }}
                  />
                  <span>{device.name}</span>
                </button>
              )
            })}
          </div>

          <section className="hero-orb-card glass-panel" style={{ color: activeDisplayColor }}>
            <div
              className="orb-glow-layer"
              style={{
                background: activeDisplayColor,
                opacity: selected.power ? Math.max(0.14, selected.brightness / 350) : 0.02,
              }}
            />

            <div className="orb-visual-stage">
              <div className="orb-wave-pulse" />
              <div className="orb-wave-one" />
              <div className="orb-center-node">
                <Lightbulb size={24} />
              </div>
            </div>

            <div className="orb-info-row">
              <h1 className="orb-light-name">{selected.name}</h1>

              <div className="orb-metrics-bar">
                <span>
                  Brightness: <strong>{selected.brightness}%</strong>
                </span>
                <span>·</span>
                <span>
                  Effect: <strong>{selected.effect}</strong>
                </span>
                {timerRemaining && (
                  <>
                    <span>·</span>
                    <span className="text-cyan-300">
                      Off in: <strong>{timerRemaining}</strong>
                    </span>
                  </>
                )}
              </div>
            </div>
          </section>

          <section className="control-sliders-card glass-panel">
            <div className="slider-row-item">
              <div className="slider-header-text">
                <span className="flex items-center gap-1">
                  <Sun size={12} className="text-amber-400" /> Brightness
                </span>
                <span className="slider-val-badge">{selected.brightness}%</span>
              </div>
              <input
                type="range"
                min="1"
                max="100"
                value={selected.brightness}
                className="fluid-slider"
                style={{ '--percent': `${selected.brightness}%` } as React.CSSProperties}
                onChange={(e) => patchDevice({ brightness: Number(e.target.value) })}
              />
            </div>
          </section>

          <nav className="mode-segmented-bar">
            <button
              className={`mode-tab-btn ${activeTray === 'color' ? 'active' : ''}`}
              onClick={() => setActiveTray('color')}
            >
              <Palette size={12} /> Color
            </button>
            <button
              className={`mode-tab-btn ${activeTray === 'effects' ? 'active' : ''}`}
              onClick={() => setActiveTray('effects')}
            >
              <Wand2 size={12} /> Effects ({effectDefinitions.length})
            </button>
            <button
              className={`mode-tab-btn ${activeTray === 'timer' ? 'active' : ''}`}
              onClick={() => setActiveTray('timer')}
            >
              <Clock size={12} /> Timer
            </button>
          </nav>

          <div className="mode-content-tray glass-panel">
            {activeTray === 'color' && (
              <div className="color-swatches-grid">
                {curatedPalettes.slice(0, 5).map((p) => {
                  const isActive = selected.color.toLowerCase() === p.color.toLowerCase() && selected.effect === 'Solid'
                  return (
                    <button
                      key={p.name}
                      className={`color-swatch-item ${isActive ? 'active' : ''}`}
                      style={{ backgroundColor: p.color }}
                      title={p.name}
                      onClick={() => patchDevice({ color: p.color, effect: 'Solid' })}
                    />
                  )
                })}
                <div className="custom-picker-slot" title="Custom Color Picker">
                  <Palette size={14} className="text-slate-300" />
                  <input
                    type="color"
                    value={selected.color}
                    onChange={(e) => patchDevice({ color: e.target.value, effect: 'Solid' })}
                  />
                </div>
              </div>
            )}

            {activeTray === 'effects' && (
              <div className="effects-compact-grid">
                {effectDefinitions.map((eff) => {
                  const isActive = selected.effect === eff.id
                  return (
                    <button
                      key={eff.id}
                      className={`effect-compact-btn ${isActive ? 'active' : ''}`}
                      onClick={() => patchDevice({ effect: eff.id })}
                      title={eff.description}
                    >
                      <span className="effect-btn-icon">{eff.icon}</span>
                      <span className="effect-btn-label">{eff.label.split(' ')[0]}</span>
                    </button>
                  )
                })}
              </div>
            )}

            {activeTray === 'timer' && (
              <div className="timer-compact-grid">
                {[15, 30, 60, 120].map((mins) => (
                  <button
                    key={mins}
                    className={`timer-compact-btn ${
                      selected.timerMinutes === mins ? 'active' : ''
                    }`}
                    onClick={() => {
                      const timerEnd = Date.now() + mins * 60 * 1000
                      patchDevice({ timerMinutes: mins, timerEnd })
                    }}
                  >
                    {mins >= 60 ? `${mins / 60}h` : `${mins}m`}
                  </button>
                ))}
                <button
                  className="timer-compact-btn text-rose-300"
                  onClick={() => patchDevice({ timerMinutes: null, timerEnd: null })}
                >
                  Cancel
                </button>
              </div>
            )}
          </div>

          <footer className="bottom-control-bar">
            <button
              className={`bottom-power-pill ${selected.power ? 'is-on' : ''}`}
              onClick={() => patchDevice({ power: !selected.power })}
            >
              <Power size={15} />
              <span>{selected.power ? 'LIGHT IS POWERED ON' : 'LIGHT IS OFF'}</span>
            </button>
          </footer>
        </div>
      ) : (
        <div className="app-frame">
          <header className="app-header glass-panel">
            <div className="header-left">
              <div className="brand-dot">
                <Lightbulb size={16} />
              </div>
              <span className="font-bold text-sm text-white">Aura Lights</span>
            </div>
            <button className="icon-btn" onClick={() => setShowAccount(true)}>
              <UserRound size={15} />
            </button>
          </header>

          <form
            className="empty-single-frame glass-panel"
            onSubmit={(e) => {
              e.preventDefault()
              handleAddLight(newDevice.name, newDevice.uid)
            }}
          >
            <div className="empty-title-block">
              <div className="orb-center-node mx-auto mb-2 text-cyan-400" style={{ color: '#70e8ff' }}>
                <Lightbulb size={22} />
              </div>
              <h1>Connect your light</h1>
              <p>Enter the friendly name and hardware Device UID to connect with Firebase.</p>
            </div>

            <label className="field-label" style={{ marginTop: 0 }}>
              Light Name <span>(shown in app)</span>
              <input
                value={newDevice.name}
                onChange={(e) => setNewDevice({ ...newDevice, name: e.target.value })}
                placeholder="e.g. Living room strip, Desk lamp"
                required
                autoFocus
              />
            </label>

            <label className="field-label">
              Device UID <span>(hardware ID for Firebase connect)</span>
              <input
                value={newDevice.uid}
                onChange={(e) => setNewDevice({ ...newDevice, uid: e.target.value })}
                placeholder="e.g. ESP32-AURA-01, room_strip_1"
                required
              />
            </label>

            <button
              type="submit"
              className="primary-action-btn"
              disabled={busy || !newDevice.name.trim() || !newDevice.uid.trim()}
            >
              {busy ? (
                <RefreshCw size={15} className="animate-spin" />
              ) : (
                <>
                  <Plus size={15} />
                  <span>Connect Light</span>
                </>
              )}
            </button>

            {message && <p className="text-xs text-rose-400 text-center">{message}</p>}
          </form>

          <div className="text-center text-[10px] text-slate-500 font-mono">
            Firebase Realtime Database · {currentUserId}
          </div>
        </div>
      )}

      {showAccount && (
        <div className="modal-backdrop" onMouseDown={() => setShowAccount(false)}>
          <div className="modal-dialog" onMouseDown={(e) => e.stopPropagation()}>
            <div className="flex items-center justify-between mb-4">
              <div>
                <p className="text-[10px] font-mono font-bold text-cyan-400">SESSION</p>
                <h2 className="text-base font-bold text-white">{currentUserId}</h2>
              </div>
              <button
                className="icon-btn"
                onClick={() => setShowAccount(false)}
              >
                <Plus size={18} className="rotate-45" />
              </button>
            </div>

            <div className="flex flex-col gap-2 text-xs">
              <div className="flex justify-between p-2 rounded-lg bg-black/30 border border-white/5 text-slate-300">
                <span>Auto-login</span>
                <span className="font-mono text-cyan-400 font-bold">
                  {savedCreds?.autoLogin ? 'Active' : 'Disabled'}
                </span>
              </div>

              <button
                className="flex items-center justify-center gap-2 p-2.5 rounded-lg bg-cyan-500/10 border border-cyan-500/30 text-cyan-300 hover:bg-cyan-500/20 font-medium transition-colors mt-1"
                onClick={() => {
                  setShowAccount(false)
                  setShowAdd(true)
                }}
              >
                <Plus size={14} /> Add New Light Connection
              </button>

              {devices.length > 0 && selected && user && (
                <button
                  className="flex items-center justify-center gap-2 p-2 rounded-lg bg-rose-950/20 border border-rose-500/30 text-rose-300 hover:bg-rose-950/40 mt-1"
                  onClick={async () => {
                    if (confirm(`Remove "${selected.name}"?`)) {
                      await removeDevice(user.uid, selected.id)
                    }
                  }}
                >
                  <Trash2 size={14} /> Remove Current Light
                </button>
              )}

              {user && (
                <button
                  className="flex items-center justify-center gap-2 p-2 rounded-lg bg-white/5 border border-white/10 text-slate-300 hover:bg-white/10 mt-1"
                  onClick={handleLogout}
                >
                  <LogOut size={14} /> Sign Out
                </button>
              )}
            </div>
          </div>
        </div>
      )}

      {showAdd && (
        <div className="modal-backdrop" onMouseDown={() => setShowAdd(false)}>
          <form
            className="modal-dialog"
            onSubmit={(e) => {
              e.preventDefault()
              handleAddLight(newDevice.name, newDevice.uid)
            }}
            onMouseDown={(e) => e.stopPropagation()}
          >
            <div className="flex items-center justify-between mb-3">
              <div>
                <p className="text-[10px] font-mono font-bold text-cyan-400">NEW LIGHT</p>
                <h2 className="text-base font-bold text-white">Add Connection</h2>
              </div>
              <button
                type="button"
                className="icon-btn"
                onClick={() => setShowAdd(false)}
              >
                <Plus size={18} className="rotate-45" />
              </button>
            </div>

            <label className="field-label">
              Light Name <span>(shown in app)</span>
              <input
                value={newDevice.name}
                onChange={(e) => setNewDevice({ ...newDevice, name: e.target.value })}
                placeholder="e.g. Living room strip"
                autoFocus
                required
              />
            </label>

            <label className="field-label">
              Device UID <span>(hardware ID for Firebase)</span>
              <input
                value={newDevice.uid}
                onChange={(e) => setNewDevice({ ...newDevice, uid: e.target.value })}
                placeholder="e.g. ESP32-AURA-01"
                required
              />
            </label>

            <button
              type="submit"
              className="primary-action-btn"
              disabled={busy}
            >
              {busy ? <RefreshCw size={15} className="animate-spin" /> : 'Connect Light'}
            </button>
          </form>
        </div>
      )}
    </div>
  )
}
