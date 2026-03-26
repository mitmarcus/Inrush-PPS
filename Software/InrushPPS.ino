/*
  PPS + MOSFET + Full Web Dashboard (INDUSTRIAL ROBUST)
  Hardware:
  - M5Stack CoreS3 SE
  - Module13.2 PPS (10V~30V, 0~5A, 100W rated / 150W peak)
  - DFRobot Gravity MOSFET (GPIO 5)
  Features:
  - Verified voltage set / PPS ON/OFF / MOSFET ON/OFF
  - Auto I2C recovery
  - Live dashboard
  - CSV replay with variable speed playback
  - CSV persisted to LittleFS — survives reboot and PC disconnect
*/

#include <M5Unified.h>
#include <M5UnitUnified.h>
#include <M5UnitUnifiedMETER.h>
// PPS module I2C via M5.In_I2C (shared internal bus — no separate Wire)
#include <WiFi.h>
#include <WebServer.h>
#include <LittleFS.h>
#include "esp_log.h"
#include "esp_system.h"
#include "esp_task_wdt.h"
#include "index_html.h" // Web dashboard (HTML/CSS/JS)

// ─────────────────────────── Objects ──────────────────────────
WebServer server(80);
m5::unit::UnitUnified inaUnits;
m5::unit::UnitINA226_10A ina226;

// ─────────────────────────── Config ───────────────────────────
#define WIFI_SSID "YOUR_SSID"
#define WIFI_PASS "YOUR_PASSWORD"
#define CURRENT_LIMIT 5.0f   // A  (module max)
#define POWER_LIMIT_W 100.0f // W  (derate to rated; peak is 150W)
#define I2C_SPEED 400000U
#define PPS_ADDR 0x35

// PPS register map (from M5Module PPS I2C protocol)
#define PPS_REG_ENABLE 0x04
#define PPS_REG_VOUT_RB 0x08  // readback voltage (ADC)
#define PPS_REG_IOUT_RB 0x0C  // readback current (ADC)
#define PPS_REG_TEMP_RB 0x10  // readback temperature
#define PPS_REG_VOUT_SET 0x18 // set voltage
#define PPS_REG_IOUT_SET 0x1C // set current
#define VERIFY_TOL 0.05f
#define MOSFET_PIN 5
#define CSV_PATH "/data.csv"        // LittleFS path
#define REPLAY_STATE_PATH "/rstate" // replay state (power-loss recovery)
#define WIFI_TIMEOUT_MS 15000U
#define WIFI_CHECK_INTERVAL_MS 10000U // check WiFi every 10s

// ─────────────────────────── Live state ───────────────────────
bool screenOn = false; // display starts off
bool displayDirty = true;
float cmdV = 10.0f;
float readV = 0, readA = 0, readT = 0;
float outV = 0;              // estimated actual output after diode drop
float dropComp = 0;          // measured voltage drop between PPS output and load (diode + path)
float lastPpsCommandedV = 0; // actual voltage last written to PPS register (incl. dropComp)
bool ppsOK = false, ppsOn = false, mosfetOn = false;
bool inaOK = false;
float inaV = 0, inaA = 0, inaW = 0;

uint32_t ppsCmdSeq = 0;
bool ppsCmdOk = true;
char ppsCmdMsg[40] = "READY";

// ─────────────────────────── Diode-drop compensation ──────────
// Linear model calibrated from two measured points:
//   PPS 30 V → actual 29.39 V  (drop 0.61 V)
//   PPS  5 V → actual  4.52 V  (drop 0.48 V)
// drop = 0.0052 * Vpps + 0.454
float estimateOutputVoltage(float vpps)
{
  if (vpps < 0.3f)
    return 0.0f; // PPS off
  float v = vpps * 0.9948f - 0.454f;
  return (v >= 0.0f) ? v : 0.0f;
}

// ─────────────────────────── CSV replay state ──────────────────
#define MAX_CSV_ROWS 2000

struct CsvRow
{
  unsigned long tsMs;
  float voltage;
};

CsvRow csvData[MAX_CSV_ROWS];
int csvCount = 0;
int replayIdx = 0;
bool replayActive = false;
bool replayWaiting = false;
bool replayLoop = false;
float replaySpeed = 1.0f;
unsigned long replayStartWall = 0;
int replayRangeIn = -1;  // -1 = beginning
int replayRangeOut = -1; // -1 = end

// ──────────────────────────────────────────────────────────────
//  Logging
// ──────────────────────────────────────────────────────────────
void logI(const char *m) { Serial.printf("[INFO]  %s\n", m); }
void logE(const char *m) { Serial.printf("[ERROR] %s\n", m); }

void setPpsCommandMessage(const char *msg)
{
  strncpy(ppsCmdMsg, msg, sizeof(ppsCmdMsg) - 1);
  ppsCmdMsg[sizeof(ppsCmdMsg) - 1] = '\0';
  displayDirty = true;
}

// ──────────────────────────────────────────────────────────────
//  Safe voltage: clamp to module spec and power limit
// ──────────────────────────────────────────────────────────────
float safeVoltage(float v)
{
  if (v < 10.0f)
    v = 10.0f;
  if (v > 30.0f)
    v = 30.0f;
  return v;
}

// Clamp current so V*I never exceeds POWER_LIMIT_W
float currentForVoltage(float v)
{
  if (v < 0.1f)
    return CURRENT_LIMIT;
  float maxA = POWER_LIMIT_W / v;
  return (maxA < CURRENT_LIMIT) ? maxA : CURRENT_LIMIT;
}

// ──────────────────────────────────────────────────────────────
//  PPS I2C helpers — talk to PPS via M5.In_I2C (shared internal bus)
// ──────────────────────────────────────────────────────────────
static inline bool ppsWriteFloat(uint8_t reg, float val)
{
  union
  {
    float f;
    uint8_t b[4];
  } u;
  u.f = val;
  return M5.In_I2C.writeRegister(PPS_ADDR, reg, u.b, 4, I2C_SPEED);
}

