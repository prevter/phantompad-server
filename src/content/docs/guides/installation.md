---
title: Installation
description: How to install the Phantom Pad Server on Windows and Linux.
---

The Phantom Pad Server is a lightweight background process that receives input from the mobile app and emulates a game controller on your PC. Follow the instructions for your platform below.

---

## Windows

### Installer (recommended)

1. Go to the [latest release](https://github.com/prevter/phantompad-server/releases/latest) and download the `phantompad-x.x.x-setup.exe` file.
2. Run the installer and follow the prompts.
3. The installer will:
   - Install and start the Phantom Pad background service.
   - Automatically install the **ViGEmBus** driver required for controller emulation.
   - Add the firewall rule to allow the server to work properly
4. No further configuration is needed. The service starts automatically with Windows.

### Portable (.exe)

Use this if you don't want to install anything system-wide.

1. Download the portable `.exe` from the [releases page](https://github.com/prevter/phantompad-server/releases/latest).
2. Install the **ViGEmBus** driver manually — download `ViGEmBus_<version>.exe` from the [ViGEmBus releases page](https://github.com/nefarius/ViGEmBus/releases/v1.22.0/) and run it.
3. Launch `phantompad.exe`. It will run in the foreground until closed.

> Note: The portable version does not run as a background service and will not start automatically on boot.

### Firewall

If you chose to skip the installer, Windows Firewall may prompt you to allow Phantom Pad through the firewall. **Allow access on private networks** so the mobile app can discover and connect to your PC.

If you dismissed the prompt or blocked it by accident, you can add a manual rule:

1. Open **Windows Defender Firewall with Advanced Security**.
2. Add an **Inbound Rule** for `phantompad.exe`, allowing **UDP** on port **`35700`**.

---

## Linux

### Prebuilt Binary

1. Download the prebuilt binary for your architecture from the [releases page](https://github.com/prevter/phantompad-server/releases/latest).
2. Make it executable:
   ```sh
   chmod +x phantompad
   ```
3. The server uses the **uinput** kernel subsystem. Make sure the module is loaded:
   ```sh
   sudo modprobe uinput
   ```
4. Your user needs write access to `/dev/uinput`. Either run the server with `sudo`, or add a udev rule (recommended):
   ```sh
   echo 'KERNEL=="uinput", GROUP="input", MODE="0660"' | sudo tee /etc/udev/rules.d/99-uinput.rules
   sudo udevadm control --reload-rules && sudo udevadm trigger
   sudo usermod -aG input $USER
   ```
   Then log out and back in for the group change to take effect.
5. Run the server:
   ```sh
   ./phantompad
   ```

### systemd Service

To have the server start automatically in the background:

1. Complete the prebuilt binary steps above.
2. Copy the binary to a permanent location, e.g. `/usr/local/bin/`:
   ```sh
   sudo cp phantompad /usr/local/bin/
   ```
3. Copy the service file from the repository (or download it separately):
   ```sh
   sudo cp phantompad.service /etc/systemd/system/
   ```
4. Reload systemd and enable the service:
   ```sh
   sudo systemctl daemon-reload
   sudo systemctl enable --now phantompad
   ```
5. Check that it's running:
   ```sh
   sudo systemctl status phantompad
   ```

</TabItem>
</Tabs>

### Firewall (Linux)

If you're running a firewall (e.g. `ufw`), allow UDP traffic on the server's port:

```sh
sudo ufw allow 35700/udp
```

---

## Mobile App

Regardless of platform, install the **Phantom Pad** app on your Android device from the Google Play Store:
https://play.google.com/store/apps/details?id=com.prevter.phantompad

<a href="https://play.google.com/store/apps/details?id=com.prevter.phantompad">
<img src="https://play.google.com/intl/en_us/badges/static/images/badges/en_badge_web_generic.png" width="256px" alt="Get it on Google Play"/>
</a>

Make sure your PC and phone are on the **same Wi-Fi network** before launching the app.