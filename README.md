# FolderWifi

**FolderWifi** is a wireless file manager for Nintendo Switch Homebrew.

It runs a local web server directly on the Nintendo Switch, allowing files and folders on the SD card to be browsed and managed from a web browser on another device connected to the same local network.

> **Current version:** `v0.1.0-alpha`

Developed by **Tssr (Diego Ramirez)**.

---

## Status

FolderWifi is currently in **Alpha**.

The application is functional and can already be used for local SD card management, but some features are incomplete and additional security and stability improvements are planned.

This release is intended primarily for testing and early feedback.

---

## Features

### Nintendo Switch

- Native Nintendo Switch interface.
- Displays the Switch local IP address.
- Built-in activity log.
- QR code for quick access to the web interface.
- Online / Offline operating modes.
- Runs a local HTTP server on port `8080`.

### Web Interface

From a PC, phone or other device connected to the same network you can:

- Browse the Nintendo Switch SD card.
- Navigate through folders.
- Download files.
- Create folders.
- Rename files and folders.
- Delete files.
- Delete empty folders.
- Copy files.
- Move files and folders.
- Select multiple items.
- Switch between light and dark web themes.

---

## Online and Offline Modes

FolderWifi starts in **Offline mode**.

### Offline

The SD card can be browsed from the web interface, but modification operations are disabled.

### Online

Press `A` on the Nintendo Switch to enable Online mode.

Online mode enables file management operations such as:

- Creating folders
- Renaming
- Copying
- Moving
- Deleting
- Downloading

> **Warning**
>
> FolderWifi currently does not use authentication.
>
> While Online mode is enabled, devices on the same local network that can access the FolderWifi address may potentially interact with the SD card.
>
> Only enable Online mode on networks you trust.

---

## Controls

| Button | Action |
|---|---|
| `A` | Toggle Online / Offline mode |
| `Y` | Show or hide QR code |
| `B` | Close QR window |
| `+` | Exit FolderWifi |

---

## How to Use

1. Launch FolderWifi from the Nintendo Switch Homebrew Menu.
2. Make sure the Nintendo Switch is connected to Wi-Fi.
3. FolderWifi will display a local address similar to:

```text
http://192.168.1.100:8080
```

4. Connect your PC or mobile device to the same network.
5. Open the displayed address in your web browser.

You can also press `Y` to display the QR code.

---

## Installation

Copy the application to your Nintendo Switch SD card using a structure similar to:

```text
/switch/FolderWifi/FolderWifi.nro
```

Then launch **FolderWifi** from the Homebrew Menu.

---

## Building from Source

### Requirements

FolderWifi is developed using:

- devkitPro
- devkitA64
- libnx
- Nintendo Switch Homebrew development environment

A working devkitPro installation with the Nintendo Switch development packages is required.

### Build

Clone the repository and enter the project directory:

```bash
git clone https://github.com/YOUR_USERNAME/FolderWifi.git
cd FolderWifi
```

Compile with:

```bash
make
```

To clean the project:

```bash
make clean
```

The resulting Homebrew application will be generated as:

```text
FolderWifi.nro
```

---

## Project Structure

```text
FolderWifi/
├── source/
│   ├── font8x16.cpp
│   ├── font8x16.h
│   ├── gui.cpp
│   ├── gui.hpp
│   ├── main.cpp
│   ├── qrcodegen.cpp
│   └── qrcodegen.hpp
│
├── icon.jpg
├── Makefile
├── README.md
└── .gitignore
```

---

## Alpha Limitations

The current `v0.1.0-alpha` release has known limitations.

- Uploading files from the web interface is not implemented yet.
- Folder copying is not fully implemented.
- Non-empty folders cannot currently be deleted recursively.
- File operations still require additional path validation and error handling.
- There is currently no authentication or access PIN.
- The HTTP server is intended for trusted local networks only.
- Additional testing on different SD cards, networks and Switch configurations is required.

These areas are planned for future versions.

---

## Planned Features

Future development may include:

- File uploads.
- Recursive folder copy.
- Recursive folder deletion.
- Improved file operation validation.
- Improved HTTP request handling.
- Authentication or local access PIN.
- Better error reporting.
- GitHub issue / feedback integration.
- Additional interface improvements.
- Improved security for file operations.

---

## Feedback and Bug Reports

FolderWifi is currently under active development.

Once the GitHub repository is configured, bugs and feature suggestions will be tracked using **GitHub Issues**.

A future FolderWifi version is also planned to provide direct links from the web interface for:

- Reporting a bug.
- Suggesting a feature.

---

## Disclaimer

FolderWifi directly interacts with files stored on the Nintendo Switch SD card.

Although safeguards are being developed, this is currently Alpha software.

Make backups of important data before testing file modification features.

Use FolderWifi at your own risk.

---

## Author

**Tssr (Diego Ramirez)**

Nintendo Switch Homebrew project.

---

## Version

`v0.1.0-alpha`
