#include <cstdio>
#include "inkfox_core.hpp"
using namespace inkfox;

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
  g.input.start = true; 
  g.tick(1.f / 60); 
  g.tick(1.f / 60);
  
  float dt = 1.f / 60; 
  int jumps = 0, paints = 0;
  
  for (int i = 0; i < 60 * 120 && g.mode != Mode::Over; i++) {
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
    jumps += g.input.jumpPressed; 
    paints += g.input.paintPressed;
    g.tick(dt);
    g.events.clear();
  }
  
  printf("X360 Build OK! mode=%d dist=%.1f drops=%d lives=%d chapter=%d plats=%zu jumps=%d paints=%d\n",
              int(g.mode), g.distance, g.dropsTaken, g.lives, g.chapter, g.plats.size(), jumps, paints);
  return 0;
}