static inline float ppsReadFloat(uint8_t reg)
{
  union
  {
    float f;
    uint8_t b[4];
  } u;
  u.f = 0;
  M5.In_I2C.readRegister(PPS_ADDR, reg, u.b, 4, I2C_SPEED);
  return u.f;
}

static inline bool ppsSetEnable(bool en)
{
  return M5.In_I2C.writeRegister8(PPS_ADDR, PPS_REG_ENABLE, en ? 0x01 : 0x00, I2C_SPEED);
}

static inline uint8_t ppsGetEnable()
{
  return M5.In_I2C.readRegister8(PPS_ADDR, PPS_REG_ENABLE, I2C_SPEED);
}

static inline bool ppsSetVoltage(float v) { return ppsWriteFloat(PPS_REG_VOUT_SET, v); }
static inline bool ppsSetCurrent(float a) { return ppsWriteFloat(PPS_REG_IOUT_SET, a); }
static inline float ppsGetSetVoltage() { return ppsReadFloat(PPS_REG_VOUT_SET); }
static inline float ppsGetReadbackV() { return ppsReadFloat(PPS_REG_VOUT_RB); }
static inline float ppsGetReadbackA() { return ppsReadFloat(PPS_REG_IOUT_RB); }
static inline float ppsGetTemperature() { return ppsReadFloat(PPS_REG_TEMP_RB); }

static inline bool ppsProbe()
{
  return M5.In_I2C.scanID(PPS_ADDR, I2C_SPEED);
}

// ──────────────────────────────────────────────────────────────
//  PPS initialisation
// ──────────────────────────────────────────────────────────────
bool initPPS()
{
  if (!ppsProbe())
  {
    logE("PPS not detected");
    return false;
  }
  ppsSetEnable(false);
  ppsSetCurrent(currentForVoltage(cmdV));
  ppsSetVoltage(cmdV);
  lastPpsCommandedV = cmdV;
  ppsOn = false;
  logI("PPS ready");
  return true;
}

// ──────────────────────────────────────────────────────────────
//  PPS re-probe — check if module responds on internal I2C
// ──────────────────────────────────────────────────────────────
bool probePPS()
{
  return ppsProbe();
}

// ──────────────────────────────────────────────────────────────
//  Consolidated PPS voltage helper (applies drop compensation)
// ──────────────────────────────────────────────────────────────
void setPpsVoltage(float v)
{
  if (!ppsOK)
    return;
  v = safeVoltage(v);
  float actual = safeVoltage(v + dropComp);
  ppsSetCurrent(currentForVoltage(actual));
  ppsSetVoltage(actual);
  lastPpsCommandedV = actual;
}

// ──────────────────────────────────────────────────────────────
//  Verification helpers
// ──────────────────────────────────────────────────────────────
// Register-based verification: reads the SET register (not the ADC readback)
// to confirm the I2C write actually reached the PPS module's STM32.
bool verifySetVoltage(float target)
{
  delay(10);
  float v = ppsGetSetVoltage(); // reads PSU_VOUT_SET register
  bool ok = fabsf(v - target) <= VERIFY_TOL;
  Serial.printf("[PPS] verifySetV: target=%.2f readback=%.2f %s\n", target, v, ok ? "OK" : "FAIL");
  return ok;
}

// Register-based verification: reads MODULE_ENABLE (0x04) to confirm
// the enable/disable command was received by the PPS module.
bool verifyEnable(bool expected)
{
  delay(10);
  uint8_t en = ppsGetEnable();
  bool ok = (expected ? (en == 1) : (en == 0));
  Serial.printf("[PPS] verifyEnable: expected=%s reg=0x%02X %s\n",
                expected ? "ON" : "OFF", en, ok ? "OK" : "FAIL");
  return ok;
}

bool setMosfet(bool state)
{
  digitalWrite(MOSFET_PIN, state ? HIGH : LOW);
  delay(5);
  bool ok = (digitalRead(MOSFET_PIN) == (state ? HIGH : LOW));
  if (ok)
    mosfetOn = state;
  return ok;
}

// ──────────────────────────────────────────────────────────────
//  CSV parser  (works on String in RAM or streamed from file)
// ──────────────────────────────────────────────────────────────
unsigned long isoToMs(const char *s)
{
  int Y = 0, Mo = 0, D = 0, h = 0, m = 0, sec = 0, ms = 0;
  sscanf(s, "%d-%d-%dT%d:%d:%d.%dZ", &Y, &Mo, &D, &h, &m, &sec, &ms);
  struct tm t = {};
  t.tm_year = Y - 1900;
  t.tm_mon = Mo - 1;
  t.tm_mday = D;
  t.tm_hour = h;
  t.tm_min = m;
  t.tm_sec = sec;
  time_t epoch = mktime(&t);
  return (unsigned long)epoch * 1000UL + (unsigned long)ms;
}

