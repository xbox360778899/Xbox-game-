// Ink Fox - portable game core (C++17, no rendering, no platform calls).
// Ported from the Three.js version (index.html). Works on PC (SDL2) and can be
// reused as-is inside an xsdk (Xbox 360) project. Avoid <thread>/<chrono>/iostream here.
#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <memory>
#include <vector>

namespace inkfox {

constexpr float PI = 3.14159265358979f;
inline float clampf(float v, float a, float b) { return std::max(a, std::min(b, v)); }
inline float lerpf(float a, float b, float t) { return a + (b - a) * t; }
inline float dampf(float a, float b, float k, float dt) { return lerpf(a, b, 1.f - std::exp(-k * dt)); }
inline float rad(float deg) { return deg * PI / 180.f; }
constexpr float NONE = -1e30f;               // "null" surface
inline bool has(float v) { return v > -1e29f; }

struct Cfg {
  float gUp = 27, gUpRelease = 64, gDown = 40, jumpV = 11.6f, maxFall = 24;
  float speed0 = 6.3f, speedStep = 0.45f, speedMax = 10.2f;
  float coyote = 0.12f, jumpBuffer = 0.14f;
  float inkMax = 100, inkPerUnit = 4.6f, inkMinStart = 8, strokeGrow = 18, strokeMax = 9.5f, strokeMin = 2.8f;
  float strokeLife = 4.4f, strokeFade = 1.0f, strokeT = 0.82f, dropInk = 12;
  int lives = 3;
};
inline const Cfg CFG;

struct RNG {                                   // mulberry32, identical sequence to the JS version
  uint32_t s;
  explicit RNG(uint32_t seed = 1) : s(seed) {}
  float next() {
    s += 0x6D2B79F5u;
    uint32_t t = s;
    t = (t ^ (t >> 15)) * (t | 1u);
    t ^= t + (t ^ (t >> 7)) * (t | 61u);
    return float((t ^ (t >> 14))) / 4294967296.f;
  }
  float range(float a, float b) { return a + (b - a) * next(); }
  int   irange(int a, int b) { return int(std::floor(range(float(a), float(b) + 1.f))); }
  bool  chance(float p) { return next() < p; }
  template <class T> const T& pick(const std::vector<T>& v) { return v[size_t(next() * v.size()) % v.size()]; }
};

// ---- sprite metadata (comes from assets/manifest.json on the web version) ----
struct RockSpr { int id = 0; float w = 1, h = 1; std::vector<float> top, bot; };   // profiles 0..1, NONE where empty
struct StrokeSpr { int id = 0; float w = 1, h = 1, headU = .1f, tailU = .9f, topV = .5f; };
struct Sprites { std::vector<RockSpr> rocks; std::vector<StrokeSpr> strokes; };

// ---- gameplay events for the renderer/audio layer ----
enum class Ev { Jump, Land, Brush, Drop, Gate, Fall, Splash };
struct Event { Ev type; float x, y, power; };

enum class Mode { Title, Play, Dying, Over };

struct Plat {
  enum Kind { Rock, Stroke } kind = Rock;
  int spriteId = 0;
  // rock
  bool flip = false; float W = 0, H = 0, yq = 0, x0q = 0, inset = 0, xr = 0;
  std::vector<float> top, bot;
  // stroke
  float a = 0, x = 0, y = 0, target = 0, len = 0, shape = 0, life = 0, fade = 0, ca = 1, ta = 0, tipX = 0, revealAt = -1e9f;
  bool player = false, growing = false, revealed = false, dead = false;
  // shared
  float x0 = 0, x1 = 0;
  float profile(const std::vector<float>& arr, float px) const {
    float u = (px - x0q) / W * float(arr.size()) - 0.5f;
    int i = int(std::floor(u)); float f = u - float(i);
    int n = int(arr.size());
    float p = arr[size_t(clampf(float(i), 0, float(n - 1)))];
    float q = arr[size_t(clampf(float(i + 1), 0, float(n - 1)))];
    if (!has(p) && !has(q)) return NONE;
    if (!has(p)) return q;
    if (!has(q)) return p;
    return lerpf(p, q, f);
  }
  float surf(float px) const {
    if (kind == Rock) {
      if (px < x0 || px > x1) return NONE;
      float t = profile(top, px); if (!has(t)) return NONE;
      return yq - t * H - inset;
    }
    if (dead || fade > 0.55f || len < 0.25f) return NONE;
    if (px < x0 || px > x1) return NONE;
    return y + (px - x) * ta;
  }
  bool span(float px, float& lo, float& hi) const {          // vertical extent of a rock (walls/ceilings)
    if (kind != Rock || px < x0q || px > xr) return false;
    float t = profile(top, px), b = profile(bot, px);
    if (!has(t) || !has(b)) return false;
    lo = yq - b * H; hi = yq - t * H; return true;
  }
  float xEnd() const { return kind == Rock ? xr : std::max(x + std::max(len, shape) * ca, x1); }
  void updateExtent() { x1 = x + len * ca * 0.9f; xr = x + std::max(len, shape) * ca; }
};

struct Drop { float x, y, t, gone = 0; bool taken = false; float yNow() const { return y + std::sin(t * 2.2f) * 0.09f; } };
struct Snap { uint32_t s; float x, y; int chapter; float since; int n; float lastPaint; };
struct Gate { float x, y; Snap snap; bool passed = false; float flash = 0; };

struct Fox {
  float x = 0, y = 0, vx = 0, vy = 0, coyote = 0, jumpBuf = 0, blocked = 0, squash = 0;
  bool grounded = true, jumpHeld = false, alive = true;
  Plat* ground = nullptr;
};

struct Input { bool jump = false, paint = false, down = false, jumpPressed = false, paintPressed = false, start = false; float speedAxis = 0; };

class Game {
 public:
  Sprites spr;
  Mode mode = Mode::Title;
  std::vector<std::unique_ptr<Plat>> plats;
  std::vector<Drop> drops;
  std::vector<Gate> gates;
  std::vector<Event> events;
  Fox fox; Input input;
  float time = 0, ink = CFG.inkMax, distance = 0, startX = 0, camX = 0, camY = 0, camTY = 0, shake = 0;
  float paintCD = 0, deadT = 0, overT = 0, aspect = 16.f / 9.f;
  int lives = CFG.lives, dropsTaken = 0, chapter = 0, bestDist = 0;
  uint32_t seed = 12345;
  bool hasCheckpoint = false; Snap checkpoint{}; int checkpointChapter = 0;
  Plat* painting = nullptr;

