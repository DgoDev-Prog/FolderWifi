# FolderWifi

An HTTP file manager for the Nintendo Switch SD card, developed by **Tssr (Diego Ramirez)**. Current version: **0.2.0-alpha**.

The Nintendo Switch and web interfaces are in Spanish. Repository documentation is in English.

## Usage

1. Copy `FolderWifi.nro` to `/switch/FolderWifi/` and launch it from hbmenu.
2. Keep the console and browser on the same local network. Internet access is not required.
3. Open the address displayed on the console or scan the HTTP QR code with **Y**. The QR code pairs the browser; if you open the address manually, enter the pairing code shown on the console.
4. The application starts in **read-only mode**. Press **A** on the main screen to enable modifications. Returning to read-only mode stops pending modifying operations; completed changes remain.

| Control | Function |
|---|---|
| A, main screen | Toggle read-only / write access |
| Y | Open the QR window |
| Left / right, QR window | Switch between HTTP, Wi-Fi, and Wi-Fi credentials as text |
| X | Open the activity log |
| Up / down, activity log | Scroll; A resumes following new messages |
| ZL | Open connection and local network options |
| A, connection options | Confirm the selected option |
| ZL → connected devices entry | View clients of the Wi-Fi network created by FolderWifi |
| Up / down, connected devices | Scroll the list; B returns to network options |
| B | Close the window; decline the local network offer |
| + | Stop services and exit |

## Web file management

- Browse without page reloads, use browser history, select multiple items or ranges, search the current folder or subfolders, and sort or paginate the listing.
- Create folders, copy and move directory trees, rename, duplicate, merge folders, and view properties.
- Download files using temporary authorization; prepare selections or folders as ZIP archives; create ZIP/ZIP64 archives and extract ZIP files with integrity checks and path protection.
- Move items to the trash, restore them, or delete them permanently. Operations run in the background with progress and cancellation.
- Resolve conflicts by replacing or merging, keeping both, skipping, or canceling. Apply decisions individually or to subsequent conflicts. Known conflicts are checked before copy, move, rename, and ZIP creation, then checked again during execution. Additional conflicts and extraction conflicts are resolved while the operation runs.
- Replacement files are prepared separately. A backup and recovery record protect the rename step. At startup, interrupted replacements are reviewed and backups requiring attention are preserved in the trash.
- Built-in dialogs, notifications, and logs; light/dark themes; a context menu and desktop shortcuts. Mobile devices have touch selection and a bottom action bar; desktop listings scroll independently.

Cancellation does not automatically undo completed work. Results distinguish completed, skipped, and replaced files. Items that were not moved remain in the clipboard.

## Uploads and recovery

Each item keeps the destination selected when it is added. An unconfirmed selection is a **draft**. Confirming the draft starts an upload batch; subsequent confirmed batches wait for the active batch to finish. Adding files does not confirm the new draft automatically.

Uploads use persistent identifiers, chunks of up to 1 MiB, a queryable offset, and CRC32 checks for each chunk and the complete file. Lost responses are reconciled with the console before data is resent. Partial files remain in `sdmc:/.folderwifi/uploads/` and are published after completion and verification. CRC32 detects corruption; access authorization is handled separately.

The queue and files are stored in IndexedDB while the browser has sufficient space and retains its data. File contents are stored once; progress updates only change the queue state. If storage quota prevents persistence, the user is informed and the selection remains in the open tab's memory. Exporting pending uploads saves destinations and identifiers; restoring them requires selecting the original files again. Size and content are checked before resuming.

Closing or suspending a page stops execution until it is opened again, when remote status is checked. **Changing the IP address changes the browser's web origin**: stored files do not migrate automatically. With the old tab open, the queue transfer action opens the local address you enter and shares the queue and files with the new tab. It validates the origin, window, and a random transfer identifier, then pauses the old tab. The draft remains unconfirmed. If the old tab is closed or the browser blocks the new window, export and restore pending uploads and reassociate the original files. Recovery is automatic when a temporary connection loss preserves the same address.

Folder selection preserves relative file paths. To include empty folders, drag the folder onto the listing in a compatible desktop browser, or upload and extract a ZIP containing them. ZIP uploads also provide an alternative for mobile devices without folder selection support.

## Local Wi-Fi

An existing Wi-Fi or Ethernet connection with a local IP address is retained even without Internet access. After a connection is lost, FolderWifi requests **five reconnection attempts** from Nintendo's network manager, allowing up to 10 seconds per attempt with delays of 2, 4, 8, and 15 seconds. The system chooses between known profiles; FolderWifi does not enumerate or modify saved networks.

If reconnection fails, a local network is offered. It is also offered at startup when no connection exists. Declining keeps the current screen and does not activate the access point; **ZL** lets you activate it later.

Local mode uses **LP2P with standard WPA2-PSK** and requires system version **11.0.0 or later** plus access to the service from the Homebrew environment. The service supplies the actual SSID and IP address. The returned profile is retained for later activations. The password consists of 20 random characters generated by the system's cryptographic service and remains unchanged until the user regenerates it. Changing it requires reconnecting clients.

