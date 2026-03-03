# <img src="res/icon192.webp" width="128" align="left" /> Phantom Pad Server

Game controller emulator for Windows and Linux

![Build](https://github.com/prevter/phantompad-server/actions/workflows/build.yml/badge.svg)
![GitHub License](https://img.shields.io/github/license/prevter/phantompad-server)
![GitHub Downloads](https://img.shields.io/github/downloads/prevter/phantompad-server/total)
[![GitHub Release](https://img.shields.io/github/v/release/prevter/phantompad-server)](https://github.com/prevter/phantompad-server/releases/latest)
[![Google Play](https://img.shields.io/badge/Google%20Play-34A853?style=flat&logo=google-play&logoColor=white)](https://play.google.com/store/apps/details?id=com.prevter.phantompad)

---

## About

Phantom Pad turns your smartphone into a wireless game controller for your PC.

It runs as a lightweight background service on Windows and Linux, receiving input data from the mobile app
and emulating a real hardware gamepad. Games recognize it as a standard controller, so no additional configuration is needed in most cases.

On Windows, Phantom Pad uses the ViGEmBus driver to emulate an Xbox 360 controller.

On Linux, it creates a virtual input device using the uinput subsystem, which is compatible with most games and applications.

Designed with performance and low latency in mind:
- Low input latency (typically below 5ms on a local Wi-Fi network under optimal conditions).
- Efficient data transmission using UDP protocol.
- Minimal CPU usage on the host machine.
- Supports up to 8 simultaneous controllers, allowing for local multiplayer gaming.
- Cross-platform support (Windows & Linux)

## How to Use

1. Download and install the Phantom Pad Server on your PC from the [releases page](https://github.com/prevter/phantompad-server/releases/latest).

**Windows users**:

For regular users, the .msi installer is recommended. It will set up the service and install the necessary driver automatically.

If you prefer to run the server without installing it, you can download the portable .exe version.
In this case, you will need to install the ViGEmBus driver manually from [here](https://github.com/nefarius/ViGEmBus/releases/v1.22.0/).

**Linux users**:

You can run the server using the prebuilt binary, or build it from source.
A systemd service file is included in the repository if you wish to run the server as a background service.

2. Install the Phantom Pad app on your mobile device from the [Google Play Store](https://play.google.com/store/apps/details?id=com.prevter.phantompad).
3. Make sure your PC and mobile device are connected to the same Wi-Fi network.
4. Launch the app and pick your computer in the nearby devices list.
5. If everything is set up correctly, computer should recognize a new connected game controller.


## Third-party Libraries

Phantom Pad Server uses the following third-party libraries:

- [fmtlib](https://github.com/fmtlib/fmt) - text formatting library (MIT)
- [Result](https://github.com/geode-sdk/result) - rust-like Result<Ok, Err> container (BSL-1.0)
- [ViGEmClient](https://github.com/nefarius/ViGEmClient) - gamepad driver for Windows (MIT)

Additionally, the installer for Windows includes the ViGEmBus driver, which is licensed under the BSD-3-Clause license.

## License

This project is licensed under Apache License 2.0 - see the [LICENSE](LICENSE) file for details.