# Dev containers

Two containers, both Alpine (musl), both for Qt Creator's Development
Container support (and for VS Code / the `devcontainers` CLI).

| folder | base | Qt | what it is for |
| --- | --- | --- | --- |
| `.devcontainer/` | alpine:3.23 | 6.10.3 from apk, **shared** | day to day: links in a second, unit tests, gdb/lldb, the app on your desktop |
| `.devcontainer/static/` | alpine:3.21 | 6.12.0 built from source, **static** | the configuration that ships: `-DNOTES_STATIC_BUILD=ON`, the same binary `./docker-build.sh` produces |

`.devcontainer/devcontainer.json` is the default one.

## Using them from Qt Creator

Open the project, then open `devcontainer.json` in the editor: the plugin puts
a container action in the editor's tool bar (start / restart / rebuild). The
first start builds the image; `run-processes-in-terminal: true` makes that
`docker build` show up in a terminal window so you can watch it.

The kits come from the file rather than from PATH probing
(`auto-detect-kits: false`): one per toolchain - GCC + gdb, clang + lldb - all
with the Ninja generator and the container's `qmake6`. Both config folders are
recognised, so the static container is reachable too.

Qt Creator 20 implements a good part of the devcontainers spec - `build`
(`dockerfile`/`context`/`args`), `image`, `dockerComposeFile`, `mounts`,
`runArgs`, `capAdd`, `securityOpt`, `init`, `privileged`, `containerEnv`,
`remoteEnv`, `remoteUser`, `containerUser`, `workspaceFolder`,
`workspaceMount`, `overrideCommand`, the `initializeCommand` /
`onCreateCommand` / `postCreateCommand` / `postStartCommand` hooks, and
`${localEnv:}` / `${localWorkspace:}` substitution - so the files here use
those directly instead of working around a smaller subset. On top of that the
plugin understands `customizations.qt-creator`: `auto-detect-kits`, `kits`
(`name`, `qt`, `compiler.C`/`Cxx`, `cmake.binary`/`generator`, `debugger`) and
the device options `mount-libexec`, `libexec-mount-point`, `copy-cmd-bridge`,
`run-processes-in-terminal`.

`devcontainer.json` is strict JSON - no comments, no trailing commas: Qt
Creator parses it with `QJsonDocument`, which rejects them. This file is where
the prose goes.

### What Qt Creator does for you

From the Docker plugin, on by default (Preferences -> Devices -> Docker):
starts the container with `--entrypoint /bin/sh` (busybox ash is fine), bind
mounts the project directory, mounts its own `libexec` at
`/devcontainer/libexec` for the Command Bridge, mounts the host's
`/tmp/.X11-unix` and `XAUTHORITY`, and adds `--cap-add=SYS_PTRACE
--security-opt seccomp=unconfined` - which is what makes the debuggers work.

musl is not an obstacle: the Command Bridge helper (`cmdbridge-linux-amd64`) is
a *statically linked Go* binary, so it runs in this image exactly as it does on
glibc. Verified by mounting `/usr/lib/qtcreator` into the container and running
it - which is why `mount-libexec` can stay `true`, the faster of the two
options.

### CMake options

A kit cannot carry `-D` flags, so add them in Projects -> Build -> CMake
Configuration:

```
-DNOTES_BUILD_TESTS=ON                     # dev container only, see below
-DNOTES_STATIC_BUILD=ON                    # static container only
-DCMAKE_CXX_COMPILER_LAUNCHER=ccache       # optional, ccache is installed
```

### The GUI

The `mounts` entry in both files bind mounts the host's `$XDG_RUNTIME_DIR`
(wayland socket + D-Bus session bus) into the container at `/run/user/1000`,
and `containerEnv`/`remoteEnv` point `XDG_RUNTIME_DIR` and `WAYLAND_DISPLAY` at
it. That is what lets the app run natively on Wayland from inside the container
- verified: the app built in the container opens its window on the host's KWin
session, and the session bus is reachable, which is how it picks up the Plasma
theme.

Notes:

* The mount target is `/run/user/1000` to match the image's default
  `USER_UID=1000`; if your uid differs, change both (`ARG USER_UID` in the
  Dockerfile and the target/env paths in the JSON). Qt Creator does not pass
  `build.args` from `devcontainer.json`, so the uid has to be baked into the
  image.
* If `$XDG_RUNTIME_DIR` is not set where you start the container (no
  systemd-logind session), drop the `mounts`, `containerEnv` and `remoteEnv`
  keys - everything else still works.
* X11 also works with no configuration at all: Qt Creator mounts the X socket
  and Alpine's Qt ships the xcb plugin, so the app lands in the host's
  XWayland.
* Headless: `-e QT_QPA_PLATFORM=offscreen` works.

## Tests

```sh
cmake -B build -G Ninja -DNOTES_BUILD_TESTS=ON && cmake --build build
ctest --test-dir build --output-on-failure
```

Only in the default container: the release Qt is configured with
`QT_FEATURE_testlib=OFF`, so `tests/` cannot be configured in the static one.

## The static container

`.devcontainer/static/Dockerfile` repeats the top level `Dockerfile`'s
toolchain steps **byte for byte** up to the qtsvg install, so the two images
share Docker's layer cache: after one `./docker-build.sh`, building this image
reuses the whole Qt compile. If you change the release toolchain there, change
it here too.

It leaves out the AppImage stages and the `COPY . ./` build step (the sources
are bind mounted), and adds the debuggers, `musl-dbg`, `strace` and ccache. Two
things to expect: relinking takes a while (static Qt), and the app is
Wayland-only with no OpenGL/EGL, so it needs the Wayland mount above to run.

## Why Alpine 3.23 for the dev image

Not the 3.21 the release image uses: 3.21 ships Qt 6.8.2 and this code does not
compile against it - `QString::arg(char, ...)` in `Note::decodeFromFilename`
needs Qt >= 6.10. 3.23 has 6.10.3, edge has 6.11.1; both build the project as
it is.

## What was actually checked

* `alpine:3.23` and `alpine:edge` build the project as-is; `alpine:3.21` does
  not (Qt 6.8.2, see above).
* In the dev image, as the non-root container user: configure + build + `ctest`
  (3/3 pass), with the build directory coming back owned by the host user.
* `gdb` and `lldb` both hit a breakpoint in `main` inside the container.
* Qt Creator's Command Bridge binary runs inside the musl container.
* The container stays up under Qt Creator's `--entrypoint /bin/sh` start.
* The app built in the container runs against the host's Wayland session with
  the mount recipe from `devcontainer.json`.
* Qt Creator 20.0.1 loads both `devcontainer.json` files (the container action
  appears in the editor tool bar for each) and the Development Container
  Support plugin is enabled by default.
