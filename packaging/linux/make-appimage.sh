#!/usr/bin/env bash
# Build a portable EasyMorse AppImage from a configured and compiled tree.
# Usage: make-appimage.sh <build directory> <output directory>
#
# Qt and its QML modules come from the CMake deployment (EASYMORSE_DEPLOY_QT),
# switched on here only: the .deb and .tar.gz keep relying on the system Qt.
set -euo pipefail

build_dir=$(realpath "$1")
output_dir=$(realpath "$2")
source_dir=$(realpath "$(dirname "$0")/../..")

tool_url=https://github.com/AppImage/appimagetool/releases/download/1.9.1/appimagetool-x86_64.AppImage
tool_sha256=ed4ce84f0d9caff66f50bcca6ff6f35aae54ce8135408b3fa33abfc3cb384eb0
runtime_url=https://github.com/AppImage/type2-runtime/releases/download/20251108/runtime-x86_64
runtime_sha256=2fca8b443c92510f1483a883f60061ad09b46b978b2631c807cd873a47ec260d

work_dir=$(mktemp -d)
cleanup() {
    rm -rf "$work_dir"
    cmake -S "$source_dir" -B "$build_dir" -DEASYMORSE_DEPLOY_QT=OFF > /dev/null
}
trap cleanup EXIT
app_dir=$work_dir/EasyMorse.AppDir

cmake -S "$source_dir" -B "$build_dir" -DEASYMORSE_DEPLOY_QT=ON > /dev/null
cmake --install "$build_dir" --prefix "$app_dir/usr" > /dev/null

# The xcb platform plugin needs it and many desktops do not install it
cursor_library=$(ldconfig -p | awk '/libxcb-cursor\.so\.0/ { path = $NF } END { print path }')
if [ -z "$cursor_library" ]; then
    echo "libxcb-cursor0 is missing on this system" >&2
    exit 1
fi
cp -L "$cursor_library" "$app_dir/usr/lib/"

cp "$source_dir/packaging/linux/easymorse.desktop" "$app_dir/"
cp "$source_dir/gfx/Morse_256x256.png" "$app_dir/easymorse.png"
cat > "$app_dir/AppRun" <<'APPRUN'
#!/bin/sh
here=$(dirname "$(readlink -f "$0")")
exec "$here/usr/bin/easymorse" "$@"
APPRUN
chmod +x "$app_dir/AppRun"

curl -fsSL -o "$work_dir/appimagetool" "$tool_url"
curl -fsSL -o "$work_dir/runtime" "$runtime_url"
sha256sum -c - <<CHECK
$tool_sha256  $work_dir/appimagetool
$runtime_sha256  $work_dir/runtime
CHECK
chmod +x "$work_dir/appimagetool"

version=$(sed -n 's/^CMAKE_PROJECT_VERSION:STATIC=//p' "$build_dir/CMakeCache.txt")
ARCH=x86_64 VERSION=$version APPIMAGE_EXTRACT_AND_RUN=1 "$work_dir/appimagetool" \
    --runtime-file "$work_dir/runtime" \
    "$app_dir" "$output_dir/easymorse-$version-x86_64.AppImage"