// Parse CSV directly with C-style ops — avoids per-row String allocations
int parseCsvString(const String &body)
{
  int count = 0;
  const char *buf = body.c_str();
  int len = body.length();
  int pos = 0;
  bool firstRow = true;
  unsigned long baseTs = 0;
  char tsField[40];

  while (pos < len && count < MAX_CSV_ROWS)
  {
    // Find end of line
    int lineEnd = pos;
    while (lineEnd < len && buf[lineEnd] != '\n' && buf[lineEnd] != '\r')
      lineEnd++;

    int lineLen = lineEnd - pos;

    // Skip blank lines
    if (lineLen == 0)
    {
      pos = lineEnd + 1;
      continue;
    }

    // Skip header rows
    if (lineLen >= 9 && (memchr(buf + pos, 'T', 9) || memchr(buf + pos, 't', 9)))
    {
      // Cheap check: look for "imestamp" after the T/t
      const char *p = buf + pos;
      bool isHeader = false;
      for (int i = 0; i < lineLen - 8; i++)
      {
        if ((p[i] == 'T' || p[i] == 't') &&
            strncmp(p + i + 1, "imestamp", 8) == 0)
        {
          isHeader = true;
          break;
        }
      }
      if (isHeader)
      {
        pos = lineEnd + 1;
        continue;
      }
    }

    // Find comma separator
    const char *linePtr = buf + pos;
    const char *comma = (const char *)memchr(linePtr, ',', lineLen);
    if (!comma)
    {
      pos = lineEnd + 1;
      continue;
    }

    // Extract timestamp field
    int tsLen = comma - linePtr;
    if (tsLen <= 0 || tsLen >= (int)sizeof(tsField))
    {
      pos = lineEnd + 1;
      continue;
    }
    memcpy(tsField, linePtr, tsLen);
    tsField[tsLen] = '\0';

    // Parse voltage with strtof (skips whitespace)
    float v = strtof(comma + 1, nullptr);

    unsigned long tsMs = isoToMs(tsField);

    if (firstRow)
    {
      baseTs = tsMs;
      firstRow = false;
    }

    csvData[count].tsMs = tsMs - baseTs;
    csvData[count].voltage = v;
    count++;

    // Advance past line ending (handle \r\n)
    pos = lineEnd;
    if (pos < len && buf[pos] == '\r')
      pos++;
    if (pos < len && buf[pos] == '\n')
      pos++;
  }
  return count;
}

// ──────────────────────────────────────────────────────────────
//  LittleFS helpers
// ──────────────────────────────────────────────────────────────
bool saveCsvToFlash(const String &body)
{
  size_t total = LittleFS.totalBytes();
  size_t used = LittleFS.usedBytes();
  Serial.printf("[FS] total=%u used=%u free=%u bodyLen=%u\n",
                (unsigned)total, (unsigned)used,
                (unsigned)(total - used), (unsigned)body.length());

  if (total == 0)
  {
    logE("FLASH WRITE FAILED: no filesystem partition — change partition scheme in Arduino IDE");
    return false;
  }
  // Account for space freed by overwriting the existing file
  size_t existingSize = 0;
  if (LittleFS.exists(CSV_PATH))
  {
    File ef = LittleFS.open(CSV_PATH, "r");
    if (ef)
    {
      existingSize = ef.size();
      ef.close();
    }
  }
  if (body.length() > (total - used + existingSize))
  {
    logE("FLASH WRITE FAILED: not enough space");
    return false;
  }

  File f = LittleFS.open(CSV_PATH, "w");
  if (!f)
  {
    logE("FLASH WRITE FAILED: cannot open file for write");
    return false;
  }
  f.print(body);
  f.close();
  logI("CSV saved to flash");
  return true;
}

// Stream-parse CSV directly from flash file — avoids loading
// the entire file into a String (which can spike heap by 60-80KB)
bool loadCsvFromFlash()
{
  if (!LittleFS.exists(CSV_PATH))
  {
    logI("No saved CSV found");
    return false;
  }
  File f = LittleFS.open(CSV_PATH, "r");
  if (!f)
  {
    logE("Cannot open saved CSV");
    return false;
  }

  int count = 0;
  bool firstRow = true;
  unsigned long baseTs = 0;
  char line[128];

  while (f.available() && count < MAX_CSV_ROWS)
  {
    int len = f.readBytesUntil('\n', line, sizeof(line) - 1);
    if (len <= 0)
      continue;
    line[len] = '\0';
    // Strip trailing \r
    if (len > 0 && line[len - 1] == '\r')
      line[--len] = '\0';
    if (len == 0)
      continue;

    // Skip header
    if (strstr(line, "imestamp"))
      continue;

    char *comma = strchr(line, ',');
    if (!comma)
      continue;

    *comma = '\0';
    float v = strtof(comma + 1, nullptr);
    unsigned long tsMs = isoToMs(line);

    if (firstRow)
    {
      baseTs = tsMs;
      firstRow = false;
    }

    csvData[count].tsMs = tsMs - baseTs;
    csvData[count].voltage = v;
    count++;
  }
  f.close();
  csvCount = count;
  Serial.printf("[CSV] Loaded %d rows from flash (streamed)\n", csvCount);
  return csvCount > 0;
}

// ── Cached flash CSV state (avoids LittleFS I/O on every status poll) ──
bool cachedCsvExists = false;
size_t cachedCsvSize = 0;

void updateCsvCache()
{
  cachedCsvExists = LittleFS.exists(CSV_PATH);
  if (cachedCsvExists)
  {
    File f = LittleFS.open(CSV_PATH, "r");
    cachedCsvSize = f ? f.size() : 0;
    if (f)
      f.close();
  }
  else
  {
    cachedCsvSize = 0;
  }
}

// ──────────────────────────────────────────────────────────────
//  Replay state persistence (survives power loss)
// ──────────────────────────────────────────────────────────────
void saveReplayState()
{
  File f = LittleFS.open(REPLAY_STATE_PATH, "w");
  if (f)
  {
    f.printf("%d\n", replayIdx);
    f.close();
  }
}

void clearReplayState()
{
  replayWaiting = false;
  if (LittleFS.exists(REPLAY_STATE_PATH))
    LittleFS.remove(REPLAY_STATE_PATH);
}

int loadReplayState()
{
  if (!LittleFS.exists(REPLAY_STATE_PATH))
    return -1;
  File f = LittleFS.open(REPLAY_STATE_PATH, "r");
  if (!f)
    return -1;
  int idx = f.parseInt();
  f.close();
  return idx;
}

