# Agent Handoff

Context for continuing development in a new Codex/CLion chat, especially on the macOS machine.

## Repository

GitHub:

```text
https://github.com/ayurash77/pp18-video-tools_qml.git
```

Current Windows workspace:

```text
D:\DevProjects\pp18-video-tools\pp18-video-tools_qml
```

Expected macOS workspace:

```text
/Users/ayurash/Development/_Projects/pp18-video-tools/pp18-video-tools_qml
```

This is now a Qt 6 QML/C++ app named `PP18_VideoTools`. Older handoff notes that mentioned `pp18-video-tools_qt`, `MainWindow`, or Qt Widgets are obsolete.

The sibling CLI project is still expected at:

```text
../pp18-video-tools_cli
```

The app can also use bundled tools from local `bin/`:

```text
bin/ffmpeg
bin/ffprobe
bin/ffmpeg.exe
bin/ffprobe.exe
```

## Git State

Latest commits pushed to `origin/main` from the Windows machine:

```text
4b0c5e7 Handle missing GitHub releases
f6412ed Add GitHub release update checker
d36e85c Fix Windows dialogs fonts and playback
97c309d Add cache settings and playback polish
8401a67 Refactor resource paths and improve build scripts for better compatibility and dependency handling.
```

Before continuing on macOS:

```bash
git checkout main
git pull origin main
```

`AGENT_HANDOFF.md` is ignored by `.gitignore`, so this file is local context and is not pushed unless `.gitignore` is changed.

## Main Files

Important source files:

```text
CMakeLists.txt
README.md
build-mac-arm.command
build-win-x64.bat
src/main.cpp
src/app/AppController.h
src/app/AppController.cpp
src/app/VideoFileModel.h
src/app/VideoFileModel.cpp
src/services/FfmpegBatchService.h
src/services/FfmpegBatchService.cpp
src/services/FfmpegPreviewService.h
src/services/FfmpegPreviewService.cpp
src/services/TelegramController.h
src/services/TelegramController.cpp
src/services/TelegramService.h
src/services/TelegramService.cpp
src/services/UpdateService.h
src/services/UpdateService.cpp
src/platform/NativeFileDialog.h
src/platform/NativeFolderDialog.h
src/platform/NativeFileDialog.cpp
src/platform/NativeFolderDialog.cpp
src/platform/NativeFileDialog_mac.mm
src/platform/NativeFolderDialog_mac.mm
src/qml/Main.qml
src/qml/AppFonts.qml
src/qml/AppStyle.qml
src/qml/SettingsWindow.qml
src/qml/PlayerWindow.qml
src/qml/LogWindow.qml
src/qml/components/*.qml
src/resources.qrc
```

## Current App Behavior

- QML desktop UI with a file/task list.
- Add video files or folders.
- Generate thumbnails and metadata with `ffmpeg`/`ffprobe`.
- Run fixes with `FfmpegBatchService`.
- Generate lightweight preview files with `FfmpegPreviewService`.
- Send preview files to Telegram.
- Manage multiple Telegram recipients in Settings.
- Built-in QML player supports versions, zoom, frame stepping, split compare, cache preview playback.
- Media cache stores thumbnails and H.264 preview files under a configurable cache root.
- Settings window contains cache, Telegram, and update-checking UI.

## Windows Fixes Already Done

Do not remove these when continuing on macOS:

- Windows native file/folder dialogs were implemented in:

```text
src/platform/NativeFileDialog.cpp
src/platform/NativeFolderDialog.cpp
```

- These use PowerShell/Windows Forms and safely embed Russian titles/start paths.
- macOS native dialogs remain in:

```text
src/platform/NativeFileDialog_mac.mm
src/platform/NativeFolderDialog_mac.mm
```

- Windows font rendering was fixed by using system fonts on Windows:
  - UI: `Segoe UI`
  - mono/logs: `Consolas`

Relevant files:

```text
src/main.cpp
src/qml/AppFonts.qml
src/qml/Main.qml
```

- Windows player file URL handling was fixed by adding:

```text
AppController::localFileUrl()
```

and using `QUrl::fromLocalFile()` instead of manual `file://` string construction.

