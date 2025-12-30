/***** ESP32-S3 Voting UI + R307 Fingerprint (2x3 grid, flag backdrop, white modals) *****
 * - Display: ILI9341 (SPI)
 * - Touch  : XPT2046 (SPI)
 * - Finger : R307 @ 57600 on Serial1 (GPIO16 RX, GPIO17 TX)
 * - Rules  : Vote increments only when a valid fingerprint matches templates in sensor DB
 *
 * Notes:
 *  - Modals (Authenticate / Vote Recorded / No Match) are WHITE with colored border+text.
 *  - Behind the 6 white party buttons is an Indian-flag themed background.
 ***************************************************************************************/

#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <XPT2046_Touchscreen.h>
#include <HardwareSerial.h>
#include <Adafruit_Fingerprint.h>

// ----------------------- CONFIG -----------------------
#define USE_LOGOS 0  // set 1 if you paste RGB565 logo arrays below

// TFT pins (adjust if needed)
#define TFT_CS   10
#define TFT_DC    8
#define TFT_RST   9

// Touch pins
#define TOUCH_CS  4
#define TOUCH_IRQ 5

// Fingerprint UART pins (ESP32-S3)
#define FP_RX 16   // sensor TX -> ESP32 RX
#define FP_TX 17   // sensor RX <- ESP32 TX
#define FP_BAUD 57600

// Touch mapping (calibrate if needed)
#define TS_X_MIN  290
#define TS_X_MAX 3732
#define TS_Y_MIN  421
#define TS_Y_MAX 3850

#define LED_G_PIN   18
#define LED_R_PIN   21

// Grid layout
const int GRID_COLS = 2;
const int GRID_ROWS = 3;
const int CARD_W = 110;
const int CARD_H = 55;
const int CARD_RX = 10;         // corner radius
const int GRID_GAP_X = 18;
const int GRID_GAP_Y = 16;
const int GRID_TOP = 70;        // top y for grid
const int GRID_SIDE = 15;       // side margin

// Modal card size
const int MODAL_W = 220;
const int MODAL_H = 110;

// Optional logos (50x50)
const int logoW = 50;
const int logoH = 50;

#if USE_LOGOS
// --- paste your 50x50 RGB565 arrays here and rename to logo_* as needed ---
const uint16_t logo_bjp[] PROGMEM = { 0xFFFF };
const uint16_t logo_inc[] PROGMEM = { 0xFFFF };
const uint16_t logo_aap[] PROGMEM = { 0xFFFF };
const uint16_t logo_sp[]  PROGMEM = { 0xFFFF };
const uint16_t logo_bsp[] PROGMEM = { 0xFFFF };
const uint16_t logo_cpi[] PROGMEM = { 0xFFFF };
#else
// tiny 1x1 placeholders so code compiles without big arrays
const uint16_t logo_bjp[] PROGMEM = { 0xFFFF };
const uint16_t logo_inc[] PROGMEM = { 0xFFFF };
const uint16_t logo_aap[] PROGMEM = { 0xFFFF };
const uint16_t logo_sp[]  PROGMEM = { 0xFFFF };
const uint16_t logo_bsp[] PROGMEM = { 0xFFFF };
const uint16_t logo_cpi[] PROGMEM = { 0xFFFF };
#endif

// ------------------- CANDIDATES (6) -------------------
const char* candidates[] = { "BJP", "INC", "AAP", "SP", "BSP", "CPI" };
const uint16_t* logos[]  = { logo_bjp, logo_inc, logo_aap, logo_sp, logo_bsp, logo_cpi };
const int NUM_CANDIDATES = sizeof(candidates)/sizeof(candidates[0]);
int voteCounts[NUM_CANDIDATES] = {0};

// ---------------- OBJECTS -----------------------------
Adafruit_ILI9341 tft(TFT_CS, TFT_DC, TFT_RST);
XPT2046_Touchscreen touch(TOUCH_CS, TOUCH_IRQ);

// ESP32-S3: use Serial1 (HardwareSerial) for fingerprint
HardwareSerial FPSerial(1);
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&FPSerial);

