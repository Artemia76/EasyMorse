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

# Same machine as the packages: x86_64 or aarch64 (Raspberry Pi and other ARM boards)
arch=$(uname -m)
case "$arch" in
    x86_64)
        tool_sha256=ed4ce84f0d9caff66f50bcca6ff6f35aae54ce8135408b3fa33abfc3cb384eb0
        runtime_sha256=2fca8b443c92510f1483a883f60061ad09b46b978b2631c807cd873a47ec260d ;;
    aarch64)
        tool_sha256=f0837e7448a0c1e4e650a93bb3e85802546e60654ef287576f46c71c126a9158
        runtime_sha256=00cbdfcf917cc6c0ff6d3347d59e0ca1f7f45a6df1a428a0d6d8a78664d87444 ;;
    *)
        echo "no AppImage tools for $arch" >&2
        exit 1 ;;
esac
tool_url=https://github.com/AppImage/appimagetool/releases/download/1.9.1/appimagetool-$arch.AppImage
runtime_url=https://github.com/AppImage/type2-runtime/releases/download/20251108/runtime-$arch

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
ARCH=$arch VERSION=$version APPIMAGE_EXTRACT_AND_RUN=1 "$work_dir/appimagetool" \
    --runtime-file "$work_dir/runtime" \
    "$app_dir" "$output_dir/easymorse-$version-$arch.AppImage"
