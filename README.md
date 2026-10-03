# About

NotesTree is a note taking app, which organizes notes in a hierarchical (tree like) structure.

What makes it different?

* Very simple file structure. Your notes are easily available even without NotesTree installed. They are stored as html files. Just open them in any web browser, including the one on your mobile phone.
* You can store complete web pages, simply by copy-pasting a page into a note. NotesTree automatically downloads all the images, making the note available offline.
* You can attach files of any types to your notes.

![screenshot](screenshot.png) 

# Getting it

You can get the wayland only AppImage for x86-64, arm64 and riscv64 at the releases section.

Or build it yourself.

The docker build (`./docker-build.sh`) needs nothing but docker: it builds a fully static musl binary (plus an AppImage of it) for the host architecture.