The header identifies **applet mode** or **application mode** using the type returned by libnx, and records it at startup. The launch mode does not guarantee that the process has the identifiers required by LP2P. After a `00020AE7` rejection, the requested limit was reduced from eight to **one client** for hardware compatibility checks. The log reports the requested limit, the failed LP2P operation, and its error code. The latest activation error remains visible even if the usual network connection is available. When the host application changes, FolderWifi requests the identifier allowed for the current process rather than reusing the saved group's identifier.

The **Y** window provides three QR codes:

- **HTTP:** the current URL and authorization in its fragment, which is removed from browser history when the page opens.
- **Wi-Fi:** `WIFI:T:WPA;S:...;P:...;H:false;;`, with escaped fields and a quiet zone around the code.
- **Wi-Fi credentials as text:** network name, password, security, and status for saving. Credentials are also displayed for manual entry.

After the profile has been created, its QR codes can be viewed while the local network is inactive, with that status indicated. The phone's reader determines whether it recognizes the Wi-Fi format. **A successful build does not verify access point compatibility, SSID persistence, or camera recognition; these require checks on the console and actual devices.**

**ZL → connected devices entry** opens a window showing the number of clients and each member's IP and MAC address. The network worker refreshes the list every second. Inactive network, pending query, empty list, and query failure are displayed separately; a failed query is not reported as zero clients. The console itself is excluded by MAC or IP when it can be identified. This window does not enumerate clients of an external router or infer device names or models. It does not increase the configured one-client limit.

The developer has reported successful local network creation, connection through the Wi-Fi QR code, and reading the credentials QR code as text on the device used. The member list still requires verification after building the change.

FolderWifi does not fall back to an open network or create a bridge/proxy to the phone. The service uses local HTTP, not TLS. Use trusted local networks; the Wi-Fi password and web pairing serve different purposes.

## Building and source layout

Requirements: devkitPro, devkitA64, libnx, and the `minizip` / `zlib` portlibs. Build with `make` using C++17. The Makefile produces ELF, NACP, and NRO files. The application icon is `icon.jpg`.

| Source | Responsibility |
|---|---|
| `source/main.cpp` | Startup, shutdown, controls, and rendering; no transfers in its loop |
| `source/gui.cpp/.hpp`, `font8x16.cpp/.h` | Windows, QR codes, logs, and framebuffer rendering with the actual stride |
| `source/network.cpp`, `runtime.hpp` | Reconnection, WPA2 profile, commands, and shared state |
| `source/http_server.cpp` | HTTP, authorization, downloads, and the file operation worker |
| `source/upload_manager.cpp/.hpp` | Temporary files, offsets, integrity checks, and upload recovery |
| `source/file_manager.cpp/.hpp`, `operations.cpp/.hpp` | Paths, file operations, trash, planning, and search |
| `source/archive_manager.cpp/.hpp` | ZIP/ZIP64 creation and extraction |
| `source/notification_manager.cpp/.hpp` | Event history for the web interface |
| `source/web_ui.cpp` | HTML, CSS, and JavaScript embedded in the NRO |
| `source/qr_codec.c/.h` | Nayuki's QR generator, with its MIT license preserved |

The server has three HTTP workers and one file operation worker. Responses use complete-send loops with bounded waits. Modifications require authorization and POST requests. Ambiguous parameters, duplicate headers, oversized bodies, paths outside the SD card, unsafe components, invalid Unicode, and web access to internal configuration are rejected.

Current limits: paths of up to 768 UTF-8 bytes, components of up to 255 bytes, 64 recursion levels, 100,000 items per traversal, 50,000 entries per folder, 5,000 search results, 512 selected sources per operation, and 32 pending jobs. The SD card's filesystem also limits maximum file size. The console font supports basic Spanish/Latin characters and substitutes unsupported characters; the web interface preserves Unicode.

## Validation status

Pending hardware checks include controls during transfers, large files and insufficient storage, cancellation and recursive conflicts, lost connections and responses, restarting with partial uploads, entering and leaving LP2P, five reconnection attempts, declining the local network offer, password/SSID persistence between activations, and all three QR codes on Android/iOS. A copy or ZIP operation whose tracking is lost after restarting the NRO must be reviewed before repeating it; persistent identifier recovery applies to uploads.

The version remains **0.2.0-alpha**. Successful network and QR checks do not establish the stability of all SD operations. A stable V1 tag requires a build of the final state and results from the hardware checks above, including the counter when a client connects or disconnects. Future USB/FTP transports and other extensions are not required to stabilize the current HTTP functionality.

References: [libnx LP2P](https://switchbrew.github.io/libnx/lp2p_8h.html), [official example](https://github.com/switchbrew/switch-examples/tree/master/network/lp2p), [ZXing Wi-Fi format](https://github.com/zxing/zxing/wiki/Barcode-Contents#wi-fi-network-config-android-ios-11), [Nayuki QR-Code-generator](https://github.com/nayuki/QR-Code-generator). The QR library files retain their MIT license.
