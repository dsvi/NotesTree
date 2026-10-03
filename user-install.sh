#!/usr/bin/bash

set -e

cd "$(dirname "$0")"

BIN_DIR="$HOME/.local/bin"
DESKTOP_DIR="$HOME/.local/share/applications"
ICON_DIR="$HOME/.local/share/icons/hicolor/scalable/apps"

# Build in docker (same image as docker-build.sh).
echo "Building NotesTree in docker (this takes a while)..."
docker build --tag notestree-build .

# Pull just the bare static binary out of the image.
tmp=$(mktemp -d)
id=$(docker create notestree-build)
docker cp "$id:/src/build/NotesTree" "$tmp/NotesTree"
docker rm "$id"
docker image rm notestree-build

# Install into ~/.local (no root needed).
mkdir -p "$BIN_DIR" "$DESKTOP_DIR" "$ICON_DIR"
install -m 755 "$tmp/NotesTree" "$BIN_DIR/NotesTree"
rm -rf "$tmp"

# Desktop entry with an absolute Exec path, so it works even when
# ~/.local/bin is not in PATH; Icon=NotesTree resolves to the svg below.
# Plus an Uninstall desktop action (a submenu entry in the launcher's
# app menu) that deletes the three installed files and refreshes the
# caches. It is added here rather than in NotesTree.desktop itself so
# that the AppImage, which uses the same source file, does not get it.
{
    sed "s|^Exec=.*|Exec=$BIN_DIR/NotesTree|" NotesTree.desktop
    cat <<EOF
Actions=Uninstall;

[Desktop Action Uninstall]
Name=Uninstall Notes Tree
Exec=sh -c "rm -f $BIN_DIR/NotesTree $DESKTOP_DIR/NotesTree.desktop $ICON_DIR/NotesTree.svg; update-desktop-database $DESKTOP_DIR 2>/dev/null; gtk-update-icon-cache $HOME/.local/share/icons/hicolor 2>/dev/null; true"
EOF
} > "$DESKTOP_DIR/NotesTree.desktop"
install -m 644 NotesTree.svg "$ICON_DIR/NotesTree.svg"

# Best-effort cache refresh so the launcher picks it up immediately.
command -v update-desktop-database >/dev/null 2>&1 \
    && update-desktop-database "$DESKTOP_DIR" || true
command -v gtk-update-icon-cache >/dev/null 2>&1 \
    && gtk-update-icon-cache "$HOME/.local/share/icons/hicolor" || true

HI='\033[0;32m'
NC='\033[0m' # No Color

echo
echo  =^..^=   =^..^=   =^..^=    =^..^=    =^..^=    =^..^=    =^..^=    =^..^=    =^..^=
echo
echo -e Installed:
echo -e "  binary : ${HI}$BIN_DIR/NotesTree${NC}"
echo -e "  menu   : ${HI}$DESKTOP_DIR/NotesTree.desktop${NC}"
echo -e "  icon   : ${HI}$ICON_DIR/NotesTree.svg${NC}"
echo
echo -e "Notes Tree should now appear in your application launcher."
echo -e "The binary is fully static (musl), Wayland only, software rendered."
echo -e "To uninstall: pick ${HI}Uninstall${NC} in the Notes Tree launcher menu,"
echo -e "or delete those three files manually."
