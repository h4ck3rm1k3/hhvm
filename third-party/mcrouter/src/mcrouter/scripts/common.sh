set -ex

die() { printf "%s: %s\n" "$0" "$@"; exit 1; }
grab() {
	DEST=${2:-$(basename "$1")}
	wget --no-check-certificate "$1" -O "$DEST"
}

[ -n "$1" ] || die "PKG_DIR missing"
[ -n "$2" ] || die "INSTALL_DIR missing"

PKG_DIR=$1
INSTALL_DIR=$2
MAKE_ARGS=$3
SCRIPT_DIR=$(cd $(dirname "$0") && pwd)

mkdir -p "$PKG_DIR" "$INSTALL_DIR"

cd "$PKG_DIR"