  explicit Game(Sprites s) : spr(std::move(s)) { newRun(); mode = Mode::Title; }

  // ------------------------------------------------------------------ main step
  void tick(float dt) {
    time += dt;
    if (mode == Mode::Title && input.start) { newRun(); mode = Mode::Play; }
    if (mode == Mode::Over && input.start && overT > 0.8f) { newRun(); mode = Mode::Play; }
    input.start = false;
    if (mode == Mode::Over) overT += dt;
    bool running = mode == Mode::Play;
    if (running) { if (input.paintPressed) startPaint(); paintCD -= dt; }
    acc_ += dt;
    while (acc_ >= FIXED) {
      acc_ -= FIXED;
      if (running) { stepFox(FIXED); updatePaint(FIXED); }
      input.jumpPressed = input.paintPressed = false;
    }
    input.jumpPressed = input.paintPressed = false;

    float floorY = lowestNear(fox.x);
    if (running) {
      for (auto& d : drops) {
        if (d.taken) continue;
        float dx = d.x - (fox.x + 0.2f), dy = d.yNow() - (fox.y + 0.6f);
        if (dx * dx + dy * dy < 0.7f) {
          d.taken = true; dropsTaken++; ink = std::min(CFG.inkMax, ink + CFG.dropInk);
          events.push_back({Ev::Drop, d.x, d.y, float(dropsTaken % 5)});
        }
      }
      for (auto& g : gates) {
        if (!g.passed && fox.x > g.x) {
          g.passed = true; g.flash = 1;
          hasCheckpoint = true; checkpoint = g.snap; checkpointChapter = g.snap.chapter;
          chapter = g.snap.chapter + 1; ink = CFG.inkMax;
          events.push_back({Ev::Gate, g.x, g.y, float(chapter)});
        }
      }
      distance = std::max(distance, fox.x - startX);
      float camBottom = camY - halfHeightAt(0);
      bool fell = fox.y < floorY - 3.4f || fox.y < camBottom - 6;
      if (!fell && !fox.grounded && fox.y < camBottom - 1.0f) {
        float ahead = fox.x + std::max(0.f, fox.vx) * 0.35f;
        fell = !has(surfaceUnder(fox.x, fox.y + 0.05f, fox.y - 9).first) && !has(surfaceUnder(ahead, fox.y + 0.05f, fox.y - 9).first);
      }
      if (fell) die();
    }
    if (mode == Mode::Dying) {
      deadT += dt;
      fox.vy -= CFG.gDown * dt; fox.y += fox.vy * dt; fox.x += fox.vx * dt * 0.5f;
      if (deadT > 0.9f) { if (lives <= 0) gameOver(); else respawn(); }
    }
    for (auto& p : plats) updatePlat(*p, dt);
    for (auto& d : drops) { d.t += dt; if (d.taken) d.gone += dt * 4; }
    for (auto& g : gates) g.flash = std::max(0.f, g.flash - dt * 1.6f);
    if (mode != Mode::Over) animateFox(dt);

    float lookAhead = 2.4f + clampf(fox.vx, 0, 10) * 0.08f;
    camX = dampf(camX, fox.x + lookAhead, 5, dt);
    float titleLift = mode == Mode::Title ? 2.3f : 0;
    if (mode != Mode::Dying) {
      if (fox.grounded) camTY = fox.y + 1.4f + titleLift;
      else {
        if (fox.y + 1.4f + titleLift > camTY + 1.2f) camTY = fox.y + 0.2f + titleLift;
        if (fox.y < camTY - 2.4f - titleLift) camTY = std::max(fox.y + 2.4f + titleLift, floorY + 0.9f + titleLift);
      }
    }
    camY = dampf(camY, camTY, fox.grounded ? 3.2f : 4.5f + std::max(0.f, -fox.vy - 8) * 0.45f, dt);
    shake = std::max(0.f, shake - dt * 1.5f);
    ensureWorld(camX);
  }

