# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build & Flash

```bash
python3 -m platformio run -e m5sticks3              # build
python3 -m platformio run -e m5sticks3 -t upload     # build + flash
python3 -m platformio run -e m5sticks3 -t uploadfs   # flash filesystem only
python3 -m platformio run -e m5sticks3 -t clean      # clean build
python3 -m platformio device monitor -b 115200       # serial monitor
```

No test suite. No linter. Verify by building and flashing.

## Architecture

ESP32-S3 firmware for M5StickS3 (135×240 display). Single-threaded Arduino loop.

### Data flow

```
USB Serial / BLE (Nordic UART Service)
  → data.h (JSON line parser, _applyJson)
    → TamaState struct (session counts, entries, prompt)
      → derive() → PersonaState enum
        → character.cpp (GIF/text render) or buddy.cpp (ASCII pet render)
```

Bridge (cc-buddy-bridge Python daemon) sends heartbeat JSON lines over BLE. Each line is `\n`-delimited JSON with fields like `total`, `running`, `waiting`, `entries[]`, `prompt{}`, `tokens`. Parsed in `data.h:_applyJson()`, stored in the global `TamaState`.

### Key files

| File | Role |
|---|---|
| `main.cpp` | Loop, state machine, all UI screens, button handling, menu system |
| `data.h` | Wire protocol: JSON parser, demo/live/asleep modes, line buffering |
| `ble_bridge.cpp` | Nordic UART Service, LE Secure Connections pairing, ring-buffer TX/RX |
| `character.cpp` | GIF decode + render from LittleFS `/characters/<name>/` |
| `buddy.cpp` + `buddies/*.cpp` | 18 ASCII species, each with 7 animation functions |
| `stats.h` | NVS persistence: settings, approval stats, leveling, mood, energy |
| `xfer.h` | Base64 chunked file transfer over BLE for character packs |
| `i18n.h` | EN/ZH bilingual strings via enum IDs (`S_*`) + `T()` macro |
| `character.h` | Palette struct, public API for GIF character system |

### Persona states (derive() in main.cpp)

Priority order: `ATTENTION` (prompt pending) > `CELEBRATE` (level up) > `BUSY` (≥3 running) > `IDLE` (connected) > `SLEEP` (no connection). One-shot states (`DIZZY` from shake, `HEART` from fast approval) override temporarily.

### Display modes

`DISP_NORMAL` (HUD with transcript entries, clock overlay when idle on USB), `DISP_PET` (stats/how-to pages), `DISP_INFO` (6 info pages). Permission prompts overlay on any mode. Landscape USB charging → full-screen clock.

### i18n

`i18n.h` defines `StrID` enum (`S_XXX`) and `_STR[LANG_COUNT][S_COUNT]` arrays with designated initializers. Use `T(S_XXX)` to get current-language string. To add a string: add enum entry before `S_COUNT`, add entries in both EN and ZH arrays.

### Character system

Two modes selected by `buddyMode` flag: GIF characters from LittleFS (manifest.json + GIFs), or ASCII species from SPECIES_TABLE in buddy.cpp. `nextPet()` cycles: GIF → species 0 → ... → species 17 → GIF. Position display: GIF=1/19, species i=(i+2)/19.

### Markdown rendering (HUD)

`markMd()` converts `**bold**` → `\x03...\x04`, `` `code` `` → `\x01...\x02`, `>` → `\x05`, `` ``` `` → `\x06`, `- / *` → bullet `•`. `printColored()` renders these markers with accent colors. `wrapInto()` wraps text into 36-byte rows treating \x01-\x06 as zero-width.

### NVS persistence

All in `"buddy"` namespace via Preferences. Settings struct in stats.h with `settingsSave()`/`settingsLoad()`. Species index: `species` key (0xFF = GIF mode).
