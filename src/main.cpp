#include <cstdio>
#include <cmath>
#include "inkfox_core.hpp"

using namespace inkfox;

// =========================================================
// 🎮 कस्टम XINPUT स्ट्रक्चर (xinput.h के बिना)
// =========================================================
struct XINPUT_GAMEPAD {
    unsigned short wButtons;
    unsigned char  bLeftTrigger;
    unsigned char  bRightTrigger;
    short          sThumbLX;
    short          sThumbLY;
    short          sThumbRX;
    short          sThumbRY;
};

struct XINPUT_STATE {
    unsigned long  dwPacketNumber;
    XINPUT_GAMEPAD Gamepad;
};

// बटनों के कोड (XInput स्टैंडर्ड के अनुसार)
#define XINPUT_GAMEPAD_DPAD_UP        0x0001
#define XINPUT_GAMEPAD_DPAD_DOWN      0x0002
#define XINPUT_GAMEPAD_DPAD_LEFT      0x0004
#define XINPUT_GAMEPAD_DPAD_RIGHT     0x0008
#define XINPUT_GAMEPAD_START          0x0010
#define XINPUT_GAMEPAD_BACK           0x0020
#define XINPUT_GAMEPAD_LEFT_THUMB     0x0040
#define XINPUT_GAMEPAD_RIGHT_THUMB    0x0080
#define XINPUT_GAMEPAD_LEFT_SHOULDER  0x0100
#define XINPUT_GAMEPAD_RIGHT_SHOULDER 0x0200
#define XINPUT_GAMEPAD_A              0x1000
#define XINPUT_GAMEPAD_B              0x2000
#define XINPUT_GAMEPAD_X              0x4000
#define XINPUT_GAMEPAD_Y              0x8000

// डेडज़ोन (Joystick की संवेदनशीलता)
#define XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE  7849

// =========================================================
// 🎮 XInputGetState का स्टब (Stub) फंक्शन
// =========================================================
// फिलहाल हम इसे ऐसे ही रख रहे हैं। अगर गेम क्रैश होता है, तो
// हम इसे बाद में XSDK के असली इनपुट फंक्शन से बदल देंगे।
unsigned long XInputGetState(unsigned long dwUserIndex, XINPUT_STATE* pState) {
    // हम एक डमी स्टेट लौटा रहे हैं ताकि कोड कंपाइल हो जाए
    pState->Gamepad.wButtons = 0; // कोई बटन नहीं दबा है
    pState->Gamepad.sThumbLX = 0;
    return 0; // 0 का मतलब ERROR_SUCCESS
}
// =========================================================

static RockSpr rock(int id, float w, float h) {
  RockSpr r; r.id = id; r.w = w; r.h = h; 
  r.top.assign(16, 0.10f); r.bot.assign(16, 0.95f); 
  return r;
}

int main() {
  Sprites s;
  s.rocks = { rock(0, 6, 2.5f), rock(1, 5, 2.2f), rock(2, 2.2f, 4.f), rock(3, 3.f, 3.f) };
  s.strokes = { StrokeSpr{0, 4, 1, .1f, .9f, .5f} };
  
  Game g(std::move(s));
  
  printf("Game Started. Waiting for controller input...\n");

  float dt = 1.f / 60;

  while (g.mode != Mode::Over) {
    XINPUT_STATE state;
    XInputGetState(0, &state); // हमारा कस्टम फंक्शन कॉल होगा

    // --- BUTTON MAPPING ---
    g.input.jumpPressed = (state.Gamepad.wButtons & XINPUT_GAMEPAD_A) != 0;
    g.input.jump = g.input.jumpPressed; 

    g.input.paintPressed = (state.Gamepad.wButtons & XINPUT_GAMEPAD_X) != 0;
    g.input.paint = g.input.paintPressed;

    g.input.down = (state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_DOWN) != 0;

    if ((state.Gamepad.wButtons & XINPUT_GAMEPAD_START) != 0) {
        g.input.start = true;
    }

    float stickX = (float)state.Gamepad.sThumbLX;
    if (abs(stickX) > XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE) {
        g.input.speedAxis = stickX / 32767.0f;
    } else {
        g.input.speedAxis = 0.0f;
    }

    g.tick(dt);
    g.events.clear();
  }

  printf("Game Over! Mode: %d\n", int(g.mode));
  return 0;
}
