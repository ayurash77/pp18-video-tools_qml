#!/usr/bin/env bash
set -euo pipefail

QT_BASE="${QT_BASE:-}"
if [[ -z "$QT_BASE" ]]; then
  for candidate in /Users/ayurash/Development/Qt/*/macos; do
    if [[ -f "$candidate/lib/cmake/Qt6/Qt6Config.cmake" ]]; then
      QT_BASE="$candidate"
    fi
  done
fi
if [[ -z "$QT_BASE" ]]; then
  echo "ERROR: Qt not found. Set QT_BASE=/path/to/Qt/<version>/macos"
  exit 1
fi

APP_NAME="PP18_VideoTools"
BUILD_DIR="release/${APP_NAME}_macos-arm"
APP_BUNDLE="${BUILD_DIR}/${APP_NAME}.app"
QT_CMAKE="${QT_BASE}/lib/cmake"
MACDEPLOYQT="${QT_BASE}/bin/macdeployqt"

command -v cmake >/dev/null 2>&1 || { echo "ERROR: cmake not found in PATH"; exit 1; }

if command -v ninja >/dev/null 2>&1; then
  GEN="-G Ninja"
else
  GEN="-G Unix Makefiles"
fi

cmake -S . -B "$BUILD_DIR" $GEN \
  -DCMAKE_PREFIX_PATH="$QT_CMAKE" \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_OSX_ARCHITECTURES=arm64

cmake --build "$BUILD_DIR" --config Release

if [[ -x "$MACDEPLOYQT" ]]; then
  "$MACDEPLOYQT" "$APP_BUNDLE" -qmldir="$PWD/src/qml" -verbose=1
else
  echo "WARN: macdeployqt not found at: $MACDEPLOYQT"
fi

echo
echo "[OK] Build finished:"
echo " - Bundle: $APP_BUNDLE"
