#!/bin/sh
# install.sh - Installs hd (Linux ELF binary) from the latest GitHub release of mrmisra/hexdump
set -e

REPO=mrmisra/hexdump
BINARY=hd
INSTALLDIR=/usr/local/bin
TMPDIR=$(mktemp -d)

# Download latest release info and find hd asset
ASSET_URL=$(curl -s "https://api.github.com/repos/$REPO/releases/latest" | \
  grep browser_download_url | grep "$BINARY" | cut -d '"' -f 4)

if [ -z "$ASSET_URL" ]; then
  echo "Could not find $BINARY in the latest release."
  exit 1
fi

# Download the binary
curl -L "$ASSET_URL" -o "$TMPDIR/$BINARY"
chmod +x "$TMPDIR/$BINARY"

# Install to INSTALLDIR
sudo cp "$TMPDIR/$BINARY" "$INSTALLDIR/$BINARY"

# Clean up	
rm -rf "$TMPDIR"

echo "Installed $BINARY to $INSTALLDIR. You can now run 'hd' from anywhere."
