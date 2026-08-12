# Aura Lights

Smart LED strip controller using an ESP8266 and a Next.js web app, connected through Firebase Realtime Database. Control your LED strips in real time from any browser or install the app to your phone.

---

## How It Works

The ESP8266 authenticates with Firebase and listens to its device node over a persistent SSE stream. When you change a setting on the web app, Firebase pushes the change to the ESP8266 immediately. No polling. No manual refresh.

---

## Folder Structure

```
Aura Lights/
├── ESP8266_LED_Controller/     # Arduino firmware
│   ├── ESP8266_LED_Controller.ino
│   ├── config.h                # LED pin, count, timing constants
│   ├── secrets.h               # WiFi, Firebase credentials, device ID
│   ├── firebase_manager.cpp/h
│   ├── led_controller.cpp/h
│   ├── device_state.cpp/h
│   └── wifi_manager.cpp/h
└── webapp/                     # Next.js web app
    ├── app/
    ├── lib/
    │   ├── firebase.ts
    │   ├── effects.ts
    │   ├── types.ts
    │   └── services/
    │       ├── auth.ts
    │       └── devices.ts
    ├── .env.example
    └── vercel.json
```

---

## Prerequisites

- Arduino IDE with ESP8266 board support installed
- Node.js 18+ and npm / pnpm
- A Firebase project (free Spark plan is sufficient)
- A WS2812B LED strip

### Arduino Libraries Required

Install these from Arduino IDE > Library Manager:

- `Firebase ESP8266 Client` by Mobizt
- `FastLED`

---

## Part 1 — Firebase Setup

This step is shared between the firmware and the web app. Do it once.

**1. Create a Firebase project**

