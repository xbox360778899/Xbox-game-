#include <cstdio>
#include <cmath> // abs() ke liye
#include "inkfox_core.hpp"

// Xbox 360 Controller Header
#include <xinput.h> 

using namespace inkfox;

static RockSpr rock(int id, float w, float h) {
  RockSpr r; r.id = id; r.w = w; r.h = h; 
  r.top.assign(16, 0.10f); r.bot.assign(16, 0.95f); 
  return r;
}

int main() {
  // 1. Game Initialize Karein
  Sprites s;
  s.rocks = { rock(0, 6, 2.5f), rock(1, 5, 2.2f), rock(2, 2.2f, 4.f), rock(3, 3.f, 3.f) };
  s.strokes = { StrokeSpr{0, 4, 1, .1f, .9f, .5f} };
  
  Game g(std::move(s));
  
  // Title screen se aage badhne ke liye Start dabayein
  bool controllerConnected = false;
  printf("Game Started. Waiting for controller input...\n");

  float dt = 1.f / 60;

  // 2. Infinite Game Loop (Jab tak game over nahi hota)
  while (g.mode != Mode::Over) {
    
    // Controller ki state padhein
    XINPUT_STATE state;
    ZeroMemory(&state, sizeof(XINPUT_STATE));
    
    // Player 1 (Port 0) ka controller check karein
    if (XInputGetState(0, &state) == ERROR_SUCCESS) {
        controllerConnected = true;

        // --- BUTTON MAPPING ---
        // A Button = Jump
        g.input.jumpPressed = (state.Gamepad.wButtons & XINPUT_GAMEPAD_A) != 0;
        g.input.jump = g.input.jumpPressed; 

        // X Button = Paint (Ink Shoot karna)
        g.input.paintPressed = (state.Gamepad.wButtons & XINPUT_GAMEPAD_X) != 0;
        g.input.paint = g.input.paintPressed;

        // D-Pad Down ya Left Stick Down = Neeche dekhna/angle change
        g.input.down = (state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_DOWN) != 0;

        // Start Button = Game Start/Restart
        if ((state.Gamepad.wButtons & XINPUT_GAMEPAD_START) != 0) {
            g.input.start = true;
        }

        // Left Joystick (Speed Control)
        // 32767 max value hoti hai joystick ki
        float stickX = (float)state.Gamepad.sThumbLX;
        if (abs(stickX) > XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE) {
            g.input.speedAxis = stickX / 32767.0f; // -1.0 se 1.0 ke beech
        } else {
            g.input.speedAxis = 0.0f; // Deadzone (No input)
        }
    } else {
        // Agar controller unplugged hai, toh bot khud khelega
        auto& f = g.fox;
        bool ahead = false;
        for (auto& p : g.plats) { 
          float sfc = p->surf(f.x + 0.3f + f.vx * 0.06f); 
          if (has(sfc) && sfc < f.y + 0.8f && sfc > f.y - 1.0f) ahead = true; 
        }
        g.input.jumpPressed = f.grounded && !ahead; 
        g.input.jump = g.input.jumpPressed || (!f.grounded && f.vy > 0);
        g.input.paintPressed = !f.grounded && f.vy < -6 && g.ink > 10; 
        g.input.paint = g.input.paintPressed;
    }

    // 3. Game ko Update Karein
    g.tick(dt);
    g.events.clear();
    
    // (Optional) Yahan aap GPU par draw karne ka code likhenge
    // Render(g); 
  }

  printf("Game Over! Mode: %d\n", int(g.mode));
  return 0;
}
