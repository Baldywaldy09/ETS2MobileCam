/*-----------------------------------------*/
// Project	: ETS2MobileCam
// File		: Net/phone_page.h
/*-----------------------------------------*/

#pragma once
#include <string>

// The self-contained page served to the phone at http://<pc-lan-ip>:<port>/ (see PhoneServer).
// Android/Chrome only - AbsoluteOrientationSensor has no Safari/WebKit equivalent.
//
// Split across multiple raw string literals, concatenated at compile time, because MSVC
// rejects a single literal beyond ~16KB (C2026) - the content is otherwise one big literal.
namespace phone_page
{
	inline const std::string html = R"PART0(<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1, user-scalable=no, viewport-fit=cover">
<title>ETS2MobileCam</title>
<style>
	:root { color-scheme: dark; }
	body {
		margin: 0;
		padding: 24px 16px 48px;
		background: #101114;
		color: #e8e8ea;
		font-family: -apple-system, Segoe UI, Roboto, sans-serif;
		text-align: center;
	}
	body.camera-active { overflow: hidden; padding: 0; touch-action: none; }
	body.camera-active.settings-open { overflow-y: auto; touch-action: pan-y; }
	h1 { font-size: 20px; margin: 0 0 4px; }
	p.sub { color: #8b8d94; margin: 0 0 24px; font-size: 13px; }
	body.camera-active h1, body.camera-active p.sub { display: none; }

	.status { display: inline-flex; align-items: center; gap: 8px; margin-bottom: 24px; font-size: 14px; }
	.dot { width: 10px; height: 10px; border-radius: 50%; background: #d3455b; }
	.dot.on { background: #3ec97c; }
	body.camera-active .status { display: none; }

	.warning-banner { display: none; }
	.warning-banner.visible {
		display: block;
		max-width: 340px;
		margin: 0 auto 16px;
		padding: 10px 14px;
		background: rgba(211,69,91,.15);
		border: 1px solid #d3455b;
		color: #f3b8c0;
		border-radius: 10px;
		font-size: 13px;
		text-align: left;
	}
	body.camera-active .warning-banner.visible {
		position: fixed;
		top: calc(max(14px, env(safe-area-inset-top)) + 56px);
		left: 14px; right: 14px;
		max-width: none;
		margin: 0;
		z-index: 26;
		background: rgba(30,14,17,.85);
		white-space: nowrap;
		overflow: hidden;
		text-overflow: ellipsis;
	}

	button {
		display: block;
		width: 100%;
		max-width: 340px;
		margin: 0 auto 8px;
		padding: 12px;
		font-size: 14px;
		font-weight: 600;
		border: none;
		border-radius: 10px;
		background: #23252b;
		color: #e8e8ea;
		transition: background .15s;
	}
	button:active { background: #2f323a; }
	button.toggle.on { background: #2c6e49; }
	button:disabled { opacity: .4; }
	.btn-secondary { background: #3a3d46; max-width: 170px; padding: 10px; font-size: 13px; }

	body.camera-active .cam-toolbar {
		display: flex;
		align-items: center;
		gap: 4px;
		position: fixed;
		top: calc(max(14px, env(safe-area-inset-top)) + 20px);
		right: 14px;
		z-index: 30;
		background: rgba(20,21,25,.55);
		border-radius: 999px;
		padding: 5px;
	}
	body.camera-active .cam-toolbar button {
		position: static;
		width: 34px;
		height: 34px;
		max-width: none;
		padding: 0;
		margin: 0;
		border-radius: 50%;
		background: transparent;
		font-size: 0;
		color: transparent;
		display: flex;
		align-items: center;
		justify-content: center;
	}
	body.camera-active .cam-toolbar button:active { background: rgba(255,255,255,.15); }
	body.camera-active .cam-toolbar button:disabled { opacity: .4; }
	body.camera-active #recenter-btn::before { content: '\21ba'; font-size: 18px; color: #e8e8ea; }
	body.camera-active #settings-nav::before { content: '\2699'; font-size: 17px; color: #e8e8ea; }
	body.camera-active #camera-mode-btn::before { content: '\2715'; font-size: 15px; color: #e8e8ea; }

	.cast-frame { display: none; }
	body.camera-active .cast-frame {
		display: block;
		position: fixed;
		inset: 0;
		z-index: 1;
	}
	.cast-view { display: block; width: 100%; border-radius: 12px; background: #000; }
	body.camera-active .cast-view { width: 100%; height: 100%; border-radius: 0; object-fit: cover; }

	.cast-corner { display: none; position: absolute; width: 14px; height: 14px; border-color: rgba(255,255,255,.75); pointer-events: none; z-index: 5; }
	body.camera-active .cast-corner { display: block; }
	.cast-corner.tl { top: 8px; left: 8px; border-top: 2px solid; border-left: 2px solid; }
	.cast-corner.tr { top: 8px; right: 8px; border-top: 2px solid; border-right: 2px solid; }
	.cast-corner.bl { bottom: 8px; left: 8px; border-bottom: 2px solid; border-left: 2px solid; }
	.cast-corner.br { bottom: 8px; right: 8px; border-bottom: 2px solid; border-right: 2px solid; }
	.cast-rec { display: none; position: absolute; top: calc(max(14px, env(safe-area-inset-top)) + 20px); left: 14px; align-items: center; gap: 6px; padding: 4px 8px; background: rgba(0,0,0,.5); border-radius: 6px; font-size: 11px; color: #fff; letter-spacing: .05em; z-index: 5; }
	body.camera-active .cast-rec { display: flex; }
	.cast-rec .dot { width: 8px; height: 8px; border-radius: 50%; background: #d3455b; animation: cast-rec-blink 1.4s steps(1,end) infinite; }
	@keyframes cast-rec-blink { 0%, 92% { opacity: 1; } 94%, 96% { opacity: .25; } 98% { opacity: 1; } }

	.controls-row { display: none; justify-content: center; align-items: center; gap: 20px; margin-top: 24px; }
	.controls-row.visible { display: flex; }
	body.camera-active .controls-row {
		position: fixed;
		left: 0; right: 0;
		bottom: max(16px, env(safe-area-inset-bottom));
		z-index: 21;
		margin: 0;
	}
	.joystick {
		width: 180px; height: 180px; border-radius: 50%;
		background: #1a1c21; border: 2px solid #2f323a;
		position: relative; touch-action: none;
	}
	body.camera-active .joystick { width: 140px; height: 140px; background: rgba(20,21,25,.5); }
	.joystick .knob {
		width: 72px; height: 72px; border-radius: 50%; background: #3ec97c;
		position: absolute; left: 50%; top: 50%;
		transform: translate(-50%, -50%); pointer-events: none;
	}
	.vertical {
		width: 64px; height: 180px; border-radius: 32px;
		background: #1a1c21; border: 2px solid #2f323a;
		position: relative; touch-action: none;
	}
	body.camera-active .vertical { height: 140px; background: rgba(20,21,25,.5); }
	.vertical .knob {
		width: 48px; height: 48px; border-radius: 50%; background: #a5673a;
		position: absolute; left: 50%; top: 50%;
		transform: translate(-50%, -50%); pointer-events: none;
	}

	body.camera-active #control-btn { display: none; }

	.fov-row { max-width: 360px; margin: 24px auto 0; text-align: left; }
	.fov-row label { font-size: 13px; color: #8b8d94; display: block; margin-bottom: 6px; }
	.fov-row input[type=range] { width: 100%; }

	.zoom-row { max-width: 260px; margin: 24px auto 0; display: flex; flex-direction: column; align-items: center; gap: 8px; }
	.zoom-pill { background: rgba(20,21,25,.7); color: #fff; font-size: 13px; font-weight: 600; padding: 4px 14px; border-radius: 999px; }
	.zoom-dial { display: flex; align-items: center; gap: 2px; background: rgba(20,21,25,.55); border-radius: 999px; padding: 4px; }
	.zoom-row input[type=range] { width: 160px; accent-color: #3ec97c; }
	body.camera-active #fov-row {
		position: fixed;
		left: 50%;
		transform: translateX(-50%);
		bottom: calc(max(16px, env(safe-area-inset-bottom)) + 170px);
		z-index: 21;
		max-width: none;
		margin: 0;
	}
	.lens-row { display: flex; gap: 2px; }
	.lens-btn { flex: none; width: auto; margin: 0; padding: 6px 12px; font-size: 12px; border-radius: 999px; background: transparent; color: #b7b9c0; }
	.lens-btn.active { background: #fff; color: #101114; }

	.trim-section { max-width: 360px; margin: 24px auto 0; }
	body.camera-active .trim-section { display: none; }
	.trim-section .section-label { font-size: 13px; color: #8b8d94; margin-bottom: 8px; }
	.trim-row { display: flex; align-items: center; justify-content: center; gap: 16px; }
	.trim-pad {
		width: 120px; height: 120px; border-radius: 50%;
		background: #1a1c21; border: 2px solid #2f323a;
		position: relative; touch-action: none; flex-shrink: 0;
	}
	.trim-pad .knob {
		width: 44px; height: 44px; border-radius: 50%; background: #6a4fa5;
		position: absolute; left: 50%; top: 50%;
		transform: translate(-50%, -50%); pointer-events: none;
	}
	.trim-roll-col { flex: 1; text-align: left; }
	.trim-roll-col label { font-size: 13px; color: #8b8d94; display: block; margin-bottom: 6px; }
	.trim-roll-col input[type=range] { width: 100%; }
	.trim-values { font-size: 12px; color: #8b8d94; margin-top: 8px; }

	.cam-trim-h, .cam-trim-v { display: none; }
	.cam-trim-knob { width: 22px; height: 22px; border-radius: 50%; background: #6a4fa5; position: absolute; left: 50%; top: 50%; transform: translate(-50%, -50%); pointer-events: none; }
	body.camera-active .cam-trim-h {
		display: block;
		position: fixed;
		top: calc(max(14px, env(safe-area-inset-top)) + 96px);
		left: 50%;
		transform: translateX(-50%);
		width: 180px; height: 30px;
		border-radius: 999px;
		background: rgba(20,21,25,.5);
		border: 1px solid rgba(255,255,255,.15);
		z-index: 25;
		touch-action: none;
	}
	body.camera-active .cam-trim-v {
		display: block;
		position: fixed;
		top: 50%;
		right: 12px;
		transform: translateY(-50%);
		width: 30px; height: 150px;
		border-radius: 999px;
		background: rgba(20,21,25,.5);
		border: 1px solid rgba(255,255,255,.15);
		z-index: 25;
		touch-action: none;
	}

	body.camera-active #motion-btn { display: none; }

	#view-settings { display: none; }
	#view-settings.visible {
		display: block;
		position: relative;
		z-index: 40;
		background: #101114;
		min-height: 100vh;
		padding: 24px 16px 48px;
	}
	#view-main.hidden { display: none; }
</style>
</head>
<body>
	<h1>ETS2MobileCam</h1>
	<p class="sub">Switch the in-game camera to the free camera, then enable control below.</p>

	<div class="status"><span id="dot" class="dot"></span><span id="status-text">Connecting...</span></div>
	<div id="warning-banner" class="warning-banner"></div>

	<div id="view-main">
		<button id="motion-btn">Enable Motion Access</button>
		<button id="control-btn" class="toggle" disabled>Enable Control</button>
		<div class="cam-toolbar">
			<button id="recenter-btn" class="btn-secondary" disabled>Recenter</button>
			<button id="settings-nav" class="btn-secondary">Settings</button>
			<button id="camera-mode-btn" class="toggle" disabled>Camera Mode</button>
		</div>

		<div id="cast-frame" class="cast-frame">
			<img id="cast-view" class="cast-view" alt="Live game view">
			<div class="cast-corner tl"></div>
			<div class="cast-corner tr"></div>
			<div class="cast-corner bl"></div>
			<div class="cast-corner br"></div>
			<div class="cast-rec"><span class="dot"></span>LIVE</div>
		</div>

		<div class="cam-trim-h" id="trim-bar-h"><div class="cam-trim-knob" id="trim-bar-h-knob"></div></div>
		<div class="cam-trim-v" id="trim-bar-v"><div class="cam-trim-knob" id="trim-bar-v-knob"></div></div>

		<div class="controls-row" id="controls-row">
			<div class="joystick" id="joystick"><div class="knob" id="joy-knob"></div></div>
			<div class="vertical" id="vertical"><div class="knob" id="vert-knob"></div></div>
		</div>

		<div class="zoom-row" id="fov-row">
			<div class="zoom-pill"><span id="fov-value">1.0</span>&times;</div>
			<div class="lens-row zoom-dial" id="lens-row">
				<button class="lens-btn" data-fov="200">.5&times;</button>
				<button class="lens-btn active" data-fov="100">1&times;</button>
				<button class="lens-btn" data-fov="50">2&times;</button>
				<button class="lens-btn" data-fov="33">3&times;</button>
			</div>
			<input type="range" id="fov-slider" min="10" max="400" value="100">
		</div>

		<div class="trim-section">
			<div class="section-label">Trim - drag to nudge yaw/pitch, release to hold</div>
			<div class="trim-row">
				<div class="trim-pad" id="trim-pad"><div class="knob" id="trim-knob"></div></div>
				<div class="trim-roll-col">
					<label for="trim-roll">Roll trim: <span id="trim-roll-value">0</span>&deg;</label>
					<input type="range" id="trim-roll" min="-45" max="45" value="0">
				</div>
			</div>
			<div class="trim-values" id="trim-values">yaw 0&deg; / pitch 0&deg;</div>
			<button id="trim-reset-btn" class="btn-secondary" style="margin-top:12px;">Reset Trim</button>
		</div>
	</div>

	<div id="view-settings">
		<button id="settings-back" class="btn-secondary">Back</button>

		<div class="fov-row">
			<label for="set-stabilizer">Stabilization strength: <span id="set-stabilizer-value">0.18</span>s</label>
			<input type="range" id="set-stabilizer" min="0.05" max="0.50" step="0.01" value="0.18">
		</div>
		<div class="fov-row">)PART0"
		R"PART1(			<label for="set-speed">Move speed: <span id="set-speed-value">2.5</span> u/s</label>
			<input type="range" id="set-speed" min="0.5" max="5.0" step="0.1" value="2.5">
		</div>
		<div class="fov-row">
			<label for="set-accel">Move response: <span id="set-accel-value">0.20</span>s</label>
			<input type="range" id="set-accel" min="0.05" max="0.50" step="0.01" value="0.20">
		</div>
		<div class="fov-row">
			<label for="set-zoom">Zoom response: <span id="set-zoom-value">0.18</span>s</label>
			<input type="range" id="set-zoom" min="0.05" max="0.50" step="0.01" value="0.18">
		</div>
		<div class="fov-row">
			<label for="set-cast-fps">Cast rate: <span id="set-cast-fps-value">12</span> fps</label>
			<input type="range" id="set-cast-fps" min="1" max="30" step="1" value="12">
		</div>
		<div class="fov-row">
			<label for="set-cast-quality">Cast quality: <span id="set-cast-quality-value">70</span></label>
			<input type="range" id="set-cast-quality" min="20" max="95" step="5" value="70">
		</div>
		<div class="fov-row">
			<label for="set-cast-resolution">Cast resolution: <span id="set-cast-resolution-value">720</span>px</label>
			<input type="range" id="set-cast-resolution" min="360" max="1080" step="60" value="720">
		</div>
		<div class="fov-row">
			<label for="set-head-bob">Head bob: <span id="set-head-bob-value">100</span>%</label>
			<input type="range" id="set-head-bob" min="0" max="150" step="10" value="100">
		</div>
		<div class="fov-row">
			<label for="set-idle-sway">Idle sway: <span id="set-idle-sway-value">100</span>%</label>
			<input type="range" id="set-idle-sway" min="0" max="150" step="10" value="100">
		</div>
		<button id="settings-reset-btn" class="btn-secondary">Reset to Defaults</button>
	</div>

<script>
(function () {
	const dot = document.getElementById('dot');
	const statusText = document.getElementById('status-text');
	const warningBanner = document.getElementById('warning-banner');
	const motionBtn = document.getElementById('motion-btn');
	const controlBtn = document.getElementById('control-btn');
	const recenterBtn = document.getElementById('recenter-btn');
	const controlsRow = document.getElementById('controls-row');
	const joystickEl = document.getElementById('joystick');
	const joyKnobEl = document.getElementById('joy-knob');
	const verticalEl = document.getElementById('vertical');
	const vertKnobEl = document.getElementById('vert-knob');
	const fovSlider = document.getElementById('fov-slider');
	const fovValueEl = document.getElementById('fov-value');
	const lensRow = document.getElementById('lens-row');
	const lensButtons = lensRow.querySelectorAll('.lens-btn');
	const trimPadEl = document.getElementById('trim-pad');
	const trimKnobEl = document.getElementById('trim-knob');
	const trimRollSlider = document.getElementById('trim-roll');
	const trimRollValueEl = document.getElementById('trim-roll-value');
	const trimValuesEl = document.getElementById('trim-values');
	const trimResetBtn = document.getElementById('trim-reset-btn');
	const trimBarH = document.getElementById('trim-bar-h');
	const trimBarHKnob = document.getElementById('trim-bar-h-knob');
	const trimBarV = document.getElementById('trim-bar-v');
	const trimBarVKnob = document.getElementById('trim-bar-v-knob');
	const viewMain = document.getElementById('view-main');
	const viewSettings = document.getElementById('view-settings');
	const settingsNav = document.getElementById('settings-nav');
	const settingsBack = document.getElementById('settings-back');
	const setStabilizer = document.getElementById('set-stabilizer');
	const setStabilizerValue = document.getElementById('set-stabilizer-value');
	const setSpeed = document.getElementById('set-speed');
	const setSpeedValue = document.getElementById('set-speed-value');
	const setAccel = document.getElementById('set-accel');
	const setAccelValue = document.getElementById('set-accel-value');
	const setZoom = document.getElementById('set-zoom');
	const setZoomValue = document.getElementById('set-zoom-value');
	const setCastFps = document.getElementById('set-cast-fps');
	const setCastFpsValue = document.getElementById('set-cast-fps-value');
	const setCastQuality = document.getElementById('set-cast-quality');
	const setCastQualityValue = document.getElementById('set-cast-quality-value');
	const setCastResolution = document.getElementById('set-cast-resolution');
	const setCastResolutionValue = document.getElementById('set-cast-resolution-value');
	const setHeadBob = document.getElementById('set-head-bob');
	const setHeadBobValue = document.getElementById('set-head-bob-value');
	const setIdleSway = document.getElementById('set-idle-sway');
	const setIdleSwayValue = document.getElementById('set-idle-sway-value');
	const settingsResetBtn = document.getElementById('settings-reset-btn');
	const cameraModeBtn = document.getElementById('camera-mode-btn');
	const castFrame = document.getElementById('cast-frame');
	const castView = document.getElementById('cast-view');

	let ws = null;
	let motionGranted = false;
	let controlEnabled = false;
	let yawOffset = 0;
	let lastOrientation = { alpha: 0 };

	// --- Compatibility/status warnings: one banner, worst issue wins so a small screen never
	// shows more than one message at a time. Checked once at startup (environment) or kept
	// live (connection state, free-camera state via /status polling below).
	const warnings = { insecure: false, noSensor: false, sensorDenied: false, disconnected: true, noFreeCam: false };

	function updateWarningBanner() {
		let msg = null;
		if (warnings.insecure) msg = 'Insecure connection - motion sensors are disabled. See the setup notes for the Chrome flag workaround.';
		else if (warnings.noSensor) msg = "This browser doesn't support motion sensors - Android Chrome is required.";
		else if (warnings.sensorDenied) msg = 'Motion sensor failed to start - check Chrome site settings for motion access.';
		else if (warnings.disconnected) msg = 'Not connected to the plugin - make sure the game is running with ETS2MobileCam loaded.';
		else if (controlEnabled && warnings.noFreeCam) msg = 'Switch to the Free Camera in-game for control to take effect.';
		warningBanner.textContent = msg || '';
		warningBanner.classList.toggle('visible', !!msg);
	}

	warnings.insecure = !window.isSecureContext;
	warnings.noSensor = !('AbsoluteOrientationSensor' in window);
	updateWarningBanner();

	// --- Settings: persisted per-device in localStorage, sent to the plugin whenever they
	// change and once on connect. Defaults match the plugin's original hand-picked constants.
	const SETTINGS_DEFAULTS = { stabilizer_tau_max: 0.18, move_speed: 2.5, walk_accel_time: 0.20, zoom_time_constant: 0.18, cast_fps: 12, cast_quality: 70, cast_resolution: 720, head_bob_strength: 1.0, idle_sway_strength: 1.0 };
	let settings = { ...SETTINGS_DEFAULTS };

	function loadSettings() {
		try {
			const raw = localStorage.getItem('ets2mc_settings');
			if (raw) settings = { ...SETTINGS_DEFAULTS, ...JSON.parse(raw) };
		} catch (e) { /* private mode / storage unavailable - defaults stand */ }
	}

	function saveSettings() {
		try { localStorage.setItem('ets2mc_settings', JSON.stringify(settings)); } catch (e) { /* ignore */ }
	}

	function applySettingsToUI() {
		setStabilizer.value = settings.stabilizer_tau_max;
		setStabilizerValue.textContent = settings.stabilizer_tau_max.toFixed(2);
		setSpeed.value = settings.move_speed;
		setSpeedValue.textContent = settings.move_speed.toFixed(1);
		setAccel.value = settings.walk_accel_time;
		setAccelValue.textContent = settings.walk_accel_time.toFixed(2);
		setZoom.value = settings.zoom_time_constant;
		setZoomValue.textContent = settings.zoom_time_constant.toFixed(2);
		setCastFps.value = settings.cast_fps;
		setCastFpsValue.textContent = Math.round(settings.cast_fps);
		setCastQuality.value = settings.cast_quality;
		setCastQualityValue.textContent = Math.round(settings.cast_quality);
		setCastResolution.value = settings.cast_resolution;
		setCastResolutionValue.textContent = Math.round(settings.cast_resolution);
		setHeadBob.value = Math.round(settings.head_bob_strength * 100);
		setHeadBobValue.textContent = Math.round(settings.head_bob_strength * 100);
		setIdleSway.value = Math.round(settings.idle_sway_strength * 100);
		setIdleSwayValue.textContent = Math.round(settings.idle_sway_strength * 100);
	}

	function sendSettings() {
		send({ type: 'settings', ...settings });
	}

	setStabilizer.addEventListener('input', () => {
		settings.stabilizer_tau_max = parseFloat(setStabilizer.value);
		setStabilizerValue.textContent = settings.stabilizer_tau_max.toFixed(2);
		saveSettings();
		sendSettings();
	});
	setSpeed.addEventListener('input', () => {
		settings.move_speed = parseFloat(setSpeed.value);
		setSpeedValue.textContent = settings.move_speed.toFixed(1);
		saveSettings();
		sendSettings();
	});
	setAccel.addEventListener('input', () => {
		settings.walk_accel_time = parseFloat(setAccel.value);
		setAccelValue.textContent = settings.walk_accel_time.toFixed(2);
		saveSettings();
		sendSettings();
	});
	setZoom.addEventListener('input', () => {
		settings.zoom_time_constant = parseFloat(setZoom.value);
		setZoomValue.textContent = settings.zoom_time_constant.toFixed(2);
		saveSettings();
		sendSettings();
	});
	setCastFps.addEventListener('input', () => {
		settings.cast_fps = parseFloat(setCastFps.value);
		setCastFpsValue.textContent = Math.round(settings.cast_fps);
		saveSettings();
		sendSettings();
	});
	setCastQuality.addEventListener('input', () => {
		settings.cast_quality = parseFloat(setCastQuality.value);
		setCastQualityValue.textContent = Math.round(settings.cast_quality);
		saveSettings();
		sendSettings();
	});
	setCastResolution.addEventListener('input', () => {
		settings.cast_resolution = parseFloat(setCastResolution.value);
		setCastResolutionValue.textContent = Math.round(settings.cast_resolution);
		saveSettings();
		sendSettings();
	});
	setHeadBob.addEventListener('input', () => {
		settings.head_bob_strength = parseFloat(setHeadBob.value) / 100;
		setHeadBobValue.textContent = Math.round(settings.head_bob_strength * 100);
		saveSettings();
		sendSettings();
	});
	setIdleSway.addEventListener('input', () => {
		settings.idle_sway_strength = parseFloat(setIdleSway.value) / 100;
		setIdleSwayValue.textContent = Math.round(settings.idle_sway_strength * 100);
		saveSettings();
		sendSettings();
	});
	settingsResetBtn.addEventListener('click', () => {
		settings = { ...SETTINGS_DEFAULTS };
		applySettingsToUI();
		saveSettings();
		sendSettings();
	});

	function showSettings(show) {
		viewSettings.classList.toggle('visible', show);
		viewMain.classList.toggle('hidden', show);
		document.body.classList.toggle('settings-open', show);
	}
	settingsNav.addEventListener('click', () => showSettings(true));
	settingsBack.addEventListener('click', () => showSettings(false));

	// --- Camera Mode: portrait roll (real in-game camera rotation) and the live cast feed
	// are the same thing now - if you're not looking at the phone as a viewfinder, there's no
	// reason to be casting or rolled into portrait, you'd just be watching your monitor. One
	// toggle drives both, and switches the whole page into a full-screen viewfinder layout
	// with the existing controls overlaid on top (see the body.camera-active rules).
	let cameraMode = false;
	let castObjectUrl = null;

	function setCameraMode(enabled) {
		cameraMode = enabled;
		document.body.classList.toggle('camera-active', enabled);
		cameraModeBtn.classList.toggle('on', enabled);
		cameraModeBtn.textContent = enabled ? 'Exit Camera Mode' : 'Camera Mode';

		if (!enabled) {
			castView.removeAttribute('src');
			if (castObjectUrl) { URL.revokeObjectURL(castObjectUrl); castObjectUrl = null; }
		}

		// Camera Mode has no separate shutter button - being in it means you want to drive
		// the camera, so control follows it automatically instead of needing its own toggle.
		setControlEnabled(enabled);

		send({ type: 'portrait', enabled });
		send({ type: 'cast', enabled });
	}
	cameraModeBtn.addEventListener('click', () => setCameraMode(!cameraMode));

	function handleCastFrame(blob) {
		const previous = castObjectUrl;
		castObjectUrl = URL.createObjectURL(blob);
		castView.src = castObjectUrl;
		if (previous) URL.revokeObjectURL(previous);
	}

	// --- AbsoluteOrientationSensor (Android/Chrome only): a fused quaternion straight from
	// the sensor, no Euler decomposition, so no gimbal-lock artifacts near vertical.
	let orientationSensor = null;
	let calibQuat = null; // device orientation captured as "forward" (see Recenter)
	let lastDeviceQuat = { w: 1, x: 0, y: 0, z: 0 };

	function quatMultiply(a, b) {
		return {
			w: a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z,)PART1"
		R"PART2(			x: a.w * b.x + a.x * b.w + a.y * b.z - a.z * b.y,
			y: a.w * b.y - a.x * b.z + a.y * b.w + a.z * b.x,
			z: a.w * b.z + a.x * b.y - a.y * b.x + a.z * b.w
		};
	}

	function quatConjugate(q) {
		return { w: q.w, x: -q.x, y: -q.y, z: -q.z };
	}

	function onSensorReading() {
		const q = orientationSensor.quaternion; // spec order: [x, y, z, w]
		lastDeviceQuat = { x: q[0], y: q[1], z: q[2], w: q[3] };

		if (!calibQuat) calibQuat = lastDeviceQuat; // auto-calibrate on first reading

		const relative = quatMultiply(quatConjugate(calibQuat), lastDeviceQuat);
		send({ type: 'quat', w: relative.w, x: relative.x, y: relative.y, z: relative.z });
	}


	// --- Movement: touch joystick (forward/back, strafe) + vertical fader (up/down). No
	// sensors involved, so no drift/tracking-quality issues.
	let joystickVec = { x: 0, y: 0 };
	let joystickActive = false;
	let joystickPointerId = null;
	const JOYSTICK_RADIUS = 90; // must match .joystick's half-width/height in CSS (normal mode)

	function joystickRadius() {
		return document.body.classList.contains('camera-active') ? 70 : JOYSTICK_RADIUS;
	}

	function updateJoyKnob() {
		const r = joystickRadius();
		joyKnobEl.style.transform = 'translate(calc(-50% + ' + (joystickVec.x * r) + 'px), calc(-50% + ' + (-joystickVec.y * r) + 'px))';
	}

	function setJoystickFromEvent(e) {
		const rect = joystickEl.getBoundingClientRect();
		const cx = rect.left + rect.width / 2;
		const cy = rect.top + rect.height / 2;
		const r = joystickRadius();
		let dx = (e.clientX - cx) / r;
		let dy = (e.clientY - cy) / r;
		const len = Math.sqrt(dx * dx + dy * dy);
		if (len > 1) { dx /= len; dy /= len; }
		joystickVec = { x: dx, y: -dy };
		updateJoyKnob();
	}

	joystickEl.addEventListener('pointerdown', (e) => {
		if (joystickActive) return;
		joystickActive = true;
		joystickPointerId = e.pointerId;
		joystickEl.setPointerCapture(e.pointerId);
		setJoystickFromEvent(e);
	});
	joystickEl.addEventListener('pointermove', (e) => {
		if (!joystickActive || e.pointerId !== joystickPointerId) return;
		setJoystickFromEvent(e);
	});
	function endJoystick(e) {
		if (!joystickActive || e.pointerId !== joystickPointerId) return;
		joystickActive = false;
		joystickPointerId = null;
		joystickVec = { x: 0, y: 0 };
		updateJoyKnob();
	}
	joystickEl.addEventListener('pointerup', endJoystick);
	joystickEl.addEventListener('pointercancel', endJoystick);

	let verticalVal = 0;
	let verticalActive = false;
	let verticalPointerId = null;
	const VERTICAL_RADIUS = 65; // must match .vertical's half-height in CSS (normal mode)

	function verticalRadius() {
		return document.body.classList.contains('camera-active') ? 55 : VERTICAL_RADIUS;
	}

	function updateVertKnob() {
		const r = verticalRadius();
		vertKnobEl.style.transform = 'translate(-50%, calc(-50% + ' + (-verticalVal * r) + 'px))';
	}

	function setVerticalFromEvent(e) {
		const rect = verticalEl.getBoundingClientRect();
		const cy = rect.top + rect.height / 2;
		const r = verticalRadius();
		let dy = (e.clientY - cy) / r;
		dy = Math.max(-1, Math.min(1, dy));
		verticalVal = -dy;
		updateVertKnob();
	}

	verticalEl.addEventListener('pointerdown', (e) => {
		if (verticalActive) return;
		verticalActive = true;
		verticalPointerId = e.pointerId;
		verticalEl.setPointerCapture(e.pointerId);
		setVerticalFromEvent(e);
	});
	verticalEl.addEventListener('pointermove', (e) => {
		if (!verticalActive || e.pointerId !== verticalPointerId) return;
		setVerticalFromEvent(e);
	});
	function endVertical(e) {
		if (!verticalActive || e.pointerId !== verticalPointerId) return;
		verticalActive = false;
		verticalPointerId = null;
		verticalVal = 0;
		updateVertKnob();
	}
	verticalEl.addEventListener('pointerup', endVertical);
	verticalEl.addEventListener('pointercancel', endVertical);

	function setFov(value) {
		value = Math.max(parseFloat(fovSlider.min), Math.min(parseFloat(fovSlider.max), value));
		fovSlider.value = value;
		fovValueEl.textContent = (100 / value).toFixed(1);
		lensButtons.forEach((btn) => btn.classList.toggle('active', Math.abs(parseFloat(btn.dataset.fov) - value) < 1));
		send({ type: 'fov', value: value / 100 });
	}

	fovSlider.addEventListener('input', () => setFov(parseFloat(fovSlider.value)));

	lensButtons.forEach((btn) => {
		btn.addEventListener('click', () => setFov(parseFloat(btn.dataset.fov)));
	});

	// --- Pinch-to-zoom: two-finger pinch anywhere outside the pads/buttons/sliders adjusts
	// FOV directly - works the same whether overlaid on the live feed in Camera Mode or on
	// the plain background otherwise. Spreading fingers apart zooms in, pinching together
	// zooms out, like a real camera app.
	const pinchPointers = new Map();
	let pinchStartDist = null;
	let pinchStartFov = null;

	function pinchDistance() {
		const pts = [...pinchPointers.values()];
		if (pts.length < 2) return null;
		const dx = pts[0].x - pts[1].x;
		const dy = pts[0].y - pts[1].y;
		return Math.sqrt(dx * dx + dy * dy);
	}

	document.body.addEventListener('pointerdown', (e) => {
		if (e.target.closest('.joystick, .vertical, .trim-pad, .cam-trim-h, .cam-trim-v, button, input')) return;
		pinchPointers.set(e.pointerId, { x: e.clientX, y: e.clientY });
		if (pinchPointers.size === 2) {
			pinchStartDist = pinchDistance();
			pinchStartFov = parseFloat(fovSlider.value);
		}
	});
	document.body.addEventListener('pointermove', (e) => {
		if (!pinchPointers.has(e.pointerId)) return;
		pinchPointers.set(e.pointerId, { x: e.clientX, y: e.clientY });
		if (pinchPointers.size === 2 && pinchStartDist) {
			const ratio = pinchDistance() / pinchStartDist;
			setFov(pinchStartFov / ratio);
		}
	});
	function endPinch(e) {
		pinchPointers.delete(e.pointerId);
		if (pinchPointers.size < 2) { pinchStartDist = null; pinchStartFov = null; }
	}
	document.body.addEventListener('pointerup', endPinch);
	document.body.addEventListener('pointercancel', endPinch);
)PART2"
		R"PART3(	// --- Trim: yaw/pitch via a drag pad that nudges an accumulated value at a steady rate
	// while held (like a trim wheel, not a direct position-to-value joystick) - the knob
	// springs back to center on release, but the accumulated yaw/pitch trim is untouched.
	// Roll trim stays a plain slider since it's adjusted far less often. Hidden while in
	// Camera Mode (see CSS) - not something you'd reach for mid-shot.
	let trimYaw = 0, trimPitch = 0;
	let trimPadActive = false;
	let trimPadPointerId = null;
	let trimPadVec = { x: 0, y: 0 };
	const TRIM_PAD_RADIUS = 60; // must match .trim-pad's half-width/height in CSS
	const TRIM_RATE_DEG_PER_SEC = 30;

	function updateTrimKnob() {
		trimKnobEl.style.transform = 'translate(calc(-50% + ' + (trimPadVec.x * TRIM_PAD_RADIUS) + 'px), calc(-50% + ' + (-trimPadVec.y * TRIM_PAD_RADIUS) + 'px))';
	}

	function updateTrimValuesDisplay() {
		trimValuesEl.textContent = 'yaw ' + trimYaw.toFixed(0) + '° / pitch ' + trimPitch.toFixed(0) + '°';
	}

	function sendTrim() {
		send({ type: 'trim', yaw: trimYaw, pitch: trimPitch, roll: parseFloat(trimRollSlider.value) });
	}

	function setTrimPadFromEvent(e) {
		const rect = trimPadEl.getBoundingClientRect();
		const cx = rect.left + rect.width / 2;
		const cy = rect.top + rect.height / 2;
		let dx = (e.clientX - cx) / TRIM_PAD_RADIUS;
		let dy = (e.clientY - cy) / TRIM_PAD_RADIUS;
		const len = Math.sqrt(dx * dx + dy * dy);
		if (len > 1) { dx /= len; dy /= len; }
		trimPadVec = { x: dx, y: -dy };
		updateTrimKnob();
	}

	trimPadEl.addEventListener('pointerdown', (e) => {
		if (trimPadActive) return;
		trimPadActive = true;
		trimPadPointerId = e.pointerId;
		trimPadEl.setPointerCapture(e.pointerId);
		setTrimPadFromEvent(e);
	});
	trimPadEl.addEventListener('pointermove', (e) => {
		if (!trimPadActive || e.pointerId !== trimPadPointerId) return;
		setTrimPadFromEvent(e);
	});
	function endTrimPad(e) {
		if (!trimPadActive || e.pointerId !== trimPadPointerId) return;
		trimPadActive = false;
		trimPadPointerId = null;
		trimPadVec = { x: 0, y: 0 };
		updateTrimKnob();
	}
	trimPadEl.addEventListener('pointerup', endTrimPad);
	trimPadEl.addEventListener('pointercancel', endTrimPad);

	// --- Camera Mode trim bars: same nudge-while-held mechanic as the round pad above, split
	// into two independent 1-axis bars (horizontal = yaw, vertical = pitch) so trim can be
	// reached without leaving the full-screen viewfinder layout.
	const TRIM_BAR_H_RADIUS = 74; // must match .cam-trim-h's half-width minus knob radius
	const TRIM_BAR_V_RADIUS = 64; // must match .cam-trim-v's half-height minus knob radius
	let trimBarHActive = false, trimBarHPointerId = null;
	let trimBarVActive = false, trimBarVPointerId = null;

	function updateTrimBarHKnob() {
		trimBarHKnob.style.transform = 'translate(calc(-50% + ' + (trimPadVec.x * TRIM_BAR_H_RADIUS) + 'px), -50%)';
	}
	function setTrimBarHFromEvent(e) {
		const rect = trimBarH.getBoundingClientRect();
		const cx = rect.left + rect.width / 2;
		let dx = (e.clientX - cx) / TRIM_BAR_H_RADIUS;
		dx = Math.max(-1, Math.min(1, dx));
		trimPadVec.x = dx;
		updateTrimBarHKnob();
	}
	trimBarH.addEventListener('pointerdown', (e) => {
		if (trimBarHActive) return;
		trimBarHActive = true;
		trimBarHPointerId = e.pointerId;
		trimBarH.setPointerCapture(e.pointerId);
		setTrimBarHFromEvent(e);
	});
	trimBarH.addEventListener('pointermove', (e) => {
		if (!trimBarHActive || e.pointerId !== trimBarHPointerId) return;
		setTrimBarHFromEvent(e);
	});
	function endTrimBarH(e) {
		if (!trimBarHActive || e.pointerId !== trimBarHPointerId) return;
		trimBarHActive = false;
		trimBarHPointerId = null;
		trimPadVec.x = 0;
		updateTrimBarHKnob();
	}
	trimBarH.addEventListener('pointerup', endTrimBarH);
	trimBarH.addEventListener('pointercancel', endTrimBarH);

	function updateTrimBarVKnob() {
		trimBarVKnob.style.transform = 'translate(-50%, calc(-50% + ' + (-trimPadVec.y * TRIM_BAR_V_RADIUS) + 'px))';
	}
	function setTrimBarVFromEvent(e) {
		const rect = trimBarV.getBoundingClientRect();
		const cy = rect.top + rect.height / 2;
		let dy = (e.clientY - cy) / TRIM_BAR_V_RADIUS;
		dy = Math.max(-1, Math.min(1, dy));
		trimPadVec.y = -dy;
		updateTrimBarVKnob();
	}
	trimBarV.addEventListener('pointerdown', (e) => {
		if (trimBarVActive) return;
		trimBarVActive = true;
		trimBarVPointerId = e.pointerId;
		trimBarV.setPointerCapture(e.pointerId);
		setTrimBarVFromEvent(e);
	});
	trimBarV.addEventListener('pointermove', (e) => {
		if (!trimBarVActive || e.pointerId !== trimBarVPointerId) return;
		setTrimBarVFromEvent(e);
	});
	function endTrimBarV(e) {
		if (!trimBarVActive || e.pointerId !== trimBarVPointerId) return;
		trimBarVActive = false;
		trimBarVPointerId = null;
		trimPadVec.y = 0;
		updateTrimBarVKnob();
	}
	trimBarV.addEventListener('pointerup', endTrimBarV);
	trimBarV.addEventListener('pointercancel', endTrimBarV);

	trimRollSlider.addEventListener('input', () => {
		trimRollValueEl.textContent = trimRollSlider.value;
		sendTrim();
	});

	trimResetBtn.addEventListener('click', () => {
		trimYaw = 0;
		trimPitch = 0;
		trimRollSlider.value = 0;
		trimRollValueEl.textContent = 0;
		updateTrimValuesDisplay();
		sendTrim();
	});

	// Nudges trim while a pad/bar is held, at the same cadence as the movement send loop below.
	setInterval(() => {
		if (!trimPadActive && !trimBarHActive && !trimBarVActive) return;
		const step = TRIM_RATE_DEG_PER_SEC * 0.05; // 50ms tick
		trimYaw = Math.max(-180, Math.min(180, trimYaw + trimPadVec.x * step));
		trimPitch = Math.max(-90, Math.min(90, trimPitch + trimPadVec.y * step));
		updateTrimValuesDisplay();
		sendTrim();
	}, 50);

	function setStatus(connected) {
		dot.classList.toggle('on', connected);
		statusText.textContent = connected ? 'Connected' : 'Disconnected';
		controlBtn.disabled = !connected;
		recenterBtn.disabled = !connected;
		cameraModeBtn.disabled = !connected;
		warnings.disconnected = !connected;
		updateWarningBanner();
	}

	function connect() {
		// Same host/port as the page itself now (see PhoneServer) - wss over an https tunnel
		// (e.g. ngrok), plain ws otherwise, so this also works through a single tunnel URL.
		const wsScheme = location.protocol === 'https:' ? 'wss://' : 'ws://';
		ws = new WebSocket(wsScheme + location.host + '/ws');
		ws.binaryType = 'blob';
		ws.onopen = () => { setStatus(true); sendSettings(); };
		ws.onclose = () => {
			setStatus(false);
			if (cameraMode) setCameraMode(false);
			setTimeout(connect, 1500);
		};
		ws.onerror = () => ws.close();
		ws.onmessage = (event) => {
			if (typeof event.data !== 'string') handleCastFrame(event.data);
		};
	}

	function send(obj) {
		if (ws && ws.readyState === WebSocket.OPEN) ws.send(JSON.stringify(obj));
	}

	function sendConfig() {
		send({ type: 'config', control_enabled: controlEnabled });
	}

	motionBtn.addEventListener('click', () => {
		motionGranted = true;
		motionBtn.textContent = 'Motion Access Granted';
		motionBtn.disabled = true;

		try {
			orientationSensor = new AbsoluteOrientationSensor({ frequency: 60 });
			orientationSensor.addEventListener('reading', () => {
				if (warnings.sensorDenied) { warnings.sensorDenied = false; updateWarningBanner(); }
				onSensorReading();
			});
			orientationSensor.addEventListener('error', (e) => {
				// Permission denial and hardware/flag issues surface here, asynchronously,
				// rather than as a synchronous throw from the constructor/start() below.
				motionBtn.textContent = 'Sensor Failed: ' + e.error.message;
				motionBtn.disabled = false;
				warnings.sensorDenied = true;
				updateWarningBanner();
			});
			orientationSensor.start();
		} catch (e) {
			motionBtn.textContent = 'Sensor Failed: ' + e.message;
			motionBtn.disabled = false;
			warnings.sensorDenied = true;
			updateWarningBanner();
		}
	});

	function setControlEnabled(enabled) {
		controlEnabled = enabled;
		controlBtn.classList.toggle('on', controlEnabled);
		controlBtn.textContent = controlEnabled ? 'Control Enabled' : 'Enable Control';
		controlsRow.classList.toggle('visible', controlEnabled);
		if (!controlEnabled) {
			joystickVec = { x: 0, y: 0 };
			verticalVal = 0;
			updateJoyKnob();
			updateVertKnob();
		}
		sendConfig();
		updateWarningBanner();
	}
	controlBtn.addEventListener('click', () => setControlEnabled(!controlEnabled));

	recenterBtn.addEventListener('click', () => {
		yawOffset = lastOrientation.alpha || 0;
		calibQuat = lastDeviceQuat;
	});

	// --- Wake lock: keeps the screen from sleeping while this page is the thing being
	// used. Released by the browser whenever the tab goes to background, so re-request
	// it on return rather than tracking it ourselves.
	let wakeLock = null;

	async function requestWakeLock() {
		try {
			wakeLock = await navigator.wakeLock.request('screen');
			wakeLock.addEventListener('release', () => { wakeLock = null; });
		} catch (e) { /* unsupported or denied - not critical */ }
	}

	document.addEventListener('visibilitychange', () => {
		if (document.visibilityState === 'visible' && !wakeLock) requestWakeLock();
	});
	requestWakeLock();

	// Polls the plugin's HTTP status endpoint for things the websocket messages don't cover -
	// currently just whether the in-game camera is actually the free camera.
	async function pollStatus() {
		try {
			const res = await fetch('/status', { cache: 'no-store' });
			if (!res.ok) return;
			const data = await res.json();
			warnings.noFreeCam = !data.free_camera_active;
			updateWarningBanner();
		} catch (e) { /* plugin HTTP server unreachable - the connection warning already covers it */ }
	}
	setInterval(pollStatus, 2000);
	pollStatus();

	loadSettings();
	applySettingsToUI();
	connect();

	// Movement is re-sent continuously (not just on change) so a held joystick/vertical
	// keeps driving movement even if the finger doesn't move between ticks.
	setInterval(() => {
		if (controlEnabled) send({ type: 'joystick', x: joystickVec.x, y: joystickVec.y, vertical: verticalVal });
	}, 50);
})();
</script>
</body>
</html>
)PART3";
}