// ---------------- PROTOTYPES --------------------------
void drawFlagBackdrop();
void drawGrid();
void drawTitleBar();
void redrawFullUI();
void drawStatusBar(const char* msg, uint16_t color = ILI9341_CYAN);
void overlayModal(const char* title, const char* line2, uint16_t frameColor, uint16_t textColor);
bool requireFingerprintMatch(uint16_t timeout_ms = 8000);
void waitTouchRelease();
bool getTouchedPoint(int &sx, int &sy);
int  hitTestCard(int sx, int sy);

// ======================================================
//                         SETUP
// ======================================================
void setup() {
  
  Serial.begin(115200);
  delay(100);

  tft.begin();
  touch.begin();
  // LED Setup
  pinMode(LED_G_PIN, OUTPUT);
  pinMode(LED_R_PIN, OUTPUT);
  digitalWrite(LED_G_PIN, LOW);
  digitalWrite(LED_R_PIN, LOW);

  tft.setRotation(0);
  touch.setRotation(0);

  // Fingerprint
  FPSerial.begin(FP_BAUD, SERIAL_8N1, FP_RX, FP_TX);
  delay(200);
  finger.begin(FP_BAUD);
  delay(200);

  // Boot UI
  redrawFullUI();
  drawStatusBar("Init fingerprint...", ILI9341_CYAN);

  if (!finger.verifyPassword()) {
    drawStatusBar("FP ERR: verifyPassword()", ILI9341_RED);
    Serial.println("Fingerprint sensor NOT found. Check RX/TX/Power/BAUD.");
  } else {
    if (finger.getTemplateCount() == FINGERPRINT_OK) {
      char buf[64];
      snprintf(buf, sizeof(buf), "FP OK. Templates: %d", finger.templateCount);
      drawStatusBar(buf, ILI9341_GREEN);
    } else {
      drawStatusBar("FP OK. (count err)", ILI9341_ORANGE);
    }
  }
}

// ======================================================
//                          LOOP
// ======================================================
void loop() {
  int sx, sy;
  if (!getTouchedPoint(sx, sy)) return;

  // Which card?
  int idx = hitTestCard(sx, sy);
  if (idx < 0) { waitTouchRelease(); return; }

  // Show AUTH modal (white)
  overlayModal("Authenticate", "Place finger...", ILI9341_ORANGE, ILI9341_ORANGE);

  bool ok = requireFingerprintMatch(8000);
  if (ok) {
    voteCounts[idx]++;
    ledOK(); // ✅ GREEN LED pulse
    Serial.printf("Vote accepted for %s. Total=%d\n", candidates[idx], voteCounts[idx]);
    overlayModal("Vote Recorded!", candidates[idx], ILI9341_GREEN, ILI9341_GREEN);
    delay(950);
  } else {
    ledError(); // ❌ RED LED flash
    overlayModal("No Match", "Try again", ILI9341_RED, ILI9341_RED);
    delay(950);
  }

  // Return to grid
  redrawFullUI();
  drawStatusBar("Touch a party, then place finger", ILI9341_CYAN);
  waitTouchRelease();
}

// ======================================================
//                   DRAWING FUNCTIONS
// ======================================================
void drawTitleBar() {
  tft.fillRect(0, 0, tft.width(), 54, ILI9341_WHITE);
  tft.drawFastHLine(0, 54, tft.width(), ILI9341_BLACK);
  const char* title = "Select Party";
  int16_t x1,y1; uint16_t w,h;
  tft.setTextSize(3);
  tft.setTextColor(ILI9341_BLACK, ILI9341_WHITE);
  tft.getTextBounds(title, 0,0, &x1,&y1,&w,&h);
  tft.setCursor((tft.width()-w)/2, 16);
  tft.print(title);
}

