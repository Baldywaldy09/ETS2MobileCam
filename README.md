# ETS2MobileCam

Creates a Walking Camera mode for Euro Truck Simulator 2, using either your phone or WASD controls

| Mobile | WASD |
|---|---|
| https://github.com/Baldywaldy09/ETS2MobileCam/docs/preview_mobile.mp4 | https://github.com/Baldywaldy09/ETS2MobileCam/docs/preview_wasd.mp4 |



## What it does
- Changes the free camera's rotation based on your phone's orientation sensor, using a joystick to walk.
- Casts the game's screen back to the phone as a viewfinder, so you can see the full 9:16 aspect preview.
- **F9** toggles WASD + mouse walk mode instead, same movement/rotation code as the phone.
- The camera bobs and sways while walking, and drifts a little when you idle.
- Smooths out raw phone gyro jitter with a stabilizer that tightens up for real camera movement and loosens when you're holding still.

## How it works
1. **Free camera** - `Prism3D/` has the reverse-engineered `camera_manager_u`/`core_camera_u` layout, so the plugin can just read and overwrite the free camera's placement and FOV every tick.
2. **Game Engine Camera Tick** - while the plugin is in control over the camera, the game engines camera tick function gets a `ret` patched at the start so the engine update doesn't reset our writes.
3. **Phone control** - `Net/PhoneServer` serves a small page and a websocket on the same port. The phone streams orientation, joystick and settings over that socket, and `Net/CameraInput` holds the latest state for the tick function to read.
4. **WASD walk mode** - `PCInput` reads raw mouse deltas through `RegisterRawInputDevices` instead of polling cursor position, since the game itself recenters the cursor every frame. WASD gets turned into the same joystick like input the phone sends, so both sources run through the exact same code in `dllmain.cpp`.
5. **Screen casting** - `Net/ScreenCapture` grabs frames off the game window with DXGI Desktop Duplication, JPEG-encodes them, and `PhoneServer` ships them to the phone over the same websocket.
6. **Walk cycle + stabilizer** - `dllmain.cpp` layers a speed-based head bob/sway on top while walking and a slower drift while stood still, and blends the phone's raw rotation through an adaptive slerp that tightens up during fast look movements, and loosens up when you're still, so gyro noise doesn't show up, and responds like a real phone stabilizer.

## Requirements
- Euro Truck Simulator 2 with the telemetry SDK plugin interface enabled. (American Truck Simulator should also work)
- For phone control: a phone on the **same network** as your PC, browser needs `AbsoluteOrientationSensor` support (Android/Chrome only, Safari doesn't have it - including the Chrome app itself).
- Windows, x64, MSVC toolchain (see `CMakeLists.txt`).

## Known issues
- No iOS support, Safari doesn't expose the sensor API needed.
- Pressing F9 without manually entering freecam at least once that session sets your position to `0,0,0`

## Usage - Mobile
1. Build the plugin and drop the DLL into your ETS2 plugins folder.
2. Launch the game, the plugin starts a server on port `47990`.
3. Open that URL on your phone, allow motion sensor access, hit the control toggle.
4. Point the phone to look around, joystick to walk, fader for up/down. Or skip the phone and just press **F9** for WASD + mouse-look.

### Insecure Connection Fix:
Motion sensors need a "secure context" (HTTPS), and the plugin only serves plain HTTP, so Chrome disables them by default and the phone page shows an "Insecure connection" warning. To fix it, on the phone:
1. Open `chrome://flags/#unsafely-treat-insecure-origin-as-secure`.
2. Add your PC's URL (e.g. `http://192.168.x.x:47990`) to the list.
3. Set the flag to **Enabled** and restart Chrome.

## Usage - PC
1. Build the plugin and drop the DLL into your ETS2 plugins folder.
2. Launch the game, press F9 to toggle the walking mode
3. Use WASD to move around, CTRL and Space to move up and down.
4. Use Mouse to look around, scroll wheel to zoom in/out and middle click to reset.

## Contributing
See [`CONTRIBUTING.md`](CONTRIBUTING.md) for the code style and naming guides.