  // ------------------------------------------------------------------ camera geometry
  float camDist() const { float vh = std::max(10.4f, std::min(20.f, 12.5f / aspect)); return vh / 2 / TAN15; }
  float halfWidthAt(float z) const { return (camDist() - z) * TAN15 * aspect; }
  float halfHeightAt(float z) const { return (camDist() - z) * TAN15; }

  // ------------------------------------------------------------------ run flow
  void newRun() {
    ink = CFG.inkMax; lives = CFG.lives; dropsTaken = 0; distance = 0; chapter = 0; hasCheckpoint = false; painting = nullptr;
    seed = uint32_t((uint64_t(seed) * 16807u + 11u) % 2147483647u);
    auto st = genStart(seed, nullptr);
    placeFox(st.first, st.second); startX = st.first;
    camX = fox.x + 2.4f; camY = fox.y + 1.5f; camTY = camY;
    ensureWorld(camX);
  }
  void die() {
    if (mode != Mode::Play) return;
    mode = Mode::Dying; deadT = 0; painting = nullptr; lives--;
    events.push_back({Ev::Fall, fox.x, fox.y, 0});
  }
  void respawn() {
    std::pair<float, float> st;
    if (hasCheckpoint) { st = genStart(0, &checkpoint); chapter = checkpointChapter + 1; }
    else { st = genStart(seed, nullptr); chapter = 0; }
    placeFox(st.first, st.second);
    ink = CFG.inkMax; camX = fox.x + 2.4f; camY = fox.y + 1.5f; camTY = camY;
    ensureWorld(camX); mode = Mode::Play;
  }
  void gameOver() { mode = Mode::Over; overT = 0; int d = int(distance); if (d > bestDist) bestDist = d; }

