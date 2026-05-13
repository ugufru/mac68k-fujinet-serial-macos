# Building with Retro68 (work in progress)

This project's existing `.proj.bin` files target THINK C. Retro68 is a parallel
toolchain we're standing up so the project can be built from a Unix shell on
modern macOS/Linux. Porting the THINK C sources to Retro68 (resolving the
`THINK_C` / `THINK_CPLUS` conditional paths in `FujiCommon/FujiInterfaces.h`
and friends) is tracked separately from the toolchain itself.

## Toolchain setup (macOS, Apple Silicon)

### 1. Install Homebrew prerequisites

```sh
brew install cmake gmp mpfr libmpc bison texinfo
brew install boost@1.85          # NOT plain `boost` — see "Gotchas" below
```

`bison` and `boost@1.85` are keg-only; they don't go on PATH automatically.

### 2. Clone Retro68 with submodules

```sh
git clone --recursive https://github.com/autc04/Retro68.git ~/github/Retro68
# or, if already cloned:
cd ~/github/Retro68 && git submodule update --init --recursive
```

### 3. Build the toolchain

```sh
mkdir -p ~/Retro68-build && cd ~/Retro68-build
PATH="/opt/homebrew/opt/bison/bin:$PATH" \
  CMAKE_PREFIX_PATH="/opt/homebrew/opt/boost@1.85" \
  BOOST_ROOT="/opt/homebrew/opt/boost@1.85" \
  Boost_DIR="/opt/homebrew/opt/boost@1.85/lib/cmake/Boost-1.85.0" \
  bash ~/github/Retro68/build-toolchain.bash
```

Takes 30–60 minutes and produces ~5 GB under `~/Retro68-build/`. The compilers
and tools land in `~/Retro68-build/toolchain/bin/`.

### 4. Put the toolchain on PATH

Append to `~/.zshrc`:

```sh
export PATH="$HOME/Retro68-build/toolchain/bin:$PATH"
```

### 5. Smoke test

```sh
m68k-apple-macos-gcc --version   # → "m68k-apple-macos-gcc (GCC) 12.2.0"
```

Build the bundled HelloWorld sample. Note: the sample's `CMakeLists.txt` has
no `project()` / `cmake_minimum_required()` (it expects to be added as a
subdirectory of the master Retro68 build), so a tiny wrapper is needed when
building it standalone:

```sh
mkdir -p /tmp/retro68-hello/src && cd /tmp/retro68-hello
cat > src/CMakeLists.txt <<'EOF'
cmake_minimum_required(VERSION 3.9)
project(HelloWorldTest C CXX)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../helloworld helloworld)
EOF
ln -s ~/github/Retro68/Samples/HelloWorld helloworld
mkdir build && cd build
cmake ../src \
  -DCMAKE_TOOLCHAIN_FILE=$HOME/Retro68-build/toolchain/m68k-apple-macos/cmake/retro68.toolchain.cmake
cmake --build .
ls helloworld/HelloWorld.*
# → HelloWorld.bin  HelloWorld.APPL  HelloWorld.dsk  HelloWorld.code.bin ...
```

The toolchain file lives under `m68k-apple-macos/cmake/`, not `cmake/`, despite
what the sample's comment header says. PowerPC equivalents are in
`powerpc-apple-macos/cmake/{retroppc,retrocarbon}.toolchain.cmake`.

## Gotchas we hit

- **Boost 1.90 (current Homebrew default) breaks Retro68's CMake.** Boost
  removed `boost_system` as a separate library; Retro68's
  `find_package(Boost COMPONENTS system ...)` fails with
  `Could not find a package configuration file provided by "boost_system"`.
  Install `boost@1.85` and point `BOOST_ROOT` / `Boost_DIR` at it.
- **Stale CMake cache.** If the host-tools step failed once, `build-host/`
  caches the broken `Boost_DIR`. `rm -rf ~/Retro68-build/build-host` before
  re-running.
- **System bison is too old** (macOS ships 2.3, Retro68 needs ≥3.0.2).
  Brew's `bison` is keg-only — prepend `/opt/homebrew/opt/bison/bin` to PATH
  for the build invocation.
- **MacTCP is not in Multiversal Interfaces.** Retro68 ships with the
  open-source Multiversal headers, which omit MacTCP, OpenTransport, Carbon,
  Navigation Services, etc. `FujiTests/TCPTests.c` uses MacTCP, so building
  it with Retro68 requires Apple Universal Interfaces 3.x dropped into
  `~/github/Retro68/InterfacesAndLibraries/` before re-running
  `build-toolchain.bash`. Tracked as issue #2.

## Verification

- [x] `m68k-apple-macos-gcc --version` reports GCC 12.2.0
- [x] HelloWorld sample produces `HelloWorld.APPL` and `HelloWorld.dsk`
- [x] `HelloWorld.dsk` boots in Mini vMac (Plus, System 7.x boot floppy) and prints "Hello, world." in a Retro68 Console window
