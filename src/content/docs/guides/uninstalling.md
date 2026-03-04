---
title: Uninstalling
description: How to remove the Phantom Pad Server from Windows and Linux.
---

## Windows

### Installer

1. Open **Settings → Apps → Installed apps** (Windows 11) or **Control Panel → Programs → Uninstall a program** (Windows 10).
2. Find **Phantom Pad** in the list and click **Uninstall**.
3. The uninstaller will stop and remove the background service automatically.

> Note: The uninstaller will prompt to remove the ViGEmBus driver, but some other apps might depend on it, so only delete it if you're sure nothing else uses it.

### Portable (.exe)

1. Close `phantompad.exe` if it is running.
2. Delete the executable.
3. Optionally remove the firewall rule

### Removing ViGEmBus (optional)

If Phantom Pad was the only application using ViGEmBus and you want to remove it:

1. Open **Settings → Apps → Installed apps**.
2. Find **ViGEmBus** and uninstall it.
3. Reboot your PC to complete driver removal.

---

## Linux

### systemd Service

1. Stop and disable the service:
   ```sh
   sudo systemctl disable --now phantompad
   ```
2. Remove the service file:
   ```sh
   sudo rm /etc/systemd/system/phantompad.service
   sudo systemctl daemon-reload
   ```
3. Remove the binary:
   ```sh
   sudo rm /usr/local/bin/phantompad
   ```

### Standalone Binary

1. Kill the process if it's running:
   ```sh
   pkill phantompad
   ```
2. Delete the binary:
   ```sh
   rm ./phantompad
   ```

### Removing the udev Rule (optional)

If you added the udev rule during installation and want to clean it up:

```sh
sudo rm /etc/udev/rules.d/99-uinput.rules
sudo udevadm control --reload-rules
```

You can also remove your user from the `input` group if it was added solely for Phantom Pad:

```sh
sudo gpasswd -d $USER input
```

Log out and back in for the group change to take effect.