// ──────────────────────────────────────────────────────────────
//  Replay tick — called every loop()
// ──────────────────────────────────────────────────────────────
void replayTick()
{
  if (!replayActive || csvCount == 0)
    return;

  int effEnd = (replayRangeOut >= 0 && replayRangeOut < csvCount) ? replayRangeOut + 1 : csvCount;
  int effStart = (replayRangeIn >= 0 && replayRangeIn < csvCount) ? replayRangeIn : 0;

  if (replayIdx >= effEnd)
  {
    if (replayLoop)
    {
      replayIdx = effStart;
      replayStartWall = millis() - (unsigned long)((float)csvData[effStart].tsMs / replaySpeed);
      logI("CSV replay looping");
      return;
    }
    replayActive = false;
    clearReplayState();
    setMosfet(false); // MOSFET off when replay finishes
    logI("CSV replay finished — MOSFET OFF");
    return;
  }

  // Two conditions must BOTH be met before advancing:
  // 1. CSV timestamp for current row has been reached (scaled by speed)
  // 2. Previous voltage has settled within 0.1V (so every point is captured)

  // Time check — has the scaled elapsed time reached this row's timestamp?
  unsigned long now = millis();
  float scaledElapsed = (float)(now - replayStartWall) * replaySpeed;
  if ((float)csvData[replayIdx].tsMs > scaledElapsed)
    return; // not time yet — wait

  // Settle check — fresh PPS readback vs the actual voltage we commanded.
  // Uses inline I2C read (~1ms) to avoid stale background data.
  // Compares against lastPpsCommandedV (includes dropComp) so the diode
  // drop doesn't interfere with the comparison.
  if (replayIdx > effStart)
  {
    float freshRb = ppsGetReadbackV();
    if (fabsf(freshRb - lastPpsCommandedV) > 0.1f)
      return; // not settled yet — try again next loop()
  }

  // Both conditions met — apply the next row
  float v = safeVoltage(csvData[replayIdx].voltage);
  setPpsVoltage(v);
  cmdV = v;

  Serial.printf("[REPLAY] idx=%d  t=%lums  V=%.2f\n",
                replayIdx, csvData[replayIdx].tsMs, v);
  replayIdx++;

  // Throttle flash writes — save at most every 500ms during replay
  static unsigned long lastSaveMs = 0;
  if (now - lastSaveMs >= 500)
  {
    lastSaveMs = now;
    saveReplayState();
  }
}

// ──────────────────────────────────────────────────────────────
//  Web handlers
// ──────────────────────────────────────────────────────────────
void handleRoot()
{
  server.send_P(200, "text/html; charset=utf-8", HTML);
}

void handleStatusLive()
{
  char json[768];
  snprintf(json, sizeof(json),
           "{\"cmdV\":%.2f,\"ppsOn\":%s,\"mosfet\":%s,"
           "\"readV\":%.2f,\"outV\":%.2f,\"readA\":%.2f,\"readW\":%.2f,\"temp\":%.1f,"
           "\"replayActive\":%s,\"replayWaiting\":%s,"
           "\"replayIdx\":%d,\"replayCount\":%d,"
           "\"replaySpeed\":%.2f,\"replayLoop\":%s,"
           "\"rangeIn\":%d,\"rangeOut\":%d,"
           "\"inaOK\":%s,\"inaV\":%.2f,\"inaA\":%.2f,\"inaW\":%.2f,"
           "\"ppsCmdOk\":%s,\"ppsCmdSeq\":%lu,\"ppsCmdMsg\":\"%s\"}",
           cmdV,
           ppsOn ? "true" : "false",
           mosfetOn ? "true" : "false",
           readV, outV, readA, readV * readA, readT,
           replayActive ? "true" : "false",
           replayWaiting ? "true" : "false",
           replayIdx, csvCount,
           replaySpeed,
           replayLoop ? "true" : "false",
           replayRangeIn, replayRangeOut,
           inaOK ? "true" : "false",
           inaV, inaA, inaW,
           ppsCmdOk ? "true" : "false",
           (unsigned long)ppsCmdSeq,
           ppsCmdMsg);
  server.send(200, "application/json", json);
}

void handleStatusMeta()
{
  char json[192];
  snprintf(json, sizeof(json),
           "{\"replayCount\":%d,\"savedFile\":%s,\"savedSize\":%u}",
           csvCount,
           cachedCsvExists ? "true" : "false",
           (unsigned)cachedCsvSize);
  server.send(200, "application/json", json);
}

void handleStatus()
{
  char json[896];
  snprintf(json, sizeof(json),
           "{\"cmdV\":%.2f,\"ppsOn\":%s,\"mosfet\":%s,"
           "\"readV\":%.2f,\"outV\":%.2f,\"readA\":%.2f,\"readW\":%.2f,\"temp\":%.1f,"
           "\"replayActive\":%s,\"replayWaiting\":%s,"
           "\"replayIdx\":%d,\"replayCount\":%d,"
           "\"replaySpeed\":%.2f,\"replayLoop\":%s,"
           "\"rangeIn\":%d,\"rangeOut\":%d,"
           "\"savedFile\":%s,\"savedSize\":%u,"
           "\"inaOK\":%s,\"inaV\":%.2f,\"inaA\":%.2f,\"inaW\":%.2f,"
           "\"ppsCmdOk\":%s,\"ppsCmdSeq\":%lu,\"ppsCmdMsg\":\"%s\"}",
           cmdV,
           ppsOn ? "true" : "false",
           mosfetOn ? "true" : "false",
           readV, outV, readA, readV * readA, readT,
           replayActive ? "true" : "false",
           replayWaiting ? "true" : "false",
           replayIdx, csvCount,
           replaySpeed,
           replayLoop ? "true" : "false",
           replayRangeIn, replayRangeOut,
           cachedCsvExists ? "true" : "false",
           (unsigned)cachedCsvSize,
           inaOK ? "true" : "false",
           inaV, inaA, inaW,
           ppsCmdOk ? "true" : "false",
           (unsigned long)ppsCmdSeq,
           ppsCmdMsg);
  server.send(200, "application/json", json);
}