- Windows player now reloads when cache preview becomes ready, so unsupported original video formats can switch to H.264 preview.

Relevant files:

```text
src/app/AppController.h
src/app/AppController.cpp
src/qml/PlayerWindow.qml
```

## GitHub Releases / Updates

The app now has a first-stage update checker:

```text
src/services/UpdateService.h
src/services/UpdateService.cpp
```

It calls:

```text
https://api.github.com/repos/ayurash77/pp18-video-tools_qml/releases/latest
```

It compares `tag_name` with `QCoreApplication::applicationVersion()`.

The app version is now set by CMake:

```cmake
project(PP18_VideoTools VERSION 0.1.4 LANGUAGES CXX)
target_compile_definitions(${PROJECT_NAME} PRIVATE APP_VERSION="${PROJECT_VERSION}")
```

`src/main.cpp` applies this to:

```cpp
QCoreApplication::setApplicationVersion(QStringLiteral(APP_VERSION));
```

Settings window now exposes:

- `История обновлений`
- `Проверить`
- `Открыть релиз`
- `Скачать`

If the repository has no published GitHub Releases yet, GitHub returns 404 and the app now shows:

```text
На GitHub пока нет опубликованных релизов для этого репозитория.
```

## Release Naming Convention

Use tags:

```text
v0.1.0
v0.1.1
v0.1.2
v0.1.3
v0.1.4
```

The CMake project version must match the release without the `v` prefix:

```cmake
project(PP18_VideoTools VERSION 0.1.4 LANGUAGES CXX)
```

Recommended GitHub Release asset names:

```text
PP18_VideoTools_win-x64_v0.1.4.zip
PP18_VideoTools_macos-arm64_v0.1.4.zip
PP18_VideoTools_macos-universal_v0.1.4.zip
PP18_VideoTools_macos-x64_v0.1.4.zip
```

`UpdateService::platformAssetScore()` chooses an asset based on platform and architecture:

- Windows: looks for `win`/`windows`, prefers `x64`, `x86_64`, `amd64`, prefers `.zip`.
- macOS: looks for `mac`, `macos`, `darwin`, `osx`, prefers `universal`, or matching `arm64`/`x64`, prefers `.dmg`, then `.zip`.

## GitHub Actions

Local workflows now exist:

```text
.github/workflows/ci.yml
.github/workflows/release.yml
```

GitHub Actions currently installs Qt directly through `aqtinstall` and uses Qt `6.8.3`.
Base Qt archives:

```text
qtbase qtdeclarative
```

Extra Qt modules:

```text
qtmultimedia qtshadertools
```

Base Qt archives include:

```text
qtbase qtdeclarative qtsvg
```

Do not switch Actions to `6.11.x` unless `aqt`/Qt online repositories show that this version is available for both macOS and Windows.

`ci.yml` builds macOS arm64 and Windows x64 on pushes to `main`, pull requests, and manual runs.

`release.yml` builds packages and publishes a GitHub Release when a `v*` tag is pushed:

```yaml
on:
  push:
    tags:
      - "v*"
```

It produces package assets named like:

```text
PP18_VideoTools_macos-arm64_v0.1.2.zip
PP18_VideoTools_win-x64_v0.1.2.zip
```

Release packaging checks:

- macOS `macdeployqt` must run with `-qmldir=.../src/qml`; otherwise `QtQuick.Controls` QML plugins are missing from `Contents/Resources/qml`.
- macOS/Windows releases must install/link Qt Svg and verify `qsvg` image plugin; all toolbar icons are SVG resources.
- macOS bundle is ad-hoc signed after `macdeployqt`; full Apple Developer ID signing/notarization is still future work.
- Windows package must include deployed QML modules and MSVC runtime DLLs such as `vcruntime140.dll`.
- GitHub Actions checkout must use `lfs: true`; `bin/ffmpeg*` and `bin/ffprobe*` are Git LFS files, and pointer files break runtime.
- Release builds currently use Qt `6.8.3`; avoid QML APIs introduced in newer Qt versions unless Actions is updated and verified.

This will allow development on macOS while GitHub Actions builds Windows packages automatically.

For private/internal usage, unsigned macOS zip/app is acceptable but Gatekeeper may warn. For public distribution, add Apple Developer ID signing and notarization later.

