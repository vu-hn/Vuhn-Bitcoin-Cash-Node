# WINDOWS BUILD NOTES

Below are some notes on how to build Vuhn Bitcoin Cash Node for Windows.

Please note that from BCHN v0.21.3 onwards, building for Win32 is no longer
officially supported (and build system capabilities related to this may
be removed).

The options known to work for building Vuhn Bitcoin Cash Node on Windows are:

- On Linux, using the [Mingw-w64](https://www.mingw-w64.org/downloads/) cross compiler
  tool chain. Debian Bookworm is recommended and is the platform used by the
  upstream CI to build the Windows release binaries.
- On Windows, using [Windows Subsystem for Linux (WSL)](https://msdn.microsoft.com/commandline/wsl/about)
  and the Mingw-w64 cross compiler tool chain. This is covered in these notes.

Other options which may work, but which have not been extensively tested are
(please contribute instructions):

- On Windows, using a POSIX compatibility layer application such as
  [cygwin](http://www.cygwin.com/) or [msys2](http://www.msys2.org/).
- On Windows, using a native compiler tool chain such as
  [Visual Studio](https://www.visualstudio.com).

In any case please make sure that the compiler supports C++20, specifically
the `<source_location>` header. This requires **GCC 11+** (GCC 12+ recommended)
or **Clang 16+** for the MinGW cross-compiler.

**Note** These notes cover building binaries from source, for running Vuhn
Bitcoin Cash Node natively under Windows. If you wish to both compile and run
Vuhn Bitcoin Cash Node on Windows, *under WSL*, you can refer to the
[Unix build guide](build-unix.md), and follow those instructions from within WSL.

## Compiler Requirements

The MinGW cross-compiler must support C++20, including `<source_location>`.

| Distribution | MinGW GCC Version | Supported |
| --- | --- | --- |
| Ubuntu 22.04 (Jammy) | GCC 10 | **No** — lacks `<source_location>` |
| Ubuntu 24.04 (Noble) | GCC 13 | Yes |
| Debian 12 (Bookworm) | GCC 12 | Yes (recommended, matches upstream CI) |

## Windows Subsystem for Linux

With Windows 10, Microsoft has released a feature named the [Windows
Subsystem for Linux (WSL)](https://msdn.microsoft.com/commandline/wsl/about). This
feature allows you to run a bash shell directly on Windows in an Ubuntu-based
environment. Within this environment you can cross compile for Windows without
the need for a separate Linux VM or server.

WSL 2 is required. It was released with Windows 10, Version 2004, Build 19041.

WSL is not supported in versions of Windows prior to Windows 10 or on
Windows Server SKUs. In addition, it is available only for 64-bit versions of
Windows.

## Building with WSL 2 and Debian Bookworm

This is the recommended method. Debian Bookworm is preferred because it matches
the upstream CI build environment and ships with MinGW GCC 12.

### Installing Debian on Windows Subsystem for Linux 2

Install WSL 2 and Debian:

```bash
    wsl --install -d Debian
```

You will be asked to create a new UNIX user account. This is a separate
account from your Windows account.

Alternatively, you can use **Ubuntu 24.04** (which has MinGW GCC 13):

```bash
    wsl --install -d Ubuntu-24.04
```

**Important:** Do NOT use Ubuntu 22.04 or older — their MinGW GCC (v10) does
not support the C++20 `<source_location>` header required by this codebase.

### Cross-compilation for WSL 2

The steps below can be performed on Debian/Ubuntu (including in a VM) or WSL 2.
The depends system will also work on other Linux distributions, however the
commands for installing the toolchain will be different.

First, install the general dependencies:

```bash
    sudo apt update
    sudo apt upgrade
    sudo apt install autoconf automake build-essential bsdmainutils cmake curl git libboost-all-dev libevent-dev libssl-dev libtool ninja-build nsis pkg-config python3 libgmp-dev zlib1g-dev
```

A host toolchain (`build-essential`) is necessary because some dependency
packages need to build host utilities that are used in the build process.

See also: [dependencies.md](dependencies.md).

### Building for 64-bit Windows

The first step is to install the `mingw-w64` cross-compilation tool chain.

```bash
    sudo apt install g++-mingw-w64-x86-64
```

Verify the version is 11 or higher:

```bash
    x86_64-w64-mingw32-g++ --version
```

Next, configure the `mingw-w64` to the posix[¹](#footnote1) compiler option.

```bash
    sudo update-alternatives --config x86_64-w64-mingw32-g++ # Set the default mingw32 g++ compiler option to posix.
    sudo update-alternatives --config x86_64-w64-mingw32-gcc # Set the default mingw32 gcc compiler option to posix.
```

Note that for WSL 2 the source path MUST be somewhere in the default mount
file system, for example `/usr/src/vuhn-bitcoin-cash-node`, AND not under
`/mnt/d/`. This means you cannot use a directory that is located directly on
the host Windows file system to perform the build.

Acquire the source in the usual way:

```bash
    git clone https://github.com/vu-hn/Vuhn-Bitcoin-Cash-Node.git
    cd Vuhn-Bitcoin-Cash-Node
```

Once the source code is ready the build steps are below:

```bash
    # Strip out problematic Windows %PATH% imported var (contains spaces and
    # parentheses in paths like "Program Files (x86)" that break the build)
    export PATH=$(echo "$PATH" | sed -e 's/:\/mnt.*//g')

    # WSL2 fix: Tell GMP's configure to use native gcc for build-system
    # programs, not the cross-compiler. Without this, WSL2's Windows interop
    # (which can run .exe files) confuses cross-compilation detection.
    export CC_FOR_BUILD=gcc
    export CXX_FOR_BUILD=g++

    cd depends
    make build-win64
    cd ..
    mkdir build
    cd build
    cmake -GNinja .. -DCMAKE_TOOLCHAIN_FILE=../cmake/platforms/Win64.cmake -DENABLE_MAN=OFF -DBUILD_BITCOIN_SEEDER=OFF # seeder not supported in Windows yet
    ninja
    ninja package #to build the install-package
```

### Installation

After building using the Windows subsystem it can be useful to copy the compiled
executables to a directory on the windows drive in the same directory structure
as they appear in the release `.zip` archive. This can be done in the following
way. This will install to `c:\workspace\vuhn-bitcoin-cash-node`, for example:

```bash
    cmake -GNinja .. -DCMAKE_TOOLCHAIN_FILE=../cmake/platforms/Win64.cmake -DENABLE_MAN=OFF -DBUILD_BITCOIN_SEEDER=OFF -DCMAKE_INSTALL_PREFIX=/mnt/c/workspace/vuhn-bitcoin-cash-node
    sudo ninja install
```

## Depends system

For further documentation on the depends system see [README.md](../depends/README.md)
in the depends directory.

## Footnotes

<a name="footnote1">1</a>: Starting from Ubuntu Xenial 16.04, both the 32 and 64
bit `Mingw-w64` packages install two different compiler options to allow a choice
between either posix or win32 threads. The default option is win32 threads which
is the more efficient since it will result in binary code that links directly with
the Windows kernel32.lib. Unfortunately, the headers required to support win32
threads conflict with some of the classes in the C++11 standard library, in particular
`std::mutex`. It's not possible to build the code using the win32 version of the
Mingw-w64 cross compilers (at least not without modifying headers in the source code).