// Indian flag themed backdrop + simple Ashoka Chakra
void drawFlagBackdrop() {
  // Top saffron gradient
  for (int y = 54; y < 120; y++) {
    uint8_t g = map(y, 54, 120, 20, 120);
    uint16_t col = tft.color565(255, g, 0);
    tft.drawFastHLine(0, y, tft.width(), col);
  }
  // Middle white band
  tft.fillRect(0, 120, tft.width(), 60, ILI9341_WHITE);

  // Bottom green gradient
  for (int y = 180; y < 270; y++) {
    uint8_t g = map(y, 180, 270, 90, 220);
    uint16_t col = tft.color565(0, g, 30);
    tft.drawFastHLine(0, y, tft.width(), col);
  }

  // Ashoka Chakra (simple ring + spokes) centered in white band
  int cx = tft.width()/2;
  int cy = 150;
  uint16_t navy = tft.color565(0, 40, 100);
  // outer ring
  for (int r = 22; r <= 24; r++) {
    for (int a = 0; a < 360; a++) {
      float rad = a * 0.0174533f;
      int x = cx + r * cos(rad);
      int y = cy + r * sin(rad);
      tft.drawPixel(x, y, navy);
    }
  }
  // spokes
  for (int a = 0; a < 24; a++) {
    float rad = (a * (360.0f/24)) * 0.0174533f;
    int x = cx + 20 * cos(rad);
    int y = cy + 20 * sin(rad);
    tft.drawLine(cx, cy, x, y, navy);
  }
}

void drawCard(int x, int y, int w, int h, const char* label, int count, const uint16_t* bmp) {
  // white card with subtle border
  tft.fillRoundRect(x, y, w, h, CARD_RX, ILI9341_WHITE);
  tft.drawRoundRect(x, y, w, h, CARD_RX, ILI9341_BLACK);

#if USE_LOGOS
  // logo on left
  int lx = x + 10;
  int ly = y + (h - logoH)/2;
  tft.drawRGBBitmap(lx, ly, bmp, logoW, logoH);
  int tx = lx + logoW + 8;
#else
  int tx = x + 12;
#endif

  int ty = y + 16;
  // party name
  tft.setTextSize(2);
  tft.setTextColor(ILI9341_BLACK, ILI9341_WHITE);
  tft.setCursor(tx, ty);
  tft.print(label);

  // votes (right aligned)
  char buf[24];
  snprintf(buf, sizeof(buf), "%d", count);
  int16_t bx,by; uint16_t bw,bh;
  tft.getTextBounds(buf, 0,0, &bx,&by,&bw,&bh);
  tft.setCursor(x + w - bw - 10, y + h - bh - 8);
  tft.print(buf);
}

void drawGrid() {
  // compute start x to center two columns
  int totalW = (GRID_COLS * CARD_W) + (GRID_GAP_X * (GRID_COLS - 1));
  int startX = (tft.width() - totalW) / 2;
  for (int r = 0; r < GRID_ROWS; r++) {
    for (int c = 0; c < GRID_COLS; c++) {
      int idx = r * GRID_COLS + c;
      if (idx >= NUM_CANDIDATES) return;
      int x = startX + c * (CARD_W + GRID_GAP_X);
      int y = GRID_TOP + r * (CARD_H + GRID_GAP_Y);
      drawCard(x, y, CARD_W, CARD_H, candidates[idx], voteCounts[idx], logos[idx]);
    }
  }
}

void drawStatusBar(const char* msg, uint16_t color) {
  tft.fillRect(0, 270, tft.width(), 50, ILI9341_BLACK);
  tft.setCursor(6, 285);
  tft.setTextColor(color, ILI9341_BLACK);
  tft.setTextSize(2);
  tft.print(msg);
}

void redrawFullUI() {
  tft.fillScreen(ILI9341_BLACK);
  drawTitleBar();
  drawFlagBackdrop();
  drawGrid();
  drawStatusBar("Touch a party, then place finger", ILI9341_CYAN);
}

// ------- WHITE MODAL (always on top, redraws grid first) -------
void overlayModal(const char* title, const char* line2, uint16_t frameColor, uint16_t textColor) {
  // Repaint background so modal is always visually on top and clean
  redrawFullUI();

  int bx = (tft.width() - MODAL_W) / 2;
  int by = 90; // roughly centered

  // white card
  tft.fillRoundRect(bx, by, MODAL_W, MODAL_H, 10, ILI9341_WHITE);
  tft.drawRoundRect(bx, by, MODAL_W, MODAL_H, 10, frameColor);

  // Title
  tft.setTextColor(textColor, ILI9341_WHITE);
  tft.setTextSize(2);
  int16_t x1,y1; uint16_t w,h;
  tft.getTextBounds(title, 0,0, &x1,&y1,&w,&h);
  tft.setCursor(bx + (MODAL_W - w)/2, by + 20);
  tft.print(title);

  // Line 2
  if (line2 && strlen(line2)) {
    tft.getTextBounds(line2, 0,0, &x1,&y1,&w,&h);
    tft.setCursor(bx + (MODAL_W - w)/2, by + 66);
    tft.print(line2);
  }
}