void handleSet()
{
  float v = safeVoltage(server.arg("v").toFloat());
  if (replayActive)
  {
    server.send(400, "text/plain", "VOLTAGE LOCKED DURING REPLAY — STOP FIRST");
    return;
  }
  if (!ppsOK)
  {
    ppsOK = probePPS();
    if (!ppsOK)
    {
      server.send(500, "text/plain", "PPS OFFLINE");
      return;
    }
  }
  dropComp = 0;
  setPpsVoltage(v);
  if (verifySetVoltage(v))
  {
    cmdV = v;
    setPpsCommandMessage("VOLTAGE SET OK");
    ppsCmdOk = true;
    ppsCmdSeq++;
    server.send(200, "text/plain", "VOLTAGE SET OK");
  }
  else
  {
    setPpsCommandMessage("VOLTAGE SET FAILED");
    ppsCmdOk = false;
    ppsCmdSeq++;
    server.send(500, "text/plain", "VOLTAGE SET FAILED");
  }
}

void handleOn()
{
  float v = safeVoltage(server.arg("v").toFloat());
  if (!ppsOK)
  {
    ppsOK = probePPS();
    if (!ppsOK)
    {
      server.send(500, "text/plain", "PPS OFFLINE");
      return;
    }
  }
  Serial.printf("[PPS] handleOn: v=%.2f\n", v);
  dropComp = 0;
  setPpsVoltage(v);
  ppsSetEnable(true);
  if (verifyEnable(true))
  {
    cmdV = v;
    ppsOn = true;
    setPpsCommandMessage("PPS ON OK");
    ppsCmdOk = true;
    ppsCmdSeq++;
    server.send(200, "text/plain", "PPS ON OK");
    if (replayWaiting && csvCount > 0)
    {
      int effStart = (replayRangeIn >= 0 && replayRangeIn < csvCount) ? replayRangeIn : 0;
      replayIdx = min(replayIdx, csvCount);
      if (replayIdx < effStart)
        replayIdx = effStart;
      replayStartWall = millis() - (unsigned long)((float)csvData[replayIdx].tsMs / replaySpeed);
      replayActive = true;
      replayWaiting = false;
      logI("Replay resumed after power-loss recovery");
    }
  }
  else
  {
    ppsOn = false;
    setPpsCommandMessage("PPS ON FAILED");
    ppsCmdOk = false;
    ppsCmdSeq++;
    server.send(500, "text/plain", "PPS ON FAILED");
  }
}

void handleOff()
{
  ppsSetEnable(false);
  dropComp = 0;
  if (verifyEnable(false))
  {
    ppsOn = false;
    setPpsCommandMessage("PPS OFF OK");
    ppsCmdOk = true;
    ppsCmdSeq++;
    server.send(200, "text/plain", "PPS OFF OK");
  }
  else
  {
    ppsOn = false;
    setPpsCommandMessage("PPS OFF FAILED");
    ppsCmdOk = false;
    ppsCmdSeq++;
    server.send(500, "text/plain", "PPS OFF FAILED");
  }
}

void handleMosOn()
{
  if (setMosfet(true))
    server.send(200, "text/plain", "MOSFET ON OK");
  else
    server.send(500, "text/plain", "MOSFET ON FAILED");
}
void handleMosOff()
{
  if (setMosfet(false))
    server.send(200, "text/plain", "MOSFET OFF OK");
  else
    server.send(500, "text/plain", "MOSFET OFF FAILED");
}

// Upload: save to flash AND parse into RAM
void handleCsvUpload()
{
  if (!server.hasArg("plain"))
  {
    server.send(400, "text/plain", "NO BODY");
    return;
  }
  const String &body = server.arg("plain");

  // Parse into RAM first so we can validate
  int count = parseCsvString(body);
  if (count == 0)
  {
    server.send(400, "text/plain", "NO VALID ROWS PARSED");
    return;
  }
  csvCount = count;

  // Persist to flash
  if (!saveCsvToFlash(body))
  {
    server.send(500, "text/plain", "FLASH WRITE FAILED");
    return;
  }

  // Clear any stale power-loss recovery state
  replayWaiting = false;
  replayActive = false;
  replayIdx = 0;
  replayRangeIn = -1;
  replayRangeOut = -1;
  clearReplayState();

  updateCsvCache();
  char resp[80];
  snprintf(resp, sizeof(resp), "SAVED %d ROWS TO FLASH (%u BYTES)", csvCount, (unsigned)cachedCsvSize);
  server.send(200, "text/plain", resp);
}

// Download saved CSV
void handleCsvDownload()
{
  if (!LittleFS.exists(CSV_PATH))
  {
    server.send(404, "text/plain", "NO FILE");
    return;
  }
  File f = LittleFS.open(CSV_PATH, "r");
  if (!f)
  {
    server.send(500, "text/plain", "OPEN FAILED");
    return;
  }
  server.streamFile(f, "text/plain");
  f.close();
}

// Delete saved file
void handleCsvDelete()
{
  Serial.println("[CSV] Delete requested");
  replayActive = false;
  replayIdx = 0;
  csvCount = 0;
  replayRangeIn = -1;
  replayRangeOut = -1;
  bool removed = true;
  if (LittleFS.exists(CSV_PATH))
  {
    removed = LittleFS.remove(CSV_PATH);
    Serial.printf("[CSV] Remove %s: %s\n", CSV_PATH, removed ? "OK" : "FAILED");
  }
  clearReplayState();
  updateCsvCache();
  if (removed)
    server.send(200, "text/plain", "SAVED CSV DELETED");
  else
    server.send(500, "text/plain", "DELETE FAILED");
}