 private:
  static constexpr float FIXED = 1.f / 120.f;
  static constexpr float TAN15 = 0.26794919f;
  float acc_ = 0;
  RNG rng_{1}, fxRng_{1234567};
  Snap gs_{1, 0, 0, 0, 0, 0, -99};              // generator state (s = rng state)
  RNG genRng_{1};

  float speedFor(int ch) const { return std::min(CFG.speedMax, CFG.speed0 + float(ch) * CFG.speedStep); }

  // ---- world building
  Plat* addRockPlat(const RockSpr& s, float x, float y, float width, bool flip, float sy) {
    auto p = std::make_unique<Plat>(); p->kind = Plat::Rock; p->spriteId = s.id; p->flip = flip;
    p->W = width; p->H = width * s.h / s.w * sy;
    p->top = s.top; p->bot = s.bot;
    if (flip) { std::reverse(p->top.begin(), p->top.end()); std::reverse(p->bot.begin(), p->bot.end()); }
    const auto& top = p->top; int n = int(top.size());
    std::vector<float> mid;
    for (int i = n / 4; i < int(std::ceil(n * 0.75f)); i++) if (has(top[i])) mid.push_back(top[i]);
    std::sort(mid.begin(), mid.end());
    float t0 = mid.empty() ? 0.05f : mid[size_t(std::floor(mid.size() * 0.3f))];
    p->inset = std::min(0.14f, p->H * 0.03f);
    p->yq = y + t0 * p->H + p->inset; p->x0q = x;
    int a = 0, b = n - 1;
    while (a < b && (!has(top[a]) || top[a] - t0 > 0.12f)) a++;
    while (b > a && (!has(top[b]) || top[b] - t0 > 0.12f)) b--;
    p->x0 = x + (a + 0.3f) / n * p->W; p->x1 = x + (b + 0.7f) / n * p->W; p->xr = x + p->W;
    plats.push_back(std::move(p));
    return plats.back().get();
  }
  Plat* addStroke(const StrokeSpr& s, float x, float y, float angDeg, float len, bool isPlayer, float revealAt, bool instant = false) {
    auto p = std::make_unique<Plat>(); p->kind = Plat::Stroke; p->spriteId = s.id;
    p->a = rad(angDeg); p->x = x; p->y = y; p->target = len; p->len = instant ? len : 0;
    p->player = isPlayer; p->growing = isPlayer; p->revealAt = revealAt; p->revealed = instant || isPlayer;
    p->shape = isPlayer ? p->len : len;
    p->ca = std::cos(p->a); p->ta = std::tan(p->a); p->tipX = x + len * p->ca; p->x0 = x + 0.04f; p->updateExtent();
    plats.push_back(std::move(p));
    return plats.back().get();
  }
  const RockSpr& rockSprFor(float width) {
    std::vector<const RockSpr*> c;
    for (auto& r : spr.rocks) { float ar = r.w / r.h; if (width > 3 ? ar > 1.35f : (ar <= 1.35f && ar > 0.8f)) c.push_back(&r); }
    if (c.empty()) return spr.rocks[0];
    return *c[size_t(genRng_.next() * c.size()) % c.size()];
  }
  const RockSpr& widestRock() { const RockSpr* b = &spr.rocks[0]; for (auto& r : spr.rocks) if (r.w / r.h > b->w / b->h) b = &r; return *b; }
  Plat* addRock(float x, float y, float width, const RockSpr* forced = nullptr) {
    const RockSpr& s = forced ? *forced : rockSprFor(width);
    bool flip = genRng_.chance(0.5f);
    float sy = genRng_.range(0.85f, 1.05f);
    return addRockPlat(s, x, y, width, flip, sy);
    // (decoration - tufts, branches, trees - is purely visual and lives in the renderer)
  }
  void addDropsArc(float x0, float y0, float x1, float y1, int n, float lift) {
    for (int i = 0; i < n; i++) {
      float t = float(i + 1) / float(n + 1);
      drops.push_back({lerpf(x0, x1, t), lerpf(y0, y1, t) + std::sin(t * PI) * lift, fxRng_.next() * 6});
    }
  }
  float jumpReach(float speed, float dy) const {
    float v0 = CFG.jumpV, tUp = v0 / CFG.gUp, hMax = v0 * v0 / (2 * CFG.gUp);
    if (dy > hMax - 0.3f) return 0;
    return speed * (tUp + std::sqrt(2 * (hMax - dy) / CFG.gDown));
  }
  Snap snapGen() { Snap s = gs_; s.s = genRng_.s; return s; }
  void genPiece() {
    RNG& r = genRng_; int ch = gs_.chapter;
    float speed = speedFor(ch);
    float gateEvery = 105.f + ch * 10.f;
    if (gs_.since > gateEvery) { genGate(); return; }
    float dy = r.range(-1.9f, 1.25f);
    if (gs_.y + dy > 3.6f) dy = -std::fabs(dy);
    if (gs_.y + dy < -3.2f) dy = std::fabs(dy) * 0.8f;
    float reach = jumpReach(speed, dy);
    bool paintGap = gs_.x > 34 && gs_.x - gs_.lastPaint > 22 && r.chance(std::min(0.16f + ch * 0.07f, 0.45f));
    float startX = gs_.x, startY = gs_.y, gap; int kind;   // 0 stroke 1 pillar 2 rock
    if (paintGap) {
      gap = std::max(reach * r.range(1.3f, 1.65f), 6.5f);
      dy = std::min(dy, 0.4f); gs_.lastPaint = gs_.x;
      addDropsArc(startX - 2.6f, startY + 0.55f, startX - 0.2f, startY + 0.55f, 2, 0);
    } else gap = r.range(1.4f, std::max(1.6f, reach * (gs_.x < 40 ? 0.5f : 0.74f)));
    float strokeChance = std::max(0.22f, 0.55f - ch * 0.06f);
    if (!paintGap && r.chance(strokeChance)) kind = 0;
    else if (!paintGap && r.chance(0.16f + ch * 0.03f)) kind = 1;
    else kind = 2;
    float nx = startX + gap, ny = startY + dy;
    if (kind == 0) {
      float len = r.range(3.8f, 6.8f), ang = r.range(-5, 15);
      Plat* s = addStroke(r.pick(spr.strokes), nx, ny, ang, len, false, nx - 6);
      addDropsArc(startX, startY + 0.6f, nx, ny + 0.6f, gap > 2.6f ? 2 : 1, 1.4f);
      float tx = nx + len * std::cos(s->a) * 0.88f, ty = ny + len * std::sin(s->a) * 0.88f;
      if (r.chance(0.6f)) addDropsArc(nx, ny + 0.6f, tx, ty + 0.6f, 2, 0.2f);
      gs_.x = tx; gs_.y = ty;
    } else if (kind == 1) {
      const RockSpr* pil = &spr.rocks[0];
      for (auto& q : spr.rocks) if (q.w / q.h < 0.6f) { pil = &q; break; }
      Plat* p = addRock(nx, ny, r.range(1.7f, 2.2f), pil);
      addDropsArc(startX, startY + 0.6f, p->x0 + 0.3f, ny + 0.6f, 2, 1.5f);
      gs_.x = p->x1; gs_.y = ny;
    } else {
      float w = r.range(3.6f, 8.5f - std::min(ch, 4) * 0.5f);
      Plat* p = addRock(nx, ny, w);
      if (!paintGap) addDropsArc(startX, startY + 0.6f, p->x0 + 0.2f, ny + 0.6f, gap > 2.4f ? 3 : 1, gap > 2.4f ? 1.6f : 0.6f);
      else addDropsArc(startX + 1, startY + 1.2f, p->x0 - 1, ny + 1.2f, 3, 0.8f);
      if (r.chance(0.45f)) addDropsArc(p->x0 + 0.5f, ny + 0.6f, p->x1 - 0.5f, ny + 0.6f, r.irange(1, 3), 0.15f);
      float s = p->surf(p->x1 - 0.05f);
      gs_.x = p->x1; gs_.y = has(s) ? s : ny;
    }
    gs_.since += gs_.x - startX; gs_.n++;
  }
  void genGate() {
    RNG& r = genRng_;
    Snap sn = snapGen();
    float nx = gs_.x + r.range(1.6f, 2.6f), ny = gs_.y + r.range(-0.6f, 0.3f);
    Plat* p = addRock(nx, ny, 11.5f, &widestRock());
    float gx = (p->x0 + p->x1) / 2, gy = p->surf(gx);
    gates.push_back({gx, gy, sn});
    addDropsArc(gs_.x, gs_.y + 0.6f, p->x0 + 0.4f, ny + 0.6f, 2, 1.2f);
    float s = p->surf(p->x1 - 0.05f);
    gs_.x = p->x1; gs_.y = has(s) ? s : ny; gs_.since = 0; gs_.chapter++; gs_.n++;
  }
  std::pair<float, float> genStart(uint32_t sd, const Snap* snap) {
    painting = nullptr; plats.clear(); drops.clear(); gates.clear();
    if (snap) {
      genRng_ = RNG(snap->s); gs_ = *snap;
      genGate();
      Gate& g = gates[0]; g.passed = true;
      gs_.chapter = snap->chapter + 1;
      return {g.x - 2.2f, g.y};
    }
    genRng_ = RNG(sd); gs_ = Snap{0, 0, 0, 0, 0, 0, -99};
    Plat* p = addRock(-9, 0, 15, &widestRock());
    float s = p->surf(p->x1 - 0.05f);
    gs_.x = p->x1; gs_.y = has(s) ? s : 0;
    float sx = p->surf(-5.5f);
    return {-5.5f, has(sx) ? sx : 0};
  }
  void ensureWorld(float cx) {
    int guard = 0;
    while (gs_.x < cx + 34 && guard++ < 20) genPiece();
    float cut = cx - 30;
    for (int i = int(plats.size()) - 1; i >= 0; i--)
      if (plats[i].get() != painting && (plats[i]->xEnd() < cut || plats[i]->dead)) plats.erase(plats.begin() + i);
    for (int i = int(drops.size()) - 1; i >= 0; i--)
      if (drops[i].x < cut || (drops[i].taken && drops[i].gone > 1)) drops.erase(drops.begin() + i);
    for (int i = int(gates.size()) - 1; i >= 0; i--)
      if (gates[i].x < cut - 20 && gates.size() > 1) gates.erase(gates.begin() + i);
  }

