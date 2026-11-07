#!/usr/bin/env bash
# Builds OpenSSL for Android from the official sources, as Qt for Android
# loads it: libssl_3.so and libcrypto_3.so without version suffix. Qt gives
# no TLS on Android without them (no https:// or wss:// server).
#
# Usage: make-openssl.sh <output dir> [ABI, default arm64-v8a]
# Needs ANDROID_NDK_ROOT. The output dir then holds <ABI>/libssl_3.so,
# <ABI>/libcrypto_3.so and include/; give it to CMake as
# -DEASYMORSE_ANDROID_OPENSSL_DIR=<output dir>.
set -euo pipefail

VERSION=3.5.9   # long term support branch
API=28          # the minimum SDK of EasyMorse
OUT="$(mkdir -p "$1" && cd "$1" && pwd)"
ABI="${2:-arm64-v8a}"
case "$ABI" in
    arm64-v8a)   TARGET=android-arm64 ;;
    armeabi-v7a) TARGET=android-arm ;;
    x86_64)      TARGET=android-x86_64 ;;
    x86)         TARGET=android-x86 ;;
    *) echo "unknown ABI $ABI" >&2; exit 2 ;;
esac
: "${ANDROID_NDK_ROOT:?set ANDROID_NDK_ROOT}"

if [ -f "$OUT/$ABI/libssl_3.so" ] && [ -f "$OUT/$ABI/libcrypto_3.so" ]; then
    echo "OpenSSL $VERSION for $ABI already in $OUT"
    exit 0
fi

WORK="$(mktemp -d)"
trap 'rm -rf "$WORK"' EXIT
cd "$WORK"
BASE="https://github.com/openssl/openssl/releases/download/openssl-$VERSION"
curl -sSfL -o "openssl-$VERSION.tar.gz" "$BASE/openssl-$VERSION.tar.gz"
curl -sSfL -o "openssl-$VERSION.tar.gz.sha256" "$BASE/openssl-$VERSION.tar.gz.sha256"
# The .sha256 file holds the hash, possibly followed by the file name
echo "$(cut -d' ' -f1 "openssl-$VERSION.tar.gz.sha256")  openssl-$VERSION.tar.gz" | sha256sum -c -
# Signature check when gpg is there (keys of the OpenSSL project)
if command -v gpg > /dev/null && curl -sSfL -o "openssl-$VERSION.tar.gz.asc" "$BASE/openssl-$VERSION.tar.gz.asc"; then
    export GNUPGHOME="$WORK/gnupg"; mkdir -m 700 "$GNUPGHOME"
    curl -sSfL "https://raw.githubusercontent.com/openssl/openssl/openssl-$VERSION/doc/fingerprints.txt" -o fingerprints.txt || true
    if gpg --batch --keyserver hkps://keys.openpgp.org --recv-keys \
            $(grep -oE '^[0-9A-F ]{40,}' fingerprints.txt | tr -d ' ') > /dev/null 2>&1; then
        gpg --batch --verify "openssl-$VERSION.tar.gz.asc" "openssl-$VERSION.tar.gz"
    else
        echo "warning: OpenSSL signing keys not fetched, only the SHA-256 was checked" >&2
    fi
fi

tar xzf "openssl-$VERSION.tar.gz"
cd "openssl-$VERSION"
HOST_TAG=linux-x86_64
[ "$(uname)" = Darwin ] && HOST_TAG=darwin-x86_64
export PATH="$ANDROID_NDK_ROOT/toolchains/llvm/prebuilt/$HOST_TAG/bin:$PATH"
# Qt loads libssl_3.so / libcrypto_3.so: the "_3" variant, no version suffix
cat > Configurations/99-easymorse-android.conf <<CONF
my %targets = (
    "easymorse-$TARGET" => {
        inherit_from  => [ "$TARGET" ],
        shlib_variant => "_3",
    },
);
CONF
./Configure "easymorse-$TARGET" shared no-tests no-docs no-apps -D__ANDROID_API__=$API > configure.log
make -j"$(getconf _NPROCESSORS_ONLN)" SHLIB_VERSION_NUMBER= build_libs > make.log 2>&1 \
    || { tail -30 make.log; exit 1; }
mkdir -p "$OUT/$ABI"
cp libssl_3.so libcrypto_3.so "$OUT/$ABI/"
rm -rf "$OUT/include" && cp -R include "$OUT/include"
echo "$VERSION" > "$OUT/VERSION"
echo "OpenSSL $VERSION for $ABI in $OUT/$ABI"
