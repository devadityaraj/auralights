import {
  browserLocalPersistence,
  browserSessionPersistence,
  createUserWithEmailAndPassword,
  setPersistence,
  signInWithEmailAndPassword,
  signOut,
  updateProfile,
  type User,
} from 'firebase/auth'
import { ref, set } from 'firebase/database'
import { auth, database } from '@/lib/firebase'

export const SAVED_CREDENTIALS_KEY = 'aura_saved_credentials'
export const AUTO_LOGIN_SUPPRESS_KEY = 'aura_autologin_suppressed'

export interface SavedCredentials {
  userId: string
  password: string
  savePassword: boolean
  autoLogin: boolean
  updatedAt: number
}

export function resolveAuthEmail(userIdOrEmail: string): string {
  const clean = userIdOrEmail.trim().toLowerCase()
  if (clean.includes('@')) {
    return clean
  }
  const sanitized = clean.replace(/[^a-z0-9_.-]/g, '') || 'user'
  return `${sanitized}@auralights.local`
}

export function extractUserId(user: User | null): string {
  if (!user) return ''
  if (user.displayName) return user.displayName
  if (user.email) {
    if (user.email.endsWith('@auralights.local') || user.email.endsWith('@aura.local')) {
      return user.email.split('@')[0]
    }
    return user.email
  }
  return user.uid.slice(0, 8)
}

export async function registerWithCredentials(userId: string, password: string): Promise<User> {
  if (!auth) throw new Error('Firebase is not configured.')
  const email = resolveAuthEmail(userId)
  const credential = await createUserWithEmailAndPassword(auth, email, password)
  
  try {
    await updateProfile(credential.user, { displayName: userId.trim() })
  } catch (err) {
    console.warn('Profile update failed:', err)
  }

  if (database) {
    try {
      await set(ref(database, `users/${credential.user.uid}`), {
        username: userId.trim(),
        userId: userId.trim(),
        email,
        createdAt: Date.now(),
      })
    } catch (err) {
      console.warn('Database user sync failed:', err)
    }
  }

  return credential.user
}

export async function register(username: string, email: string, password: string): Promise<User> {
  if (!auth || !database) throw new Error('Firebase is not configured.')
  const targetEmail = email.trim() || resolveAuthEmail(username)
  const credential = await createUserWithEmailAndPassword(auth, targetEmail, password)
  await updateProfile(credential.user, { displayName: username })
  await set(ref(database, `users/${credential.user.uid}`), { username, email: targetEmail })
  return credential.user
}

export async function loginWithCredentials(
  userId: string,
  password: string,
  remember: boolean = true
): Promise<User> {
  if (!auth) throw new Error('Firebase is not configured.')
  await setPersistence(auth, remember ? browserLocalPersistence : browserSessionPersistence)
  const email = resolveAuthEmail(userId)
  const credential = await signInWithEmailAndPassword(auth, email, password)
  return credential.user
}

export async function login(emailOrUserId: string, password: string, remember: boolean = true): Promise<User> {
  return loginWithCredentials(emailOrUserId, password, remember)
}

export async function logout(suppressAutoLogin: boolean = true): Promise<void> {
  if (suppressAutoLogin && typeof window !== 'undefined') {
    try {
      sessionStorage.setItem(AUTO_LOGIN_SUPPRESS_KEY, 'true')
    } catch {
    }
  }
  if (auth) {
    await signOut(auth)
  }
}

export function saveSavedCredentials(
  userId: string,
  password: string,
  savePassword = true,
  autoLogin = true
): void {
  if (typeof window === 'undefined') return
  try {
    if (!savePassword) {
      localStorage.removeItem(SAVED_CREDENTIALS_KEY)
      return
    }
    const data: SavedCredentials = {
      userId: userId.trim(),
      password,
      savePassword,
      autoLogin,
      updatedAt: Date.now(),
    }
    localStorage.setItem(SAVED_CREDENTIALS_KEY, JSON.stringify(data))
    sessionStorage.removeItem(AUTO_LOGIN_SUPPRESS_KEY)
  } catch (err) {
    console.error('Failed to save credentials:', err)
  }
}

export function getSavedCredentials(): SavedCredentials | null {
  if (typeof window === 'undefined') return null
  try {
    const raw = localStorage.getItem(SAVED_CREDENTIALS_KEY)
    if (!raw) return null
    const parsed = JSON.parse(raw) as SavedCredentials
    if (parsed && typeof parsed.userId === 'string' && typeof parsed.password === 'string') {
      return parsed
    }
  } catch {
    localStorage.removeItem(SAVED_CREDENTIALS_KEY)
  }
  return null
}

export function clearSavedCredentials(): void {
  if (typeof window === 'undefined') return
  try {
    localStorage.removeItem(SAVED_CREDENTIALS_KEY)
    sessionStorage.removeItem(AUTO_LOGIN_SUPPRESS_KEY)
  } catch (err) {
    console.error('Failed to clear saved credentials:', err)
  }
}

export function isAutoLoginSuppressed(): boolean {
  if (typeof window === 'undefined') return false
  try {
    return sessionStorage.getItem(AUTO_LOGIN_SUPPRESS_KEY) === 'true'
  } catch {
    return false
  }
}

export function formatAuthError(error: unknown): string {
  if (!(error instanceof Error)) return 'Authentication failed. Please try again.'
  const msg = error.message
  if (msg.includes('auth/invalid-credential') || msg.includes('auth/user-not-found') || msg.includes('auth/wrong-password')) {
    return 'Invalid User ID or Password. Please check your credentials.'
  }
  if (msg.includes('auth/email-already-in-use')) {
    return 'This User ID already exists. Please sign in with your password.'
  }
  if (msg.includes('auth/weak-password')) {
    return 'Password is too short. Please use at least 6 characters.'
  }
  if (msg.includes('auth/invalid-email')) {
    return 'Invalid User ID format. Please use letters and numbers.'
  }
  if (msg.includes('auth/network-request-failed')) {
    return 'Network connection error. Please check your internet.'
  }
  if (msg.includes('auth/too-many-requests')) {
    return 'Too many attempts. Access temporarily restricted. Try again shortly.'
  }
  return error.message
}
