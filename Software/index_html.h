// ─────────────────────────────────────────────────────────────────────────────
// index_html.h  –  Web dashboard for PPS + MOSFET controller
// ─────────────────────────────────────────────────────────────────────────────
#pragma once

static const char HTML[] PROGMEM = R"HTML(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width,initial-scale=1,user-scalable=yes">
  <title>NodeATE</title>
  <link rel="icon" type="image/svg+xml" href="data:image/svg+xml,%3Csvg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 32 32'%3E%3Crect width='32' height='32' rx='6' fill='%23141618'/%3E%3Cpolygon points='18,3 10,17 15,17 14,29 22,15 17,15' fill='%23e8a000'/%3E%3C/svg%3E">
  <link href="https://fonts.googleapis.com/css2?family=Barlow+Condensed:wght@400;600;700&family=B612+Mono:wght@400;700&display=swap" rel="stylesheet">
  <style>
    @font-face {
      font-family: 'DSEG7Classic';
      src: url('https://cdn.jsdelivr.net/npm/dseg@0.46.0/fonts/DSEG7-Classic/DSEG7Classic-Bold.woff2') format('woff2');
      font-weight: 700;
      font-style: normal;
    }
  </style>
  <style>
    /* ── Design tokens ─────────────────────────────────────── */
    :root {
      --bg:        #141618;   /* near-black enclosure */
      --surface:   #1e2124;   /* panel face */
      --raised:    #252a2e;   /* inset panel / recessed area */
      --border:    #333a40;   /* panel seams */
      --border-hi: #4a545c;   /* highlighted edges */
      --text:      #c8cfd4;   /* primary readout */
      --text-dim:  #5c6870;   /* label / secondary */
      --amber:     #e8a000;   /* segment display / warning */
      --green:     #1e9e3a;   /* run / on state */
      --green-hi:  #26c44a;   /* LED lit */
      --red:       #b52020;   /* stop / fault */
      --red-hi:    #e02828;   /* LED lit */
      --blue:      #1a4a9e;   /* set / neutral action */
      --orange:    #b86000;   /* pause / caution */
      --fault:     #e8c000;   /* amber fault stripe */
    }

    /* ── Scrollbars (black) ────────────────────────────────── */
    ::-webkit-scrollbar              { width: 8px; height: 8px; }
    ::-webkit-scrollbar-track        { background: #0a0c0e; }
    ::-webkit-scrollbar-thumb        { background: #2a3038; border: 1px solid #1a1e24; }
    ::-webkit-scrollbar-thumb:hover  { background: #3a4248; }
    ::-webkit-scrollbar-corner       { background: #0a0c0e; }
    * { scrollbar-color: #2a3038 #0a0c0e; scrollbar-width: thin; }

    /* ── Reset ─────────────────────────────────────────────── */
    *, *::before, *::after { box-sizing: border-box; margin: 0; padding: 0; }
    html { font-size: 16px; }

    body {
      font-family: 'Barlow Condensed', sans-serif;
      background: var(--bg);
      color: var(--text);
      min-height: 100vh;
      padding: 10px;
      /* Subtle crosshatch — etched metal feel */
      background-image:
        repeating-linear-gradient(
          0deg, transparent, transparent 23px,
          rgba(255,255,255,0.018) 23px, rgba(255,255,255,0.018) 24px),
        repeating-linear-gradient(
          90deg, transparent, transparent 23px,
          rgba(255,255,255,0.018) 23px, rgba(255,255,255,0.018) 24px);
    }

    /* ── Header ────────────────────────────────────────────── */
    .hdr {
      background: #0a0c0e;
      border-bottom: 2px solid var(--amber);
      padding: 6px 14px;
      display: flex;
      justify-content: space-between;
      align-items: center;
      margin-bottom: 10px;
      max-width: 1100px;
      margin-left: auto;
      margin-right: auto;
    }
    .hdr-title {
      font-family: 'B612 Mono', monospace;
      font-size: 0.75rem;
      font-weight: 700;
      letter-spacing: 3px;
      color: var(--amber);
      text-transform: uppercase;
    }
    .hdr-meta {
      font-family: 'B612 Mono', monospace;
      font-size: 0.6rem;
      color: var(--text-dim);
      letter-spacing: 1px;
    }

    /* ── Resume fault banner ───────────────────────────────── */
    .fault-banner {
      display: none;
      max-width: 1100px;
      margin: 0 auto 10px;
      background: #1a1400;
      border: 2px solid var(--fault);
      border-left: 6px solid var(--fault);
      padding: 8px 12px;
      font-family: 'B612 Mono', monospace;
      font-size: 0.7rem;
      gap: 12px;
      align-items: center;
      justify-content: space-between;
    }
    .fault-banner.visible { display: flex; }
    .fault-title  { color: var(--fault); font-weight: 700; letter-spacing: 1px; font-size: 0.75rem; }
    .fault-detail { color: #a09060; margin-top: 3px; }
    .fault-pos    { color: var(--fault); white-space: nowrap; font-size: 0.8rem; }
    /* ── Connection lost banner ──────────────────────── */
    .conn-banner {
      display: none;
      max-width: 1100px;
      margin: 0 auto 10px;
      background: #1a0a0a;
      border: 2px solid var(--red-hi);
      border-left: 6px solid var(--red-hi);
      padding: 8px 12px;
      font-family: 'B612 Mono', monospace;
      font-size: 0.7rem;
      align-items: center;
      color: var(--red-hi);
      font-weight: 700;
      letter-spacing: 1px;
    }
    .conn-banner.visible { display: flex; }
    /* ── Grid ──────────────────────────────────────────────── */
    .grid {
      display: grid;
      grid-template-columns: 1fr 1fr;
      gap: 8px;
      max-width: 1100px;
      margin: auto;
    }
    .span2 { grid-column: 1 / -1; }

    /* ── Panel ─────────────────────────────────────────────── */
    .panel {
      background: var(--surface);
      border: 1px solid var(--border);
      border-top: 3px solid var(--border-hi);
    }
    .panel-hdr {
      background: #161a1e;
      border-bottom: 1px solid var(--border);
      padding: 4px 10px;
      display: flex;
      justify-content: space-between;
      align-items: center;
      font-family: 'B612 Mono', monospace;
      font-size: 0.6rem;
      font-weight: 700;
      letter-spacing: 2px;
      color: var(--text-dim);
      text-transform: uppercase;
    }
    .panel-body { padding: 10px 12px; }

    /* ── Seven-segment display ─────────────────────────────── */
    .seg-display {
      background: #0a0c08;
      border: 1px solid #0e1008;
      box-shadow: inset 0 2px 6px rgba(0,0,0,0.8);
      font-family: 'DSEG7Classic', 'B612 Mono', monospace;
      font-size: 52px;
      font-weight: 700;
      color: var(--amber);
      text-align: center;
      padding: 8px 16px;
      letter-spacing: 4px;
      line-height: 1;
      margin: 8px 0;
      position: relative;
    }
    .seg-unit { font-size: 22px; opacity: 0.55; margin-left: 4px; }

    /* ── Data rows ─────────────────────────────────────────── */
    .drow {
      display: flex;
      justify-content: space-between;
      align-items: center;
      border-bottom: 1px solid var(--border);
      padding: 5px 0;
    }
    .drow:last-child { border-bottom: none; }
    .dlabel {
      font-family: 'B612 Mono', monospace;
      font-size: 0.6rem;
      letter-spacing: 1.5px;
      text-transform: uppercase;
      color: var(--text-dim);
    }
    .dval {
      font-family: 'B612 Mono', monospace;
      font-size: 1rem;
      font-weight: 700;
      color: var(--text);
    }

    /* ── Status LED + label ────────────────────────────────── */
    .indic { display: inline-flex; align-items: center; gap: 7px; }
    .led {
      width: 11px; height: 11px;
      border-radius: 50%;
      border: 1px solid rgba(0,0,0,0.6);
      flex-shrink: 0;
      background: #222;
    }
    .led-on   { background: var(--green-hi); }
    .led-off  { background: #333; }
    .led-warn { background: var(--fault); }
    .s-on     { color: var(--green-hi); font-family:'B612 Mono',monospace; font-size:0.9rem; font-weight:700; }
    .s-off    { color: var(--red-hi);   font-family:'B612 Mono',monospace; font-size:0.9rem; font-weight:700; }
    .s-warn   { color: var(--fault);    font-family:'B612 Mono',monospace; font-size:0.9rem; font-weight:700; }

    /* ── Replay badge ──────────────────────────────────────── */
    .rbadge {
      font-family: 'B612 Mono', monospace;
      font-size: 0.6rem;
      font-weight: 700;
      letter-spacing: 2px;
      padding: 2px 7px;
      border: 1px solid var(--border);
      background: var(--raised);
      color: var(--text-dim);
      text-transform: uppercase;
    }
    .rbadge.running { background: #0e2014; border-color: var(--green); color: var(--green-hi); }
    .rbadge.waiting { background: #1a1200; border-color: var(--fault); color: var(--fault); }

    /* ── Buttons ───────────────────────────────────────────── */
    button {
      font-family: 'Barlow Condensed', sans-serif;
      font-weight: 700;
      font-size: 0.85rem;
      letter-spacing: 1px;
      text-transform: uppercase;
      padding: 7px 13px;
      cursor: pointer;
      border-radius: 0;
      /* Embossed industrial border */
      border-top:    2px solid rgba(255,255,255,0.12);
      border-left:   2px solid rgba(255,255,255,0.12);
      border-bottom: 2px solid rgba(0,0,0,0.5);
      border-right:  2px solid rgba(0,0,0,0.5);
    }
    button:active {
      border-top:    2px solid rgba(0,0,0,0.5);
      border-left:   2px solid rgba(0,0,0,0.5);
      border-bottom: 2px solid rgba(255,255,255,0.12);
      border-right:  2px solid rgba(255,255,255,0.12);
      filter: brightness(0.85);
    }
    .btn-green  { background: #1a3d22;  color: #4db86a; border-top-color:rgba(255,255,255,0.08); border-left-color:rgba(255,255,255,0.08); }
    .btn-red    { background: #3a1414;  color: #c05050; border-top-color:rgba(255,255,255,0.08); border-left-color:rgba(255,255,255,0.08); }
    .btn-blue   { background: #152038;  color: #5080c0; border-top-color:rgba(255,255,255,0.08); border-left-color:rgba(255,255,255,0.08); }
    .btn-orange { background: #2e1c08;  color: #a06828; border-top-color:rgba(255,255,255,0.08); border-left-color:rgba(255,255,255,0.08); }
    .btn-gray   { background: #1e1e1e;  color: #888;    border-top-color:rgba(255,255,255,0.08); border-left-color:rgba(255,255,255,0.08); }
    .btn-row    { display: flex; gap: 6px; flex-wrap: wrap; margin-top: 10px; }

    /* ── Number input ──────────────────────────────────────── */
    input[type=number] {
      background: #0a0c08;
      border: 1px solid var(--border);
      box-shadow: inset 0 1px 4px rgba(0,0,0,0.6);
      color: var(--amber);
      font-family: 'B612 Mono', monospace;
      font-size: 1.1rem;
      font-weight: 700;
      padding: 4px 8px;
      width: 88px;
      text-align: right;
    }
    input[type=number]::-webkit-inner-spin-button,
    input[type=number]::-webkit-outer-spin-button { -webkit-appearance: none; margin: 0; }
    input[type=number] { -moz-appearance: textfield; appearance: textfield; }
    input[type=range] {
      width: 100%;
      accent-color: var(--amber);
      cursor: pointer;
      background: transparent;
    }
    input[type=file] { display: none; }

    /* ── Upload zone ───────────────────────────────────────── */
    .upload-zone {
      display: block;
      cursor: pointer;
      background: var(--raised);
      border: 2px dashed var(--border-hi);
      color: var(--text-dim);
      padding: 14px 20px;
      text-align: center;
      font-family: 'B612 Mono', monospace;
      font-size: 0.7rem;
      letter-spacing: 1px;
      text-transform: uppercase;
      transition: border-color 0.15s, color 0.15s;
    }
    .upload-zone:hover   { border-color: var(--amber); color: var(--text); }
    .upload-zone.loaded  { border-style: solid; border-color: var(--amber); color: var(--amber); background: #141008; }

    /* ── Saved file bar ────────────────────────────────────── */
    .saved-bar {
      display: flex;
      align-items: center;
      justify-content: space-between;
      background: var(--raised);
      border: 1px solid var(--border);
      padding: 5px 10px;
      margin-bottom: 8px;
      font-family: 'B612 Mono', monospace;
      font-size: 0.65rem;
    }
    .saved-bar.has { border-color: var(--green); background: #0e1a10; }
    .saved-label   { letter-spacing: 1px; text-transform: uppercase; }
    .saved-info    { color: var(--text-dim); font-size: 0.6rem; margin-top: 2px; }

    /* ── Message line ──────────────────────────────────────── */
    .msgline {
      font-family: 'B612 Mono', monospace;
      font-size: 0.68rem;
      color: #a09040;
      background: #12100a;
      border: 1px solid #2a2610;
      padding: 3px 8px;
      min-height: 20px;
      margin-top: 7px;
      letter-spacing: 0.5px;
    }

    /* ── Progress bar ──────────────────────────────────────── */
    .prog-wrap {
      background: #0a0c08;
      border: 1px solid var(--border);
      box-shadow: inset 0 1px 3px rgba(0,0,0,0.6);
      height: 12px;
      flex: 1;
      min-width: 100px;
    }
    .prog-fill {
      height: 100%;
      background: var(--amber);
      width: 0%;
      transition: width 0.5s linear;
    }
    .prog-row  { display: flex; align-items: center; gap: 8px; margin-top: 8px; }
    .prog-lbl  { font-family: 'B612 Mono', monospace; font-size: 0.65rem; color: var(--text-dim); white-space: nowrap; }

    /* ── Tick labels ───────────────────────────────────────── */
    .ticks {
      display: flex;
      justify-content: space-between;
      font-family: 'B612 Mono', monospace;
      font-size: 0.58rem;
      color: var(--text-dim);
      margin-top: 2px;
    }
    .speed-box {
      font-family: 'B612 Mono', monospace;
      font-size: 0.85rem;
      font-weight: 700;
      color: var(--text);
      background: var(--raised);
      border: 1px solid var(--border);
      padding: 2px 8px;
      min-width: 42px;
      text-align: center;
      display: inline-block;
    }

    /* ── CSV preview table ─────────────────────────────────── */
    #csv-prev {
      margin-top: 8px;
      max-height: 220px;
      overflow-y: auto;
      border: 1px solid var(--border);
      background: var(--raised);
    }
    #csv-prev table  { width:100%; border-collapse:collapse; font-family:'B612 Mono',monospace; font-size:0.65rem; }
    #csv-prev th     { background:#0e1214; color:var(--amber); padding:4px 8px; text-align:left; position:sticky; top:0; z-index:1; letter-spacing:1px; border-bottom:1px solid var(--border); }
    #csv-prev td     { padding:3px 8px; border-bottom:1px solid var(--border); color:var(--text-dim); }
    #csv-prev tr:hover td { background:#1a1e22; color:var(--text); }
    #csv-prev tr.active td { background:#1a1400; color:var(--amber); }
    #csv-prev td.row-idx   { color:var(--text-dim); opacity:0.5; text-align:right; width:36px; }


    /* ── Replay graph ──────────────────────────────────────── */
    .graph-wrap {
      position: relative;
      background: #0a0c08;
      border: 1px solid var(--border);
      box-shadow: inset 0 2px 6px rgba(0,0,0,0.7);
      margin-top: 8px;
      width: 100%;
      height: 160px;
      cursor: crosshair;
    }
    .graph-wrap canvas { display: block; width: 100%; height: 100%; }
    .graph-tooltip {
      position: absolute;
      top: 6px; right: 8px;
      font-family: 'B612 Mono', monospace;
      font-size: 0.62rem;
      color: var(--amber);
      pointer-events: none;
      letter-spacing: 0.5px;
    }

    /* ── Divider ───────────────────────────────────────────────────── */
    hr { border:none; border-top:1px solid var(--border); margin:8px 0; }

    /* ── Responsive ────────────────────────────────────────── */
    @media (max-width:520px) {
      .grid { grid-template-columns:1fr; }
      .seg-display { font-size:38px; }
    }
  </style>
</head>
<body>

<!-- ── Header bar ───────────────────────────────────────────── -->
<div class="hdr">
  <span class="hdr-title">NodeATE</span>
  <span class="hdr-meta">UNIT-01 / 36V IN / 10-30V 100W OUT</span>
</div>

<!-- ── Power-loss resume alert ──────────────────────────────── -->
<div class="fault-banner" id="fault-banner">
  <div>
    <div class="fault-title">&#9888; REPLAY PAUSED — POWER LOSS DETECTED</div>
    <div class="fault-detail">Position restored from flash. Output is SAFE/OFF. Press PPS ON to resume.</div>
  </div>
  <div class="fault-pos" id="fault-pos"></div>
</div>

<!-- ── Connection lost alert ─────────────────────────────────── -->
<div class="conn-banner" id="conn-banner">&#9888; CONNECTION LOST — DEVICE UNREACHABLE</div>

<div class="grid">

  <!-- ── Output voltage display ──────────────────────────── -->
  <div class="panel span2">
    <div class="panel-hdr">
      <span>OUTPUT VOLTAGE</span>
      <span class="rbadge" id="rbadge">MANUAL CONTROL</span>
    </div>
    <div class="panel-body">
      <div class="seg-display" id="vdisp">--<span class="seg-unit">V</span></div>
    </div>
  </div>

  <!-- ── System status ───────────────────────────────────── -->
  <div class="panel">
    <div class="panel-hdr"><span>SYSTEM STATUS</span></div>
    <div class="panel-body">
      <div class="drow">
        <span class="dlabel">PPS OUTPUT</span>
        <span class="indic"><span class="led led-off" id="pps-led"></span><span class="s-off" id="pps">OFF</span></span>
      </div>
      <div class="drow">
        <span class="dlabel">MOSFET</span>
        <span class="indic"><span class="led led-off" id="mos-led"></span><span class="s-off" id="mos">OFF</span></span>
      </div>
      <div class="drow">
        <span class="dlabel">INA226</span>
        <span class="indic"><span class="led led-off" id="ina-led"></span><span class="s-off" id="ina-st">OFFLINE</span></span>
      </div>
      <div class="drow">
        <span class="dlabel">V OUT</span>
        <span class="dval" id="rv">--</span>
      </div>
    </div>
  </div>

  <!-- ── Measurements ────────────────────────────────────── -->
  <div class="panel">
    <div class="panel-hdr"><span>MEASUREMENTS</span></div>
    <div class="panel-body">
      <div class="drow">
        <span class="dlabel">LOAD V</span>
        <span class="dval" id="ina-v">--</span>
      </div>
      <div class="drow">
        <span class="dlabel">LOAD A</span>
        <span class="dval" id="ina-a">--</span>
      </div>
      <div class="drow">
        <span class="dlabel">LOAD W</span>
        <span class="dval" id="ina-w">--</span>
      </div>
      <div class="drow">
        <span class="dlabel">PPS TEMP</span>
        <span class="dval" id="temp">--</span>
      </div>
      <div class="drow">
        <span class="dlabel">CMD VOLTAGE</span>
        <span class="dval" id="cmd-v">--</span>
      </div>
    </div>
  </div>

  <!-- ── Voltage control ─────────────────────────────────── -->
  <div class="panel span2">
    <div class="panel-hdr"><span>VOLTAGE CONTROL</span></div>
    <div class="panel-body">
      <div style="display:flex;align-items:center;gap:12px;margin-bottom:8px">
        <input type="number" id="vnum" min="10" max="30" step="0.1" value="10"
          oninput="syncSlider(this.value)">
        <span style="font-family:'B612 Mono',monospace;font-size:0.7rem;color:var(--text-dim)">V DC</span>
        <div style="flex:1">
          <input type="range" id="slider" min="10" max="30" step="0.1" value="10"
            oninput="syncNum(this.value)">
          <div class="ticks"><span>10</span><span>15</span><span>20</span><span>25</span><span>30</span></div>
        </div>
      </div>
      <hr>
      <div style="display:flex;align-items:center;gap:12px;margin-top:10px;flex-wrap:wrap">
        <div style="display:flex;align-items:center;gap:6px">
          <span style="font-family:'B612 Mono',monospace;font-size:0.6rem;letter-spacing:1.5px;color:var(--text-dim);white-space:nowrap">PPS</span>
          <button class="btn-green" onclick="ppsOn()"  style="padding:6px 14px;font-size:0.78rem">ON</button>
          <button class="btn-red"   onclick="ppsOff()" style="padding:6px 14px;font-size:0.78rem">OFF</button>
        </div>
        <div style="width:1px;background:var(--border);align-self:stretch;flex-shrink:0"></div>
        <div style="display:flex;align-items:center;gap:6px" title="DFRobot load switch — toggle to apply/remove load">
          <span style="font-family:'B612 Mono',monospace;font-size:0.6rem;letter-spacing:1.5px;color:var(--text-dim);white-space:nowrap">MOSFET</span>
          <button class="btn-green" onclick="mosOn()"  style="padding:6px 14px;font-size:0.78rem">ON</button>
          <button class="btn-red"   onclick="mosOff()" style="padding:6px 14px;font-size:0.78rem">OFF</button>
        </div>
        <div style="width:1px;background:var(--border);align-self:stretch;flex-shrink:0"></div>
        <button class="btn-blue" onclick="setV()"
          style="flex:0 0 auto;padding:6px 14px;font-size:0.72rem;white-space:nowrap;opacity:0.7"
          title="Set voltage register without enabling output">SET</button>
      </div>
      <div class="msgline" id="msg"></div>
    </div>
  </div>

  <!-- ── CSV Replay ──────────────────────────────────────── -->
  <div class="panel span2">
    <div class="panel-hdr"><span>CSV REPLAY</span></div>
    <div class="panel-body">

      <!-- Saved file indicator -->
      <div class="saved-bar" id="saved-bar">
        <div>
          <div class="saved-label" id="saved-label">NO FILE ON DEVICE</div>
          <div class="saved-info"  id="saved-info">Upload CSV to persist to flash</div>
        </div>
        <button class="btn-red" id="del-btn" onclick="delCsv()"
          style="display:none;padding:3px 9px;font-size:0.72rem">DELETE</button>
      </div>

      <!-- Upload -->
      <label class="upload-zone" id="upload-lbl" for="csv-file">
        [ SELECT CSV FILE ]<br>
        <span style="font-size:0.6rem;opacity:0.6">TIMESTAMP, VOLTAGE (V)</span>
      </label>
      <input type="file" id="csv-file" accept=".csv,.txt" onchange="handleFile(this)">

      <details id="csv-details" style="margin-top:8px">
        <summary style="
          font-family:'B612 Mono',monospace;font-size:0.6rem;letter-spacing:2px;
          color:var(--text-dim);text-transform:uppercase;cursor:pointer;
          list-style:none;display:flex;justify-content:space-between;align-items:center;
          padding:4px 10px;background:#161a1e;border:1px solid var(--border);
          user-select:none;min-height:24px">
          <span>DATA TABLE</span>
          <span id="csv-row-count" style="font-size:0.55rem;opacity:0.5"></span>
        </summary>
        <div id="csv-prev" style="display:none"></div>
      </details>

      <!-- Replay graph -->
      <div class="graph-wrap" id="graph-wrap">
        <canvas id="graph-canvas"></canvas>
        <div class="graph-tooltip" id="graph-tip"></div>
      </div>

      <hr>

      <!-- Speed -->
      <div style="display:flex;align-items:center;gap:12px;margin-bottom:6px">
        <span style="font-family:'B612 Mono',monospace;font-size:0.6rem;letter-spacing:1px;
                     color:var(--text-dim);white-space:nowrap">SPEED</span>
        <div style="flex:1">
          <input type="range" id="speed-sl" min="-2" max="7" step="1" value="0"
            oninput="setSpeed(this.value)">
          <div class="ticks">
            <span>0.25x</span><span>0.5x</span><span>1x</span><span>2x</span>
            <span>4x</span><span>8x</span><span>16x</span><span>32x</span>
            <span>64x</span><span>128x</span>
          </div>
        </div>
        <span class="speed-box" id="speed-lbl">1x</span>
      </div>

      <!-- Progress -->
      <div class="prog-row">
        <span class="prog-lbl" id="prog-lbl">0 / 0</span>
        <div class="prog-wrap"><div class="prog-fill" id="prog-fill"></div></div>
        <span class="prog-lbl" id="prog-time"></span>
      </div>

      <!-- Controls -->
      <div class="btn-row" style="margin-top:10px">
        <button class="btn-green"  onclick="rPlay()">PLAY</button>
        <button class="btn-orange" onclick="rPause()">PAUSE</button>
        <button class="btn-red"    onclick="rStop()">STOP</button>
        <button class="btn-blue" id="loop-btn" onclick="rLoop()" style="margin-left:auto">LOOP: OFF</button>
      </div>
      <div class="msgline" id="rmsg"></div>

    </div>
  </div>

</div><!-- /grid -->

<script>
'use strict';

// ── Speed map ─────────────────────────────────────────────────
const SPD = {'-2':0.25,'-1':0.5,'0':1,'1':2,'2':4,'3':8,'4':16,'5':32,'6':64,'7':128};

// ── DOM refs (cached once) ────────────────────────────────────
const $ = id => document.getElementById(id);
const vdisp   = $('vdisp'),  vnum    = $('vnum'),   slider  = $('slider');
const ppsEl   = $('pps'),    ppsLed  = $('pps-led');
const mosEl   = $('mos'),    mosLed  = $('mos-led');
const rv      = $('rv'),     tempEl  = $('temp'), cmdVEl = $('cmd-v');
const rbadge  = $('rbadge');
const msgEl   = $('msg'),    rmsgEl  = $('rmsg');
const progFill= $('prog-fill'), progLbl = $('prog-lbl');
const savedBar= $('saved-bar'), savedLbl= $('saved-label'), savedInf= $('saved-info'), delBtn= $('del-btn');
const faultBanner= $('fault-banner'), faultPos= $('fault-pos');
const csvPrev = $('csv-prev'), uploadLbl= $('upload-lbl');
const connBanner = $('conn-banner');
const progTime = $('prog-time'), loopBtn = $('loop-btn');
const inaLed = $('ina-led'), inaSt = $('ina-st');
const inaVEl = $('ina-v'), inaAEl = $('ina-a'), inaWEl = $('ina-w');

const uiCache = Object.create(null);
const metaState = { replayCount: 0, savedFile: false, savedSize: 0 };

function setTextIfChanged(key, el, value) {
  if (uiCache[key] === value) return;
  uiCache[key] = value;
  el.textContent = value;
}

function setHTMLIfChanged(key, el, value) {
  if (uiCache[key] === value) return;
  uiCache[key] = value;
  el.innerHTML = value;
}

function setClassIfChanged(key, el, value) {
  if (uiCache[key] === value) return;
  uiCache[key] = value;
  el.className = value;
}

// ── Voltage controls ──────────────────────────────────────────
function syncNum(v)    { vnum.value = parseFloat(v).toFixed(1); }
function syncSlider(v) { slider.value = v; }
function getV()        { return parseFloat(vnum.value) || 5; }

// ── API call ──────────────────────────────────────────────────
let livePollAC = null;
let metaPollAC = null;
let livePollTimer = 0;
let metaPollTimer = 0;
let actionBusy = false;

function scheduleLivePoll(delay = 1000) {
  clearTimeout(livePollTimer);
  livePollTimer = setTimeout(() => {
    if (actionBusy) {
      scheduleLivePoll(150);
      return;
    }
    pollLive();
  }, delay);
}

function scheduleMetaPoll(delay = 5000) {
  clearTimeout(metaPollTimer);
  metaPollTimer = setTimeout(() => {
    if (actionBusy) {
      scheduleMetaPoll(250);
      return;
    }
    pollMeta();
  }, delay);
}

function api(url, target) {
  // Cancel any pending status poll so this action isn't queued behind it
  actionBusy = true;
  clearTimeout(livePollTimer);
  clearTimeout(metaPollTimer);
  if (livePollAC) { livePollAC.abort(); livePollAC = null; }
  if (metaPollAC) { metaPollAC.abort(); metaPollAC = null; }
  if (target) { target.textContent = '\u2026'; target.style.color = ''; }
  return fetch(url, {method:'POST', cache:'no-store'})
    .then(r => r.text().then(t => {
      if (!r.ok) throw new Error(t || ('HTTP ' + r.status));
      return t;
    }))
    .then(t => {
      if (target) { target.textContent = t; target.style.color = ''; }
      return t;
    })
    .catch(e => {
      if (target) { target.textContent = '\u274c ' + e.message; target.style.color = 'var(--red-hi)'; }
      throw e;
    })
    .finally(() => {
      actionBusy = false;
      scheduleLivePoll(50);
      scheduleMetaPoll(150);
    });
}

function setV()   { api('/set?v='+getV(),    msgEl); }
function ppsOn()  { api('/on?v='+getV(),     msgEl); }
function ppsOff() { api('/off',              msgEl); }
function mosOn()  {
  api('/mosfet/on', msgEl).then(() => {
    mosEl.textContent = 'ON';
    mosEl.className   = 's-on';
    mosLed.className  = 'led led-on';
  });
}
function mosOff() {
  api('/mosfet/off', msgEl).then(() => {
    mosEl.textContent = 'OFF';
    mosEl.className   = 's-off';
    mosLed.className  = 'led led-off';
  });
}
function rPlay()  { api('/replay/start',     rmsgEl); }
function rPause() { api('/replay/pause',     rmsgEl); }
function rStop()  {
  if (!confirm('Stop replay and reset position?')) return;
  api('/replay/stop', rmsgEl); setProgress(0,0); updateGraphHead(0,0);
}
function rLoop()  { api('/replay/loop',      rmsgEl); }

// ── Speed ─────────────────────────────────────────────────────
let currentSpeed = 1;
let syncedSpeed = -1;
const SPD_REV = {}; Object.entries(SPD).forEach(([k,v]) => SPD_REV[v]=k);

function setSpeed(val) {
  const m = SPD[val] || 1;
  $('speed-lbl').textContent = m + 'x';
  api('/replay/speed?x='+m, rmsgEl);
}

function syncSpeedSlider(speed) {
  if (speed === syncedSpeed) return;
  syncedSpeed = speed;
  const key = SPD_REV[speed];
  if (key !== undefined) $('speed-sl').value = key;
  $('speed-lbl').textContent = speed + 'x';
}

function fmtDuration(ms) {
  const s = Math.round(ms / 1000);
  if (s < 60) return s + 's';
  const m = Math.floor(s/60), rs = s%60;
  if (s < 3600) return m + 'm ' + (rs<10?'0':'') + rs + 's';
  const h = Math.floor(s/3600), rm = Math.floor((s%3600)/60);
  return h + 'h ' + (rm<10?'0':'') + rm + 'm';
}

// ── Progress ──────────────────────────────────────────────────
function setProgress(idx, total) {
  const progressKey = [idx, total, rangeIn, rangeOut, currentSpeed, graphRows.length].join('|');
  if (setProgress._key === progressKey) return;
  setProgress._key = progressKey;
  const effOut = rangeOut >= 0 ? rangeOut : total - 1;
  if (rangeIn >= 0 || rangeOut >= 0) {
    const rangeStart = rangeIn  >= 0 ? (rangeIn+1)  : 1;
    const rangeEnd   = rangeOut >= 0 ? (rangeOut+1) : total;
    progLbl.textContent = idx + ' / ' + rangeStart + '\u2013' + rangeEnd;
  } else {
    progLbl.textContent = idx + ' / ' + total;
  }
  // Progress bar: within range if set, else within total
  const barStart = rangeIn  >= 0 ? rangeIn  : 0;
  const barEnd   = rangeOut >= 0 ? (rangeOut+1) : total;
  const barSpan  = barEnd - barStart;
  const barPos   = Math.max(0, idx - barStart);
  progFill.style.width = barSpan > 0 ? (Math.min(barPos/barSpan,1)*100).toFixed(1)+'%' : '0%';
  if (graphRows.length > 1 && idx > 0 && idx <= graphRows.length) {
    const elapsed  = graphRows[Math.min(idx, graphRows.length) - 1].ms;
    const endMs    = (effOut >= 0 && effOut < graphRows.length) ? graphRows[effOut].ms : graphRows[graphRows.length - 1].ms;
    const remain   = Math.max(0, endMs - elapsed);
    const wallRem  = currentSpeed > 0 ? remain / currentSpeed : remain;
    progTime.textContent = 'ETA\u00a0' + fmtDuration(wallRem);
  } else {
    progTime.textContent = '';
  }
}

// ── CSV local parse & upload ──────────────────────────────────
let localRows = 0;

function handleFile(inp) {
  const f = inp.files[0]; if (!f) return;
  uploadLbl.textContent = f.name.toUpperCase();
  uploadLbl.classList.add('loaded');
  const rd = new FileReader();
  rd.onload = e => { localRows = parseAndPreview(e.target.result); uploadCSV(e.target.result); };
  rd.readAsText(f);
}

function parseAndPreview(text) {
  const lines = text.split(/\r?\n/);
  let base = null, rows = [], count = 0;
  for (const raw of lines) {
    const line = raw.trim();
    if (!line || /timestamp/i.test(line)) continue;
    const c = line.indexOf(','); if (c<0) continue;
    const v  = parseFloat(line.slice(c+1).trim());
    const ms = new Date(line.slice(0,c).trim()).getTime();
    if (isNaN(v)||isNaN(ms)) continue;
    if (base===null) base=ms;
    rows.push({ms:ms-base, v}); count++;
  }
  // Preview table
  if (!rows.length) { csvPrev.style.display='none'; $('csv-row-count').textContent=''; return 0; }
  csvPrev.style.display='block';
  $('csv-row-count').textContent = rows.length + ' ROWS';
  let h = '<table><tr><th>#</th><th>TIME (s)</th><th>VOLTAGE (V)</th></tr>';
  for (let i=0; i<rows.length; i++) {
    const r = rows[i];
    h += '<tr data-idx="'+i+'">';
    h += '<td class="row-idx">'+(i+1)+'</td>';
    h += '<td>'+(r.ms/1000).toFixed(2)+'</td>';
    h += '<td>'+r.v.toFixed(2)+'</td></tr>';
  }
  csvPrev.innerHTML = h+'</table>';
  setGraphData(rows);
  setProgress(0, count);
  return count;
}

function uploadCSV(text) {
  rmsgEl.textContent = '\u2026 WRITING TO FLASH';
  uploadLbl.style.opacity = '0.5';
  fetch('/csv/upload',{method:'POST',headers:{'Content-Type':'text/plain'},body:text})
    .then(r=>{ if(!r.ok) return r.text().then(t=>{throw new Error(t||'HTTP '+r.status);}); return r.text(); })
    .then(t=>{ rmsgEl.textContent=t; pollLive(); pollMeta(); })
    .catch(e=>{ rmsgEl.textContent='\u274c UPLOAD FAILED: '+e.message; rmsgEl.style.color='var(--red-hi)'; })
    .finally(()=>{ uploadLbl.style.opacity='1'; });
}

function delCsv() {
  if (!confirm('Delete saved CSV and state from flash?')) return;
  rmsgEl.textContent = 'DELETING...';
  fetch('/csv/delete',{method:'POST'})
    .then(r=>r.text()).then(t=>{
    rmsgEl.textContent=t;
    localRows=0; setProgress(0,0); graphRows=[]; drawGraph();
    csvPrev.style.display='none';
    uploadLbl.innerHTML='[ SELECT CSV FILE ]<br><span style="font-size:0.6rem;opacity:0.6">TIMESTAMP, VOLTAGE (V)</span>';
    uploadLbl.classList.remove('loaded');
    faultBanner.classList.remove('visible');
    pollLive();
    pollMeta();
  }).catch(e=>{
    rmsgEl.textContent='DELETE FAILED: '+e.message;
  });
}

// ── Saved file bar ────────────────────────────────────────────
function fmtBytes(b) { return b<1024 ? b+' B' : (b/1024).toFixed(1)+' KB'; }

function updateSavedBar(has, size, rows) {
  const savedKey = [has ? 1 : 0, size, rows].join('|');
  if (updateSavedBar._key === savedKey) return;
  updateSavedBar._key = savedKey;
  if (has) {
    savedBar.className='saved-bar has';
    savedLbl.textContent='FILE ON DEVICE';
    savedInf.textContent=fmtBytes(size)+' / '+rows+' ROWS — AUTO-LOADS ON BOOT';
    delBtn.style.display='inline-block';
  } else {
    savedBar.className='saved-bar';
    savedLbl.textContent='NO FILE ON DEVICE';
    savedInf.textContent='Upload CSV to persist to flash';
    delBtn.style.display='none';
  }
}

// ── Restore saved CSV from device on page load ─────────────────
let restoredFromDevice = false;
function maybeRestoreFromDevice(d) {
  if (restoredFromDevice || !d.savedFile || localRows > 0) return;
  restoredFromDevice = true;
  fetch('/csv/download')
    .then(r => r.text())
    .then(text => {
      localRows = parseAndPreview(text);
      uploadLbl.textContent = 'FILE RESTORED FROM DEVICE';
      uploadLbl.classList.add('loaded');
    })
    .catch(() => {});
}

// ── Status poll (live + meta) ─────────────────────────────────
function pollLive() {
  if (actionBusy || livePollAC) return;
  livePollAC = new AbortController();
  fetch('/status/live', {signal: livePollAC.signal, cache:'no-store'})
    .then(r => r.json())
    .then(d => {
      setClassIfChanged('connBanner', connBanner, 'conn-banner');

      // Voltage display (compensated for diode drop)
      setHTMLIfChanged('vdisp', vdisp, d.outV.toFixed(2)+'<span class="seg-unit">V</span>');
      setTextIfChanged('cmdV', cmdVEl, d.cmdV.toFixed(2)+' V');

      // PPS
      setTextIfChanged('ppsText', ppsEl, d.ppsOn?'ON':'OFF');
      setClassIfChanged('ppsClass', ppsEl, d.ppsOn?'s-on':'s-off');
      setClassIfChanged('ppsLed', ppsLed, d.ppsOn?'led led-on':'led led-off');

      // MOSFET
      setTextIfChanged('mosText', mosEl, d.mosfet?'ON':'OFF');
      setClassIfChanged('mosClass', mosEl, d.mosfet?'s-on':'s-off');
      setClassIfChanged('mosLed', mosLed, d.mosfet?'led led-on':'led led-off');

      setTextIfChanged('rv', rv, d.readV.toFixed(2)+' V');

      // INA226 load meter
      if (d.inaOK) {
        setTextIfChanged('inaSt', inaSt, 'OK');
        setClassIfChanged('inaStClass', inaSt, 's-on');
        setClassIfChanged('inaLed', inaLed, 'led led-on');
        setTextIfChanged('inaV', inaVEl, d.inaV.toFixed(2)+' V');
        setTextIfChanged('inaA', inaAEl, d.inaA.toFixed(2)+' A');
        setTextIfChanged('inaW', inaWEl, d.inaW.toFixed(2)+' W');
      } else {
        setTextIfChanged('inaSt', inaSt, 'OFFLINE');
        setClassIfChanged('inaStClass', inaSt, 's-off');
        setClassIfChanged('inaLed', inaLed, 'led led-off');
        setTextIfChanged('inaV', inaVEl, '--');
        setTextIfChanged('inaA', inaAEl, '--');
        setTextIfChanged('inaW', inaWEl, '--');
      }
      setTextIfChanged('temp', tempEl, d.temp.toFixed(1)+' C');

      // Sync speed & loop from device
      currentSpeed = d.replaySpeed || 1;
      syncSpeedSlider(currentSpeed);
      setTextIfChanged('loopText', loopBtn, d.replayLoop ? 'LOOP: ON' : 'LOOP: OFF');
      setClassIfChanged('loopClass', loopBtn, d.replayLoop ? 'btn-orange' : 'btn-gray');

      // Sync range markers from device
      if (d.rangeIn !== undefined && d.rangeOut !== undefined) {
        rangeIn  = d.rangeIn;
        rangeOut = d.rangeOut;
      }

      // Replay badge + progress
      const n = d.replayCount || metaState.replayCount || localRows || 0;
      if (d.replayActive) {
        setTextIfChanged('rbadgeText', rbadge, 'REPLAY');
        setClassIfChanged('rbadgeClass', rbadge, 'rbadge running');
        setProgress(d.replayIdx, n);
      } else if (d.replayWaiting) {
        setTextIfChanged('rbadgeText', rbadge, 'WAITING');
        setClassIfChanged('rbadgeClass', rbadge, 'rbadge waiting');
        setProgress(d.replayIdx, n);
      } else {
        setTextIfChanged('rbadgeText', rbadge, 'MANUAL CONTROL');
        setClassIfChanged('rbadgeClass', rbadge, 'rbadge');
      }
      updateGraphHead(d.replayIdx, n);

      // Power-loss banner
      if (d.replayWaiting) {
        setClassIfChanged('faultBanner', faultBanner, 'fault-banner visible');
        setTextIfChanged('faultPos', faultPos, 'ROW '+d.replayIdx+' / '+n);
      } else {
        setClassIfChanged('faultBanner', faultBanner, 'fault-banner');
      }
    })
    .catch(e => {
      if (e.name === 'AbortError') return; // cancelled by user action — not a real error
      setClassIfChanged('connBanner', connBanner, 'conn-banner visible');
    })
    .finally(() => {
      livePollAC = null;
      scheduleLivePoll(1000);
    });
}

function pollMeta() {
  if (actionBusy || metaPollAC) return;
  metaPollAC = new AbortController();
  fetch('/status/meta', {signal: metaPollAC.signal, cache:'no-store'})
    .then(r => r.json())
    .then(d => {
      metaState.replayCount = d.replayCount || 0;
      metaState.savedFile = !!d.savedFile;
      metaState.savedSize = d.savedSize || 0;
      updateSavedBar(metaState.savedFile, metaState.savedSize, metaState.replayCount);
      maybeRestoreFromDevice(metaState);
    })
    .catch(e => {
      if (e.name !== 'AbortError') console.warn('meta poll failed', e);
    })
    .finally(() => {
      metaPollAC = null;
      scheduleMetaPoll(5000);
    });
}


// ── Graph ─────────────────────────────────────────────────────
const canvas  = $('graph-canvas');
const ctx     = canvas.getContext('2d');
const graphTip= $('graph-tip');
const graphWrap=$('graph-wrap');

let graphRows  = [];   // {ms, v} — set when CSV is parsed
let graphIdx   = 0;    // current playhead index (from poll)
let graphTotal = 0;
let rangeIn    = -1;   // IN marker index (-1 = none)
let rangeOut   = -1;   // OUT marker index (-1 = none)

const G = {
  PAD_L: 44, PAD_R: 10, PAD_T: 10, PAD_B: 24,
  LINE:  '#e8a000',
  PLAYED:'rgba(232,160,0,0.18)',
  HEAD:  '#ff4040',
  GRID:  'rgba(255,255,255,0.04)',
  LABEL: '#5c6870',
};

function resizeCanvas() {
  const rect = graphWrap.getBoundingClientRect();
  canvas.width  = Math.floor(rect.width);
  canvas.height = Math.floor(rect.height);
  drawGraph();
}

function drawGraph() {
  const W = canvas.width, H = canvas.height;
  const pl=G.PAD_L, pr=G.PAD_R, pt=G.PAD_T, pb=G.PAD_B;
  const cw = W-pl-pr, ch = H-pt-pb;

  ctx.clearRect(0, 0, W, H);

  if (!graphRows.length) {
    ctx.fillStyle = '#2a2e32';
    ctx.font = '11px B612 Mono, monospace';
    ctx.textAlign = 'center';
    ctx.fillText('NO CSV LOADED', W/2, H/2);
    return;
  }

  const vMin  = graphRows.reduce((a,r)=>Math.min(a,r.v), Infinity);
  const vMax  = graphRows.reduce((a,r)=>Math.max(a,r.v),-Infinity);
  const vRange= (vMax-vMin) || 1;
  const tMax  = graphRows[graphRows.length-1].ms;

  const xOf = ms  => pl + (ms  / tMax)   * cw;
  const yOf = v   => pt + ch - ((v-vMin) / vRange) * ch;

  // Grid lines (voltage)
  const vStep = niceStep(vRange, 4);
  const vStart= Math.ceil(vMin / vStep) * vStep;
  ctx.strokeStyle = G.GRID;
  ctx.lineWidth   = 1;
  ctx.setLineDash([2, 4]);
  ctx.font        = '9px B612 Mono, monospace';
  ctx.fillStyle   = G.LABEL;
  ctx.textAlign   = 'right';
  for (let v=vStart; v<=vMax+0.001; v+=vStep) {
    const y = Math.round(yOf(v)) + 0.5;
    ctx.beginPath(); ctx.moveTo(pl, y); ctx.lineTo(W-pr, y); ctx.stroke();
    ctx.fillText(v.toFixed(2), pl-4, y+3);
  }

  // Grid lines (time)
  const tSec = tMax / 1000;
  const tStep = niceStep(tSec, 5) * 1000;
  ctx.textAlign = 'center';
  for (let t=tStep; t<tMax; t+=tStep) {
    const x = Math.round(xOf(t)) + 0.5;
    ctx.beginPath(); ctx.moveTo(x, pt); ctx.lineTo(x, H-pb); ctx.stroke();
    ctx.fillText(fmtTime(t), x, H-pb+14);
  }
  ctx.setLineDash([]);

  // Axis labels
  ctx.fillStyle = G.LABEL;
  ctx.textAlign = 'center';
  ctx.fillText(fmtTime(0), pl, H-pb+14);
  ctx.fillText(fmtTime(tMax), W-pr, H-pb+14);

  // Played region fill — always 0→idx (or rangeIn→idx when range set)
  if (graphIdx > 0 && graphIdx <= graphRows.length) {
    const xStart = (rangeIn >= 0 && rangeIn < graphRows.length) ? xOf(graphRows[rangeIn].ms) : pl;
    const xHead  = xOf(graphRows[Math.min(graphIdx,graphRows.length)-1].ms);
    if (xHead > xStart) {
      ctx.fillStyle = G.PLAYED;
      ctx.fillRect(xStart, pt, xHead-xStart, ch);
    }
  }

  // Range markers (IN / OUT)
  if (rangeIn >= 0 && rangeIn < graphRows.length) {
    const xIn = Math.round(xOf(graphRows[rangeIn].ms)) + 0.5;
    ctx.strokeStyle = '#26c44a';
    ctx.lineWidth   = 1.5;
    ctx.setLineDash([4, 3]);
    ctx.beginPath(); ctx.moveTo(xIn, pt); ctx.lineTo(xIn, H-pb); ctx.stroke();
    ctx.setLineDash([]);
  }
  if (rangeOut >= 0 && rangeOut < graphRows.length) {
    const xOut = Math.round(xOf(graphRows[rangeOut].ms)) + 0.5;
    ctx.strokeStyle = '#e02828';
    ctx.lineWidth   = 1.5;
    ctx.setLineDash([4, 3]);
    ctx.beginPath(); ctx.moveTo(xOut, pt); ctx.lineTo(xOut, H-pb); ctx.stroke();
    ctx.setLineDash([]);
  }

  // Waveform — full trace
  ctx.strokeStyle = G.LINE;
  ctx.lineWidth   = 1.5;
  ctx.beginPath();
  graphRows.forEach((r, i) => {
    const x = xOf(r.ms), y = yOf(r.v);
    i===0 ? ctx.moveTo(x,y) : ctx.lineTo(x,y);
  });
  ctx.stroke();

  // Playhead vertical line
  if (graphIdx > 0 && graphIdx <= graphRows.length) {
    const headMs = graphRows[Math.min(graphIdx,graphRows.length)-1].ms;
    const xHead  = Math.round(xOf(headMs)) + 0.5;
    ctx.strokeStyle = G.HEAD;
    ctx.lineWidth   = 1.5;
    ctx.setLineDash([3, 3]);
    ctx.beginPath(); ctx.moveTo(xHead, pt); ctx.lineTo(xHead, H-pb); ctx.stroke();
    ctx.setLineDash([]);
    // Head dot
    const yHead = yOf(graphRows[Math.min(graphIdx,graphRows.length)-1].v);
    ctx.fillStyle = G.HEAD;
    ctx.beginPath(); ctx.arc(xHead, yHead, 3.5, 0, Math.PI*2); ctx.fill();
  }

  // Border
  ctx.strokeStyle = 'rgba(255,255,255,0.04)';
  ctx.lineWidth   = 1;
  ctx.strokeRect(pl+0.5, pt+0.5, cw-1, ch-1);
}

function niceStep(range, targetTicks) {
  const rough = range / targetTicks;
  const mag   = Math.pow(10, Math.floor(Math.log10(rough)));
  const steps = [1,2,2.5,5,10];
  for (const s of steps) if (s*mag >= rough) return s*mag;
  return steps[steps.length-1]*mag;
}

function fmtTime(ms) {
  const s = ms/1000;
  if (s < 60)   return s.toFixed(0)+'s';
  if (s < 3600) return (s/60).toFixed(0)+'m';
  return (s/3600).toFixed(1)+'h';
}

// Binary search: find closest row index for a given ms value — O(log n)
function findNearestRow(ms) {
  let lo = 0, hi = graphRows.length - 1;
  while (lo < hi) {
    const mid = (lo + hi) >> 1;
    if (graphRows[mid].ms < ms) lo = mid + 1;
    else hi = mid;
  }
  // Check lo-1 in case it's closer
  if (lo > 0 && Math.abs(graphRows[lo-1].ms - ms) < Math.abs(graphRows[lo].ms - ms))
    return lo - 1;
  return lo;
}

// Hover tooltip
graphWrap.addEventListener('mousemove', e => {
  if (!graphRows.length) return;
  const rect = graphWrap.getBoundingClientRect();
  const px   = e.clientX - rect.left;
  const cw   = canvas.width - G.PAD_L - G.PAD_R;
  const frac = Math.max(0, Math.min(1, (px - G.PAD_L) / cw));
  const tMax = graphRows[graphRows.length-1].ms;
  const tAt  = frac * tMax;
  const best = findNearestRow(tAt);
  const r = graphRows[best];
  graphTip.textContent = fmtTime(r.ms) + '  ' + r.v.toFixed(2) + ' V';
});
graphWrap.addEventListener('mouseleave', () => { graphTip.textContent=''; });

// ── Range markers (click graph to set IN/OUT) ─────────────────
function graphClickIndex(e) {
  if (!graphRows.length) return -1;
  const rect = canvas.getBoundingClientRect();
  const px   = e.clientX - rect.left;
  const cw   = canvas.width - G.PAD_L - G.PAD_R;
  const frac = Math.max(0, Math.min(1, (px - G.PAD_L) / cw));
  const tMax = graphRows[graphRows.length-1].ms;
  const tAt  = frac * tMax;
  return findNearestRow(tAt);
}

canvas.addEventListener('click', e => {
  const idx = graphClickIndex(e);
  if (idx < 0) return;
  if (rangeOut >= 0 && idx >= rangeOut) return; // can't place IN past OUT
  rangeIn = idx;
  sendRange();
});

canvas.addEventListener('contextmenu', e => {
  e.preventDefault();
  const idx = graphClickIndex(e);
  if (idx < 0) return;
  if (rangeIn >= 0 && idx <= rangeIn) return; // can't place OUT before IN
  rangeOut = idx;
  sendRange();
});

canvas.addEventListener('auxclick', e => {
  if (e.button === 1) { e.preventDefault(); clearRange(); }
});

function sendRange() {
  if (rangeIn >= 0 && rangeOut >= 0 && rangeIn === rangeOut) {
    rmsgEl.textContent = '\u26a0 IN = OUT \u2014 single-row range';
  }
  api('/replay/range?in='+rangeIn+'&out='+rangeOut, rmsgEl);
  drawGraph();
}

function clearRange() {
  rangeIn = -1; rangeOut = -1;
  sendRange();
}

// Called after CSV parse to populate graph
function setGraphData(rows) {
  graphRows = rows;
  graphIdx  = 0;
  drawGraph();
}

// Called from poll to update playhead
function updateGraphHead(idx, total) {
  const needsRedraw = (graphIdx !== idx || graphTotal !== total);
  graphIdx  = idx;
  graphTotal= total;
  if (needsRedraw) drawGraph();
  highlightTableRow(idx);
}

// Highlight active row in CSV table
let prevHighlight = null;

function highlightTableRow(idx) {
  if (prevHighlight) prevHighlight.classList.remove('active');
  if (idx <= 0) { prevHighlight = null; return; }
  const tbl = csvPrev.querySelector('table');
  if (!tbl) return;
  const row = tbl.querySelector('tr[data-idx="'+(idx-1)+'"]');
  if (!row) return;
  row.classList.add('active');
  prevHighlight = row;
}

// Resize observer — redraw when panel width changes
new ResizeObserver(resizeCanvas).observe(graphWrap);
resizeCanvas();

pollLive();
pollMeta();
</script>
</body>
</html>
)HTML";
