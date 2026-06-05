# PP18 Video Tools Qt

Qt Widgets app for preparing PP18 video files, creating lightweight previews, and sending previews to Telegram.

## Features

- Select one or more video files.
- The file list is not filtered by extension controls; only video files are added.
- Default row actions are `Preview` and `TG`; `Fixes` is off by default.
- Toggle all row checkboxes for `Fixes`, `Preview`, or `TG` by clicking the corresponding table header.
- `Fixes` creates corrected `.mov` files in the sibling `fixed/` folder.
- `Preview` creates sibling preview files named `<source>__preview.mp4`.
- If `Fixes` is selected together with `Preview` or `TG`, the preview is created from the fixed `.mov` output.
- `TG` sends preview files to Telegram via `sendVideo`.
- If only `TG` is selected and the preview file already exists, the app sends it without re-encoding.
- The `File` column shows the full source path on the first line and the full current output path on the second line.
- The `Info` column shows source resolution and duration in frames.
- The output path line is muted; if the output already exists, only that output line is shown in pink.
- The table shows generated thumbnails for source files.
- Row selection highlighting is disabled to keep the table colors stable.
- Double-click a row to open the source file in the built-in video player.
- The built-in player supports play/pause, looping playback, timeline scrubbing, current frame display, and keyboard shortcuts.
- The app remembers the selected files, processing options, Telegram credentials, and window geometry.

## Main Workflow

1. Choose one or more video files.
2. Adjust per-row actions:
   - `Fixes`: run the main ffmpeg fix pipeline.
   - `Preview`: create `<source>__preview.mp4`.
   - `TG`: send the preview to Telegram.
3. Click `Запустить`.

If `Fixes` is selected together with `Preview` or `TG`, the app runs the main fixes batch first, then creates/sends the preview from the fixed output.

## Processing Options

The `Обработка` section controls the `Fixes` pipeline:

- `Удалять дублирующиеся кадры`: enables duplicate-frame removal.
- Duplicate removal mode:
  - `Очень мягко`
  - `Мягко`
  - `Умеренно`
  - `Агрессивно`
  - `Максимально (есть риск)`
- `25 fps без добавления кадров`: outputs 25 fps without frame interpolation.
- `Если файл уже есть`: skip or overwrite existing fixed outputs.

Changing duplicate removal or 25 fps automatically syncs the `Fixes` checkbox in the table. If both processing checkboxes are disabled, `Fixes` is cleared.

## Built-In Player

Double-click any row to open the source video in the built-in player.

Shortcuts:

- `Space`: play/pause.
- `Left`: step back by one 25 fps frame.
- `Right`: step forward by one 25 fps frame.

The player loops playback and shows the current frame counter as `current frame / duration frame`.

## Telegram

Open Telegram settings from the top toolbar action `Настройки Telegram`.

Credentials are stored in app settings. They can also be provided with environment variables before launching the app:

```bash
export TG_PP18NOTIFIER_BOT_TOKEN=...
export TG_PP18OUT_CHAT_ID=...
```

Telegram captions contain the short source path. Messages include inline copy buttons for full MacOS and Windows paths:

- ` MacOS path`
- `⊞ Win path`

Windows paths are derived from `/Volumes/work/...` as `w:\...`.

## FFmpeg

The app uses `ffmpeg` for the main `Fixes` pipeline, preview generation, and thumbnails. It uses `ffprobe` for table metadata.

Lookup order:

- app bundle/runtime `bin` folder;
- sibling CLI project paths used by the build scripts;
- `PATH`.

The CMake build tries to copy `ffmpeg` into the app bundle/runtime folder from local `bin/` or the sibling CLI project.

## Build

macOS:

```bash
./build-mac-arm.command
```

Manual CMake build:

```bash
cmake -S . -B build -G Ninja \
  -DCMAKE_PREFIX_PATH=/path/to/Qt/<version>/macos/lib/cmake \
  -DCMAKE_BUILD_TYPE=Debug
cmake --build build
```

Windows:

```bat
build-win-x64.bat
```

Set `QT_BASE`, `MINGW_BIN`, or `NINJA_EXE` if Qt is not installed in the default `C:\Qt\...` layout.

Required Qt modules:

- `Widgets`
- `Network`
- `Multimedia`
- `MultimediaWidgets`
