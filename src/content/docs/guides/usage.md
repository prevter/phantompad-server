---
title: Usage
description: How to use Phantom Pad to control games on your PC with your smartphone.
---

## Quick Start

1. **Start the server** on your PC (it may already be running as a background service after installation).
2. **Connect to the same Wi-Fi network** on both your PC and your phone.
3. **Open the Phantom Pad app** on your phone.
4. The app will scan for nearby servers automatically. **Tap your PC** in the device list.
5. Your PC will register a new virtual game controller — you're ready to play.

---

## How It Works

Phantom Pad consists of two components that work together:

**Server (PC)** — A lightweight background process that listens for UDP packets on your local network. When input data arrives from the app, it forwards the state to the virtual controller driver.

**App (Android)** — Reads your touch/button input and sends the current controller state as compact UDP packets to the server at a high frequency.

### Controller Emulation

**Windows:** The server uses the [ViGEmBus](https://github.com/nefarius/ViGEmBus) kernel driver to present a virtual **Xbox 360 controller** to the operating system. Games see it as a standard XInput device.

**Linux:** The server creates a virtual input device via the **uinput** kernel subsystem. Most games and applications that support gamepads on Linux will pick it up automatically.

### Multiple Controllers

Up to **8 controllers** can connect simultaneously, making local co-op or split-screen play possible without any additional hardware.

---

## Controller Layout

Default layout in the app looks faily similar to regular Xbox 360 controller.
You may edit it by enabling "Edit Mode" and dragging the buttons hovewer you like.

Some choices were made to allow specific inputs:
- Stick presses require double tapping on the virtual joystick
- Triggers support analog input by holding on the button and dragging the finger down to "release" them

Other than that, virtual keys are mapped directly how you would expect them to work.

---

## Game Compatibility

Because Phantom Pad emulates a standard hardware controller (Xbox 360 on Windows, generic gamepad on Linux), it works with the vast majority of games that support controllers out of the box — including those on Steam with XInput support.