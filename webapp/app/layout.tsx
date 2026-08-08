import { Analytics } from '@vercel/analytics/next'
import { DM_Mono, Manrope, Newsreader } from 'next/font/google'
import type { Metadata, Viewport } from 'next'
import './globals.css'

const inter = Manrope({ subsets: ['latin'], variable: '--font-inter' })
const display = Newsreader({ subsets: ['latin'], variable: '--font-display', style: ['normal', 'italic'] })
const mono = DM_Mono({ subsets: ['latin'], variable: '--font-mono', weight: ['400'] })

export const metadata: Metadata = {
  title: 'Aura Lights',
  description: 'Control your lights with Aura Lights.',
  generator: 'Aura Lights',
  icons: {
    icon: [
      {
        url: '/icon.svg',
        type: 'image/svg+xml',
      },
    ],
    apple: '/icon.svg',
  },
}

export const viewport: Viewport = {
  colorScheme: 'dark',
  themeColor: '#07090d',
  userScalable: false,
}

export default function RootLayout({
  children,
}: Readonly<{
  children: React.ReactNode
}>) {
  return (
    <html lang="en" className="bg-background dark">
      <body className={`${inter.variable} ${display.variable} ${mono.variable} antialiased`}>
        {children}
        {process.env.NODE_ENV === 'production' && <Analytics />}
      </body>
    </html>
  )
}