void handleReplayStart()
{
  if (csvCount == 0)
  {
    server.send(400, "text/plain", "NO CSV LOADED");
    return;
  }
  if (replayWaiting)
  {
    server.send(400, "text/plain", "POWER-LOSS RECOVERY PENDING — PRESS PPS ON TO RESUME");
    return;
  }
  // Auto-enable PPS if not already on
  if (!ppsOn)
  {
    if (!ppsOK)
    {
      ppsOK = probePPS();
      if (!ppsOK)
      {
        server.send(500, "text/plain", "PPS OFFLINE — CANNOT START REPLAY");
        return;
      }
    }
    setPpsVoltage(cmdV);
    ppsSetEnable(true);
    if (!verifyEnable(true))
    {
      server.send(500, "text/plain", "COULD NOT ENABLE PPS FOR REPLAY");
      return;
    }
    ppsOn = true;
    logI("PPS auto-enabled for replay");
  }
  // Auto-enable MOSFET for replay
  if (!mosfetOn)
  {
    setMosfet(true);
    logI("MOSFET auto-enabled for replay");
  }
  int effStart = (replayRangeIn >= 0 && replayRangeIn < csvCount) ? replayRangeIn : 0;
  replayIdx = effStart;
  replayStartWall = millis() - (unsigned long)((float)csvData[effStart].tsMs / replaySpeed);
  replayActive = true;
  replayWaiting = false;
  saveReplayState();
  logI("Replay started");
  server.send(200, "text/plain", "REPLAY STARTED");
}

void handleReplayPause()
{
  replayActive = !replayActive;
  if (replayActive)
  {
    unsigned long tsNow = (replayIdx > 0) ? csvData[replayIdx - 1].tsMs : 0;
    replayStartWall = millis() - (unsigned long)((float)tsNow / replaySpeed);
    server.send(200, "text/plain", "REPLAY RESUMED");
  }
  else
  {
    saveReplayState();
    server.send(200, "text/plain", "REPLAY PAUSED");
  }
}

void handleReplayStop()
{
  replayActive = false;
  replayIdx = 0;
  clearReplayState();
  setMosfet(false); // MOSFET off when replay stopped
  setPpsVoltage(cmdV);
  server.send(200, "text/plain", "REPLAY STOPPED");
}

void handleReplaySpeed()
{
  float x = server.arg("x").toFloat();
  if (x <= 0 || x > 128)
  {
    server.send(400, "text/plain", "INVALID SPEED");
    return;
  }
  if (replayActive)
  {
    unsigned long tsNow = (replayIdx > 0) ? csvData[replayIdx - 1].tsMs : 0;
    replaySpeed = x;
    replayStartWall = millis() - (unsigned long)((float)tsNow / replaySpeed);
  }
  else
  {
    replaySpeed = x;
  }
  char resp[40];
  snprintf(resp, sizeof(resp), "SPEED %.2fx", replaySpeed);
  server.send(200, "text/plain", resp);
}

void handleReplayLoop()
{
  replayLoop = !replayLoop;
  server.send(200, "text/plain", replayLoop ? "LOOP ON" : "LOOP OFF");
}

void handleReplayRange()
{
  int inVal = server.arg("in").toInt();
  int outVal = server.arg("out").toInt();
  replayRangeIn = (inVal >= 0 && inVal < csvCount) ? inVal : -1;
  replayRangeOut = (outVal >= 0 && outVal < csvCount) ? outVal : -1;
  char resp[60];
  snprintf(resp, sizeof(resp), "RANGE IN=%d OUT=%d", replayRangeIn, replayRangeOut);
  server.send(200, "text/plain", resp);
}

// ──────────────────────────────────────────────────────────────
//  Setup
// ──────────────────────────────────────────────────────────────
void setup()
{
  M5.begin(M5.config());
  Serial.begin(115200);
  delay(500);

  Serial.printf("[BOOT] Reset reason: %d\n", (int)esp_reset_reason());

  pinMode(MOSFET_PIN, OUTPUT);
  digitalWrite(MOSFET_PIN, LOW);

  // Suppress ESP-IDF I2C error spam globally — firmware handles failures itself
  esp_log_level_set("i2c.master", ESP_LOG_NONE);

  // Mount filesystem
  if (!LittleFS.begin(true))
  { // true = format if mount fails
    logE("LittleFS mount FAILED — check partition scheme (needs SPIFFS/LittleFS partition)");
    logE("Arduino IDE: Tools -> Partition Scheme -> Default 4MB with spiffs");
  }
  else if (LittleFS.totalBytes() == 0)
  {
    logE("LittleFS mounted but totalBytes=0 — wrong partition scheme, no FS partition allocated");
    logE("Arduino IDE: Tools -> Partition Scheme -> Default 4MB with spiffs");
  }
  else
  {
    Serial.printf("[FS] Mounted OK — total=%u used=%u free=%u\n",
                  (unsigned)LittleFS.totalBytes(),
                  (unsigned)LittleFS.usedBytes(),
                  (unsigned)(LittleFS.totalBytes() - LittleFS.usedBytes()));
    loadCsvFromFlash(); // auto-load saved CSV on boot
    updateCsvCache();   // cache file existence/size for status polls

    // Check for interrupted replay (power-loss recovery)
    int savedIdx = loadReplayState();
    if (savedIdx >= 0 && csvCount > 0)
    {
      replayWaiting = true;
      replayIdx = min(savedIdx, csvCount);
      Serial.printf("[REPLAY] Power-loss recovery: idx=%d/%d\n", replayIdx, csvCount);
    }
  }

  ppsOK = initPPS();

  // INA226 current/voltage meter on Port A (external Grove)
  {
    auto sda = M5.getPin(m5::pin_name_t::port_a_sda);
    auto scl = M5.getPin(m5::pin_name_t::port_a_scl);
    Serial.printf("[INA226] Port A pins: SDA=%u SCL=%u\n", sda, scl);
    Wire.begin(sda, scl, 400000U);
    Wire.setTimeOut(50);
    if (inaUnits.add(ina226, Wire) && inaUnits.begin())
    {
      inaOK = true;
      logI("INA226 ready");
    }
    else
    {
      logE("INA226 not detected — using diode-drop estimation");
    }
  }

  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  {
    unsigned long wifiStart = millis();
    while (WiFi.status() != WL_CONNECTED)
    {
      if (millis() - wifiStart > WIFI_TIMEOUT_MS)
      {
        logE("WiFi timeout — continuing without network");
        break;
      }
      delay(500);
    }
  }
  if (WiFi.status() == WL_CONNECTED)
    Serial.printf("[WIFI] IP: %s\n", WiFi.localIP().toString().c_str());

  // Screen starts off — toggle with power button
  M5.Display.setBrightness(0);
  M5.Display.fillScreen(TFT_BLACK);

  server.on("/", handleRoot);
  server.on("/status", handleStatus);
  server.on("/status/live", handleStatusLive);
  server.on("/status/meta", handleStatusMeta);
  server.on("/set", HTTP_POST, handleSet);
  server.on("/on", HTTP_POST, handleOn);
  server.on("/off", HTTP_POST, handleOff);
  server.on("/mosfet/on", HTTP_POST, handleMosOn);
  server.on("/mosfet/off", HTTP_POST, handleMosOff);
  server.on("/csv/upload", HTTP_POST, handleCsvUpload);
  server.on("/csv/download", handleCsvDownload);
  server.on("/csv/delete", HTTP_POST, handleCsvDelete);
  server.on("/replay/start", HTTP_POST, handleReplayStart);
  server.on("/replay/pause", HTTP_POST, handleReplayPause);
  server.on("/replay/stop", HTTP_POST, handleReplayStop);
  server.on("/replay/speed", HTTP_POST, handleReplaySpeed);
  server.on("/replay/loop", HTTP_POST, handleReplayLoop);
  server.on("/replay/range", HTTP_POST, handleReplayRange);

  server.begin();

  esp_task_wdt_add(NULL); // watchdog: resets device if loop() stalls
}