// ======================================================
//                   INPUT / FINGERPRINT
// ======================================================
bool getTouchedPoint(int &sx, int &sy) {
  if (!touch.touched()) return false;
  TS_Point p = touch.getPoint();
  sx = map(p.x, TS_X_MAX, TS_X_MIN, 0, tft.width());
  sy = map(p.y, TS_Y_MAX, TS_Y_MIN, 0, tft.height());
  return true;
}

int hitTestCard(int sx, int sy) {
  int totalW = (GRID_COLS * CARD_W) + (GRID_GAP_X * (GRID_COLS - 1));
  int startX = (tft.width() - totalW) / 2;

  for (int r = 0; r < GRID_ROWS; r++) {
    for (int c = 0; c < GRID_COLS; c++) {
      int idx = r * GRID_COLS + c;
      if (idx >= NUM_CANDIDATES) return -1;
      int x = startX + c * (CARD_W + GRID_GAP_X);
      int y = GRID_TOP + r * (CARD_H + GRID_GAP_Y);
      if (sx > x && sx < x + CARD_W && sy > y && sy < y + CARD_H) {
        return idx;
      }
    }
  }
  return -1;
}

bool requireFingerprintMatch(uint16_t timeout_ms) {
  uint32_t t0 = millis();

  while (millis() - t0 < timeout_ms) {
    int p = finger.getImage();
    if (p == FINGERPRINT_NOFINGER) { delay(10); continue; }
    else if (p == FINGERPRINT_OK) { /* got image */ }
    else if (p == FINGERPRINT_PACKETRECIEVEERR) { drawStatusBar("FP comm err", ILI9341_RED); delay(80); continue; }
    else if (p == FINGERPRINT_IMAGEFAIL) { drawStatusBar("FP image fail", ILI9341_RED); delay(80); continue; }
    else { delay(10); continue; }

    p = finger.image2Tz();
    if (p != FINGERPRINT_OK) {
      if (p == FINGERPRINT_IMAGEMESS || p == FINGERPRINT_IMAGEFAIL) {
        drawStatusBar("Bad image, try again", ILI9341_ORANGE);
      } else {
        drawStatusBar("image2Tz err", ILI9341_RED);
      }
      delay(120);
      continue;
    }

    p = finger.fingerFastSearch();
    if (p == FINGERPRINT_OK) {
      char buf[48];
      snprintf(buf, sizeof(buf), "Match ID:%d Conf:%d", finger.fingerID, finger.confidence);
      drawStatusBar(buf, ILI9341_GREEN);
      delay(200);
      return true;
    } else if (p == FINGERPRINT_NOTFOUND) {
      drawStatusBar("No match in DB", ILI9341_ORANGE);
      delay(180);
    } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
      drawStatusBar("FP comm err", ILI9341_RED);
      delay(120);
    } else {
      drawStatusBar("Search err", ILI9341_RED);
      delay(120);
    }
  }
  return false; // timeout
}

void waitTouchRelease() {
  uint32_t t0 = millis();
  while (touch.touched() && millis() - t0 < 1200) delay(10);
}

// ======================================================
//                 LED FEEDBACK FUNCTIONS
// ======================================================
void ledOK() {
  digitalWrite(LED_R_PIN, LOW);
  digitalWrite(LED_G_PIN, HIGH);
  delay(300);
  digitalWrite(LED_G_PIN, LOW);
}

void ledError() {
  digitalWrite(LED_G_PIN, LOW);
  for(int i=0;i<3;i++){
    digitalWrite(LED_R_PIN, HIGH);
    delay(120);
    digitalWrite(LED_R_PIN, LOW);
    delay(120);
  }
}