Go to [console.firebase.google.com](https://console.firebase.google.com) and create a new project. Analytics is optional.

**2. Enable Realtime Database**

- In the left sidebar: Build > Realtime Database
- Click Create Database
- Choose any region
- Start in **test mode** for now (you will lock it down after setup)

**3. Enable Email/Password Authentication**

- Build > Authentication > Get started
- Sign-in method tab > Email/Password > Enable
- Save

**4. Create a dedicated device account**

This is the account the ESP8266 uses to authenticate.

- Authentication > Users > Add user
- Use any email and a strong password, e.g. `device@auralights.local` / `yourpassword`
- Copy the UID shown after creation

**5. Set database security rules**

- Realtime Database > Rules tab
- Replace the default rules with:

```json
{
  "rules": {
    "users": {
      "$uid": {
        ".read": "$uid === auth.uid",
        ".write": "$uid === auth.uid"
      }
    }
  }
}
```

- Publish

**6. Get your web app credentials**

- Project Settings (gear icon) > General > Your apps
- Click Add app > Web
- Register the app, skip Firebase Hosting
- Copy the config object shown. You will need these values for the web app.

---

## Part 2 — ESP8266 Firmware

**1. Open the project**

Open `ESP8266_LED_Controller/ESP8266_LED_Controller.ino` in Arduino IDE. The IDE will load all `.cpp` and `.h` files in the same folder automatically.

**2. Fill in `config.h`**

```cpp
#define LED_PIN_1        D2       // Data pin for strip 1
#define LED_COUNT_1      24       // Number of LEDs on strip 1
#define LED_CHIPSET      WS2812B
#define LED_COLOR_ORDER  GRB

#define DUAL_STRIP_ENABLED  false // Set true if using two strips
#define LED_PIN_2        D3
#define LED_COUNT_2      30
```

**3. Fill in `secrets.h`**

```cpp
#define WIFI_SSID           "your_wifi_name"
#define WIFI_PASSWORD       "your_wifi_password"

#define FIREBASE_API_KEY        "your_web_api_key"
#define FIREBASE_DATABASE_URL   "https://your-project-default-rtdb.firebaseio.com"

#define FIREBASE_USER_EMAIL    "device@auralights.local"
#define FIREBASE_USER_PASSWORD "yourpassword"

#define DEVICE_ID              "my_led_strip"
```

- `FIREBASE_API_KEY` is the Web API Key from Project Settings > General
- `FIREBASE_DATABASE_URL` is shown in Realtime Database > Data tab
- `FIREBASE_USER_EMAIL` and `FIREBASE_USER_PASSWORD` are from the device account you created in Part 1, Step 4
- `DEVICE_ID` can be any string. It is the key used in the database path. Use something short with no spaces.

**4. Select board and upload**

- Tools > Board > ESP8266 Boards > NodeMCU 1.0 (or your specific board)
- Tools > Port > select your COM port
- Upload

**5. Verify connection**

Open Serial Monitor at 115200 baud. You should see:

```
[Firebase] Authentication ready & token valid
[Firebase] Device path: /users/<uid>/devices/my_led_strip
[Firebase] Realtime stream active
```

---

## Part 3 — Web App

**1. Install dependencies**

```bash
cd webapp
npm install
```

Or with pnpm:

```bash
pnpm install
```

**2. Create environment file**

Copy the example:

```bash
cp .env.example .env.local
```

Fill in `.env.local` using the Firebase config values from Part 1, Step 6:

```env
NEXT_PUBLIC_FIREBASE_API_KEY="..."
NEXT_PUBLIC_FIREBASE_AUTH_DOMAIN="your-project.firebaseapp.com"
NEXT_PUBLIC_FIREBASE_DATABASE_URL="https://your-project-default-rtdb.firebaseio.com"
NEXT_PUBLIC_FIREBASE_PROJECT_ID="your-project-id"
NEXT_PUBLIC_FIREBASE_STORAGE_BUCKET="your-project.appspot.com"
NEXT_PUBLIC_FIREBASE_MESSAGING_SENDER_ID="..."
NEXT_PUBLIC_FIREBASE_APP_ID="..."
```

**3. Run locally**

```bash
npm run dev
```

Open [http://localhost:3000](http://localhost:3000).

Login to your account (same that you created at Firebase Authentication, same as login credentials in the ESP8266). After logging in, add a light using the same DEVICE_ID string you put in secrets.h.

---

## Part 4 — Deploying to Vercel

**1. Push your repository to GitHub** if you have not already.

**2. Import project on Vercel**

- Go to [vercel.com](https://vercel.com) and sign in
- New Project > Import your repository
- When asked for the root directory, select `webapp` — this is important since the web app is not at the repo root

**3. Add environment variables**

In the Vercel project settings, go to Settings > Environment Variables and add all the same keys from your `.env.local` file. Set them for Production, Preview, and Development.

**4. Deploy**

Click Deploy. Vercel detects Next.js automatically. The `vercel.json` inside `webapp/` sets `"framework": "nextjs"` to ensure correct build behavior.

Your app is now live at a `.vercel.app` URL.

---

## Part 5 — Installing as a Mobile App (PWA)

The web app works in any mobile browser. For an app-like experience on your phone without building an APK, use the browser's built-in install option.

**Android (Chrome)**

- Open the deployed URL in Chrome
- Tap the three-dot menu > Add to Home screen
- Tap Add

**iOS (Safari)**

- Open the deployed URL in Safari
- Tap the Share icon > Add to Home Screen
- Tap Add

The app will appear on your home screen and launch full screen without browser UI. This is the recommended approach and requires no build tools, developer accounts, or app store submissions.

If you need a native APK, tools like [PWABuilder](https://www.pwabuilder.com) can wrap the deployed URL into an APK. Enter your Vercel URL and follow the Android package steps.

---

## Adding a Device

1. Log in to the web app with your user account
2. Tap Add Light
3. Enter a display name (e.g. "Desk Strip")
4. Enter the exact `DEVICE_ID` string from your `secrets.h`
5. Save

The ESP8266 and the web app share the same device ID to locate the correct node in the database. The path used is:

```
/users/<your_uid>/devices/<DEVICE_ID>
```

---

## Effects

| Effect | Description |
|---|---|
| Solid | Static single colour |
| Rainbow | Continuous spectrum cycle |
| Breathe | Slow breathing pulse |
| Fire | Red and amber heat simulation |
| Aurora | Fluid atmospheric wave |
| Candle | Warm organic flicker |
| Cyberwave | Cyan and pink neon shift |
| Strobe | High speed flash |
| Pulse | Rhythmic brightness pulse |
| Rain | Blue drop falling effect |
| Meteor | Comet with trailing fade |
| Twinkle | Random sparkle |
| Chase | Running pixel trail |
| Party | Multi-colour flash |
| Bounce | Pixel ping-pong |

---

## Notes

- The ESP8266 uses a persistent SSE stream, not polling. The Firebase free plan (100 concurrent connections, 10 GB/month bandwidth) is more than sufficient for personal use.
- The Firebase auth token expires every hour. The firmware calls `Firebase.ready()` on every loop iteration to handle automatic token renewal transparently.
- If the ESP8266 loses WiFi or the stream drops, it will attempt to reconnect automatically every 5 seconds.
- Do not commit `secrets.h` or `.env.local` to a public repository.