// ──────────────────────────────────────────────────────────────
//  Display — live status on CoreS3 screen
// ──────────────────────────────────────────────────────────────
void drawDisplayRow(int y, uint8_t textSize, uint16_t color, const char *text, int height)
{
  M5.Display.fillRect(0, y - 2, M5.Display.width(), height, TFT_BLACK);
  M5.Display.setCursor(10, y);
  M5.Display.setTextSize(textSize);
  M5.Display.setTextColor(color, TFT_BLACK);
  M5.Display.print(text);
}

void updateDisplay()
{
  if (!screenOn)
    return;

  static char line1[32] = "";
  static char line2[32] = "";
  static char line3[24] = "";
  static char line4[24] = "";
  static char line5[32] = "";
  static char line6[32] = "";
  static char line7[32] = "";
  static uint16_t color1 = 0;
  static uint16_t color3 = 0;
  static uint16_t color5 = 0;
  static uint16_t color6 = 0;
  static uint16_t color7a = 0;

  char next1[32];
  char next2[32];
  char next3[24];
  char next4[24];
  char next5[32];
  char next6[32];
  char next7[32];

  uint16_t nextColor1 = ppsOn ? TFT_GREEN : TFT_RED;
  uint16_t nextColor3 = mosfetOn ? TFT_GREEN : TFT_DARKGREY;
  uint16_t nextColor5 = replayActive ? TFT_GREEN : TFT_DARKGREY;
  uint16_t nextColor6 = (WiFi.status() == WL_CONNECTED) ? TFT_WHITE : TFT_RED;
  uint16_t nextColor7a = ppsOK ? TFT_GREEN : TFT_RED;

  snprintf(next1, sizeof(next1), "PPS %s %.2fV", ppsOn ? "ON " : "OFF", outV);
  if (inaOK)
    snprintf(next2, sizeof(next2), "%.2fA  %.1fW", inaA, inaW);
  else
    snprintf(next2, sizeof(next2), "%.2fA  %.1fW", readA, readV * readA);
  snprintf(next3, sizeof(next3), "MOSFET %s", mosfetOn ? "ON" : "OFF");
  snprintf(next4, sizeof(next4), "Temp %.0fC", readT);
  if (replayActive)
    snprintf(next5, sizeof(next5), "REPLAY %d/%d %.0fx", replayIdx, csvCount, replaySpeed);
  else if (csvCount > 0)
    snprintf(next5, sizeof(next5), "CSV %d rows", csvCount);
  else
    snprintf(next5, sizeof(next5), "No CSV");
  if (WiFi.status() == WL_CONNECTED)
    snprintf(next6, sizeof(next6), "IP %s", WiFi.localIP().toString().c_str());
  else
    snprintf(next6, sizeof(next6), "WiFi OFFLINE");
  snprintf(next7, sizeof(next7), "PPS:%s INA:%s heap:%u",
           ppsOK ? "OK" : "--",
           inaOK ? "OK" : "--",
           (unsigned)ESP.getFreeHeap());

  M5.Display.startWrite();
  if (displayDirty)
  {
    M5.Display.fillScreen(TFT_BLACK);
    M5.Display.setCursor(10, 8);
    M5.Display.setTextSize(2);
    M5.Display.setTextColor(TFT_CYAN, TFT_BLACK);
    M5.Display.print("NodeATE");
  }

  if (displayDirty || strcmp(line1, next1) != 0 || color1 != nextColor1)
  {
    drawDisplayRow(36, 2, nextColor1, next1, 18);
    strncpy(line1, next1, sizeof(line1));
    line1[sizeof(line1) - 1] = '\0';
    color1 = nextColor1;
  }
  if (displayDirty || strcmp(line2, next2) != 0)
  {
    drawDisplayRow(60, 2, TFT_WHITE, next2, 18);
    strncpy(line2, next2, sizeof(line2));
    line2[sizeof(line2) - 1] = '\0';
  }
  if (displayDirty || strcmp(line3, next3) != 0 || color3 != nextColor3)
  {
    drawDisplayRow(84, 2, nextColor3, next3, 18);
    strncpy(line3, next3, sizeof(line3));
    line3[sizeof(line3) - 1] = '\0';
    color3 = nextColor3;
  }
  if (displayDirty || strcmp(line4, next4) != 0)
  {
    drawDisplayRow(108, 2, TFT_WHITE, next4, 18);
    strncpy(line4, next4, sizeof(line4));
    line4[sizeof(line4) - 1] = '\0';
  }
  if (displayDirty || strcmp(line5, next5) != 0 || color5 != nextColor5)
  {
    drawDisplayRow(136, 2, nextColor5, next5, 18);
    strncpy(line5, next5, sizeof(line5));
    line5[sizeof(line5) - 1] = '\0';
    color5 = nextColor5;
  }
  if (displayDirty || strcmp(line6, next6) != 0 || color6 != nextColor6)
  {
    drawDisplayRow(164, 2, nextColor6, next6, 18);
    strncpy(line6, next6, sizeof(line6));
    line6[sizeof(line6) - 1] = '\0';
    color6 = nextColor6;
  }
  if (displayDirty || strcmp(line7, next7) != 0 || color7a != nextColor7a)
  {
    drawDisplayRow(196, 1, nextColor7a, next7, 12);
    strncpy(line7, next7, sizeof(line7));
    line7[sizeof(line7) - 1] = '\0';
    color7a = nextColor7a;
  }
  M5.Display.endWrite();
  displayDirty = false;
}