## Windows Release Build

Verified Windows toolchain:

```text
Qt:    C:\Qt\6.11.1\mingw_64
MinGW: C:\Qt\Tools\mingw1310_64
Ninja: C:\Qt\Tools\Ninja\ninja.exe
CMake: C:\Qt\Tools\CMake_64\bin\cmake.exe
```

Release build command used successfully:

```powershell
$env:PATH='C:\Qt\Tools\mingw1310_64\bin;' + $env:PATH
& 'C:\Qt\Tools\CMake_64\bin\cmake.exe' -S . -B 'release\PP18_VideoTools_win-x64' -G Ninja `
  -DCMAKE_MAKE_PROGRAM='C:\Qt\Tools\Ninja\ninja.exe' `
  -DCMAKE_PREFIX_PATH='C:\Qt\6.11.1\mingw_64\lib\cmake' `
  -DCMAKE_BUILD_TYPE=Release
& 'C:\Qt\Tools\CMake_64\bin\cmake.exe' --build 'release\PP18_VideoTools_win-x64' --config Release
```

Release output:

```text
release\PP18_VideoTools_win-x64\PP18_VideoTools.exe
```

The release folder includes Qt runtime, QML modules, assets, and:

```text
bin\ffmpeg.exe
bin\ffprobe.exe
```

Warnings seen during Windows build are currently non-blocking:

- `WrapVulkanHeaders` not found.
- `Qt6TaskTree` not found for `Qt6QmlAssetDownloaderPrivate`.
- `dxcompiler.dll` and `dxil.dll` not found.

## macOS Build

Use the existing script first:

```bash
./build-mac-arm.command
```

If manual build is needed, use a Qt macOS CMake path, for example:

```bash
cmake -S . -B build-mac-arm -G Ninja \
  -DCMAKE_PREFIX_PATH=/Users/ayurash/Development/Qt/6.11.0/macos/lib/cmake \
  -DCMAKE_BUILD_TYPE=Release
cmake --build build-mac-arm --config Release
```

Adjust Qt version/path to the installed Qt on the Mac.

The app target is:

```text
PP18_VideoTools.app
```

On macOS, CMake should include:

```text
src/platform/NativeFileDialog_mac.mm
src/platform/NativeFolderDialog_mac.mm
```

and should copy ffmpeg/ffprobe into app resources.

## Things To Test On macOS

After pulling `origin/main`:

1. Configure/build the QML project.
2. Open app and verify Cyrillic UI fonts.
3. Test native file selection.
4. Test native folder selection.
5. Add a folder with videos and confirm files appear.
6. Open built-in player on a source video.
7. Confirm cache preview generation and playback.
8. Open Settings:
   - check version text,
   - click `История обновлений`,
   - click `Проверить`.
9. If no GitHub releases exist, expect the “no published releases” message.
10. Create a first GitHub Release `v0.1.0` and attach platform assets.
11. Test update check again.

## Development Notes

- Preserve Windows-specific fixes when touching shared QML or C++.
- Do not replace the Windows `.cpp` dialog implementations with QML-only stubs.
- Do not remove macOS `.mm` dialog files.
- Keep update checker platform-neutral. Platform selection should stay in `UpdateService::platformAssetScore()`, not in QML.
- Current update mechanism opens/downloads a release asset. It does not silently replace the running app.
- Full auto-install requires a future separate updater executable/helper.

## Suggested Next Tasks

1. Push the workflow commit and verify the `CI` workflow on GitHub Actions.
2. Create the first tag/release `v0.1.0`.
3. Confirm that Actions attaches:

```text
PP18_VideoTools_win-x64_v0.1.0.zip
PP18_VideoTools_macos-arm64_v0.1.0.zip
```

4. On the next app version, bump CMake `VERSION`, tag `v0.1.1`, and let Actions build/upload assets.
5. Later: add macOS code signing/notarization and optional Windows signing.

When continuing in a new chat, ask the agent to read this file plus `README.md`, `CMakeLists.txt`, `src/app/AppController.*`, `src/services/UpdateService.*`, and `src/qml/SettingsWindow.qml`.