  // ---- physics queries
  float surfAtFeet(const Plat& p, float x) const {
    float s = p.surf(x); if (has(s)) return s;
    float a = p.surf(x - 0.34f), b = p.surf(x + 0.34f);
    if (has(a) && has(b)) return std::max(a, b);
    return has(a) ? a : b;
  }
  std::pair<float, Plat*> surfaceUnder(float x, float yMax, float yMin) const {
    float best = NONE; Plat* bp = nullptr;
    for (auto& p : plats) {
      float s = surfAtFeet(*p, x);
      if (!has(s) || s > yMax || s < yMin) continue;
      if (!has(best) || s > best) { best = s; bp = p.get(); }
    }
    return {best, bp};
  }
  bool wallAhead(float x, float y, const Plat* ignore) const {
    for (auto& p : plats) {
      if (p->kind != Plat::Rock || p.get() == ignore) continue;
      float b, t; if (!p->span(x, b, t)) continue;
      if (y < t - 0.32f && y + 0.75f > b) return true;
    }
    return false;
  }
  float ceilingAt(float x, float yHead) const {
    for (auto& p : plats) {
      if (p->kind != Plat::Rock) continue;
      float b, t; if (!p->span(x, b, t)) continue;
      if (yHead > b && yHead < t - 0.4f) return b;
    }
    return NONE;
  }
  float lowestNear(float x) const {
    float lo = 1e30f;
    for (auto& p : plats) {
      if (p->xEnd() < x - 4 || p->x0 > x + 12) continue;
      float a = p->surf(std::max(p->x0, x - 4) + 0.01f), b = p->surf(std::min(p->x1, x + 12) - 0.01f);
      if (has(a)) lo = std::min(lo, a);
      if (has(b)) lo = std::min(lo, b);
    }
    return lo > 1e29f ? fox.y - 2 : lo;
  }

