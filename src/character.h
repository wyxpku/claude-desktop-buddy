#pragma once
#include <stdint.h>
#include <M5GFX.h>

struct Palette {
  uint16_t body, bg, text, textDim, ink;
};

bool characterInit(const char* name);
bool characterLoaded();

// 0..6: sleep, idle, busy, attention, celebrate, dizzy, heart.
void characterSetState(uint8_t state);

void characterTick();
void characterInvalidate();
void characterClose();

void characterSetPeek(bool peek);
void characterRenderTo(lgfx::v1::LGFXBase* tgt, int cx, int cy);
lgfx::v1::LGFXBase* characterSetTarget(lgfx::v1::LGFXBase* tgt);

const Palette& characterPalette();

// Returns GIF bounding box on current target (only valid when GIF is open)
void characterGetRect(int* x, int* y, int* w, int* h);

// Override the centering area (0,0 = use target dimensions)
void characterSetArea(int w, int h);

// Scale factor for GIF rendering (default 1)
void characterSetScale(int scale);
