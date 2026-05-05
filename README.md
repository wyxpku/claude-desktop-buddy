# claude-desktop-buddy

ESP32-S3 firmware for [M5StickS3](https://docs.m5stack.com/en/core/M5StickS3) — a desk pet that displays Claude Code session status, transcript entries, and lets you approve/deny permission prompts directly on the device.

<p align="center">
  <img src="docs/device.jpg" width="500">
</p>

## Usage

### Requirements

- M5StickS3
- [PlatformIO Core](https://docs.platformio.org/en/latest/core/installation.html)
- [cc-buddy-bridge](https://github.com/wyxpku/cc-buddy-bridge) (Python daemon, connects the stick to Claude Code via BLE)

### Flash

```bash
pio run -e m5sticks3 -t upload
```

Wipe first if upgrading from a previous flash:

```bash
pio run -e m5sticks3 -t erase && pio run -e m5sticks3 -t upload
```

### Pair

1. Start the bridge daemon — it scans for the device, pairs via LE Secure Connections (passkey shown on screen), and auto-reconnects.
2. Use Claude Code normally — session state, transcript entries, and permission prompts appear on the stick in real time.

### Controls

| | Action |
|---|---|
| **A** (front) | next screen / **approve** prompt |
| **B** (right) | scroll transcript / next page / **deny** prompt |
| **Hold A** | menu |
| **Power** (short) | toggle screen off |
| **Shake** | dizzy animation |

### Settings

Hold A → settings: brightness, sound, bluetooth (on/off + clear bonds), Wi-Fi, LED, transcript HUD, language (EN/ZH), clock rotation, pet switch (19 species), reset.

## Differences from [upstream](https://github.com/anthropics/claude-desktop-buddy)

- Target M5StickS3 (ESP32-S3) instead of M5StickC Plus (ESP32)
- Use standalone Python bridge ([cc-buddy-bridge](https://github.com/wyxpku/cc-buddy-bridge)) instead of Claude desktop app
- Chinese language support (EN/ZH switchable in settings)
- HUD renders markdown: bold, code spans, quotes, bullet lists
- Repeating alert chime for pending approval prompts
- Clawd GIF as default pet (19 species total)
- BM8563 RTC validation to prevent garbage display
- Bluetooth sub-menu with bond management