  // ---- fox
  void placeFox(float x, float y) {
    fox = Fox(); fox.x = x; fox.y = y;
    auto [s, p] = surfaceUnder(x, y + 0.6f, y - 2); fox.ground = p; if (has(s)) fox.y = s;
  }
  void startPaint() {
    if (painting || paintCD > 0) return;
    if (ink < CFG.inkMinStart) return;
    int alive = 0; Plat* first = nullptr;
    for (auto& p : plats) if (p->kind == Plat::Stroke && p->player && !p->dead) { if (!first) first = p.get(); alive++; }
    if (alive >= 4) first->fade = std::max(first->fade, 0.6f);
    float ang = fox.grounded ? 7.f : 2.f;
    if (input.jump && !fox.grounded) ang = 14;
    if (input.down) ang = -13;
    float x0 = fox.x - (fox.grounded ? 0.35f : 0.75f);
    float y0 = fox.grounded ? fox.y - 0.05f : fox.y - 0.34f - std::max(0.f, -fox.vy) * 0.03f;
    const StrokeSpr& ss = spr.strokes[size_t(fxRng_.next() * spr.strokes.size()) % spr.strokes.size()];
    Plat* st = addStroke(ss, x0, y0, ang, 0.35f, true, -1e9f);
    if (!fox.grounded) { st->len = st->shape = 1.35f; st->updateExtent(); ink = std::max(0.f, ink - st->len * CFG.inkPerUnit); }
    painting = st;
    events.push_back({Ev::Brush, x0, y0, 0});
  }
  void updatePaint(float dt) {
    Plat* st = painting; if (!st) return;
    bool want = input.paint || st->len < CFG.strokeMin;
    if (want && st->len < CFG.strokeMax && ink > 0) {
      float d = CFG.strokeGrow * dt;
      st->len += d; st->updateExtent(); ink = std::max(0.f, ink - d * CFG.inkPerUnit);
    } else { st->growing = false; painting = nullptr; paintCD = 0.18f; }
  }
  void land(Plat* p, float impact) {
    fox.grounded = true; fox.ground = p; fox.vy = 0; fox.coyote = CFG.coyote;
    float power = clampf(impact / 14, 0.25f, 1.4f);
    fox.squash = std::min(0.32f, 0.1f + power * 0.18f);
    events.push_back({Ev::Land, fox.x + 0.15f, fox.y, power});
    if (impact > 15) shake = std::min(0.25f, impact * 0.008f);
  }
  void stepFox(float dt) {
    float speed = speedFor(chapter);
    float target = speed * (input.speedAxis > 0.3f ? 1.12f : input.speedAxis < -0.3f ? 0.78f : 1.f);
    if (mode != Mode::Play && mode != Mode::Title) target = 0;
    fox.vx = dampf(fox.vx, target, fox.grounded ? 7.f : 2.5f, dt);
    if (input.jumpPressed) fox.jumpBuf = CFG.jumpBuffer;
    fox.jumpBuf -= dt;
    if (fox.grounded) fox.coyote = CFG.coyote; else fox.coyote -= dt;
    if (fox.jumpBuf > 0 && fox.coyote > 0 && (mode == Mode::Play || mode == Mode::Title)) {
      fox.vy = CFG.jumpV; fox.grounded = false; fox.ground = nullptr; fox.coyote = 0; fox.jumpBuf = 0; fox.jumpHeld = true; fox.squash = -0.18f;
      events.push_back({Ev::Jump, fox.x, fox.y, 0});
    }
    if (!input.jump) fox.jumpHeld = false;

    float xPrev = fox.x, yPrev = fox.y, nx = fox.x + fox.vx * dt;
    float front = nx + 0.42f;
    const Plat* ign = fox.grounded ? fox.ground : nullptr;
    if (wallAhead(front, fox.y, ign) && fox.vx > 0) {
      float wx = front; for (int k = 0; k < 40 && wallAhead(wx, fox.y, ign); k++) wx -= 0.03f;
      nx = std::min(nx, std::max(xPrev, wx - 0.42f)); fox.vx = std::min(fox.vx, 0.4f); fox.blocked += dt;
    } else fox.blocked = 0;
    fox.x = nx;

    if (fox.grounded) {
      float cur = fox.ground ? surfAtFeet(*fox.ground, fox.x) : NONE;
      auto [best, bp] = surfaceUnder(fox.x, fox.y + 0.32f, fox.y - 0.45f);
      if (has(best) && (!has(cur) || best > cur + 0.02f)) { fox.ground = bp; fox.y = best; }
      else if (has(cur)) fox.y = cur;
      else if (has(best)) { fox.ground = bp; fox.y = best; }
      else { fox.grounded = false; fox.ground = nullptr; fox.vy = std::min(fox.vy, 0.f); }
    }
    if (!fox.grounded) {
      float g = fox.vy > 0 ? (fox.jumpHeld ? CFG.gUp : CFG.gUpRelease) : CFG.gDown;
      fox.vy = std::max(-CFG.maxFall, fox.vy - g * dt);
      fox.y += fox.vy * dt;
      if (fox.vy > 0) {
        float c = ceilingAt(fox.x, fox.y + 0.95f);
        if (has(c)) { fox.y = c - 0.96f; fox.vy = 0; }
      } else {
        Plat* hit = nullptr; float hy = -1e9f;
        for (auto& p : plats) {
          float s1 = surfAtFeet(*p, fox.x); if (!has(s1)) continue;
          float s0 = surfAtFeet(*p, xPrev); if (!has(s0)) s0 = s1;
          if (yPrev >= s0 - 0.06f && fox.y <= s1 + 0.001f && s1 > hy) { hit = p.get(); hy = s1; }
        }
        if (hit) { float impact = -fox.vy; fox.y = hy; land(hit, impact); }
      }
    }
  }
  void animateFox(float dt) { fox.squash = dampf(fox.squash, 0, 9, dt); }

  void updatePlat(Plat& p, float dt) {
    if (p.kind != Plat::Stroke) return;
    if (!p.revealed && (fox.x > p.revealAt || p.tipX < camX + halfWidthAt(-0.45f) * 0.93f - 0.4f)) {
      p.revealed = true; p.growing = true; events.push_back({Ev::Brush, p.x, p.y, 1});
    }
    if (p.growing && !p.player) {
      p.len = std::min(p.target, p.len + 21 * dt);
      if (p.len >= p.target) p.growing = false;
    }
    if (p.player) p.shape = p.len;
    if (p.player && !p.growing) {
      p.life += dt;
      if (p.life > CFG.strokeLife) p.fade = std::min(1.2f, (p.life - CFG.strokeLife) / CFG.strokeFade);
      if (p.fade >= 1.15f) p.dead = true;
    }
    p.updateExtent();
  }
};

}  // namespace inkfox