// ──────────────────────────────────────────────────────────────
//  WiFi reconnect
// ──────────────────────────────────────────────────────────────
void checkWiFi()
{
  static unsigned long lastCheck = 0;
  if (millis() - lastCheck < WIFI_CHECK_INTERVAL_MS)
    return;
  lastCheck = millis();
  if (WiFi.status() != WL_CONNECTED)
  {
    // Don't call disconnect()+begin() — it fights setAutoReconnect(true).
    // Just log status; the SDK reconnect state machine handles recovery.
    Serial.printf("[WIFI] Status=%d, auto-reconnect active\n", WiFi.status());
  }
}

// ──────────────────────────────────────────────────────────────
//  Loop
// ──────────────────────────────────────────────────────────────
void loop()
{
  esp_task_wdt_reset();
  M5.update();

  // Power button short press toggles screen
  if (M5.BtnPWR.wasClicked())
  {
    screenOn = !screenOn;
    M5.Display.setBrightness(screenOn ? 80 : 0);
    displayDirty = true;
    if (!screenOn)
      M5.Display.fillScreen(TFT_BLACK);
  }

  server.handleClient();
  replayTick();

  // ── Low-priority background work: stagger across 250ms ticks ──
  // Spreading I²C reads avoids a single 15-20ms blocking burst,
  // keeping the web server and replay responsive.
  static unsigned long lastBg = 0;
  static uint8_t bgPhase = 0;
  unsigned long now = millis();
  if (now - lastBg >= 250)
  {
    lastBg = now;
    switch (bgPhase & 3)
    {
    case 0: // PPS voltage + current read (or retry if offline)
      if (ppsOK)
      {
        float v = ppsGetReadbackV();
        float a = ppsGetReadbackA();

        static uint8_t i2cFailCount = 0;
        if (v == 0 && a == 0 && ppsOn)
        {
          if (++i2cFailCount >= 3)
          {
            logE("I2C lost — re-probing");
            ppsOK = probePPS();
            i2cFailCount = 0;
          }
        }
        else
        {
          i2cFailCount = 0;
          readV = (v < 0.0f) ? 0.0f : v;
          readA = (a < 0.0f) ? 0.0f : a;
          if (!inaOK)
            outV = estimateOutputVoltage(v);
        }
      }
      else
      {
        static unsigned long ppsRetryInterval = 2000;
        static unsigned long lastPpsRetry = 0;
        if (now - lastPpsRetry >= ppsRetryInterval)
        {
          lastPpsRetry = now;
          ppsOK = probePPS();
          if (ppsOK)
            ppsRetryInterval = 2000;
          else if (ppsRetryInterval < 30000)
            ppsRetryInterval *= 2;
        }
      }
      break;

    case 1: // PPS temperature read
      if (ppsOK)
      {
        float t = ppsGetTemperature();
        readT = t;
      }
      break;

    case 2: // INA226 measurement
      if (inaOK)
      {
        inaUnits.update();
        float mv = ina226.voltage();
        if (!isnan(mv))
        {
          inaV = mv / 1000.0f;
          inaA = ina226.current() / 1000.0f;
          inaW = ina226.power() / 1000.0f;
          outV = (inaV < 0.0f) ? 0.0f : inaV;

          // Closed-loop drop compensation
          if (ppsOn && inaV > cmdV * 0.5f && readV > 0.3f)
          {
            float measuredDrop = readV - inaV;
            if (measuredDrop >= 0 && measuredDrop < 3.0f)
            {
              dropComp = measuredDrop;
              setPpsVoltage(cmdV);
            }
          }
        }
      }
      break;

    case 3: // WiFi check + serial status + display (low priority)
      checkWiFi();
      updateDisplay();
      {
        static unsigned long lastLog = 0;
        if (now - lastLog >= 5000)
        {
          lastLog = now;
          Serial.printf("[STATUS] PPS:%s V=%.2f I=%.2f T=%.0f out=%.2f"
                        " | REPLAY:%s %d/%d %.0fx\n",
                        ppsOn ? "ON" : "OFF",
                        readV, readA, readT,
                        inaOK ? inaV : outV,
                        replayActive ? "ON" : "OFF",
                        replayIdx, csvCount, replaySpeed);
        }
      }
      break;
    }
    bgPhase++;

    // Give web server another chance after I²C work
    server.handleClient();
  }
}
