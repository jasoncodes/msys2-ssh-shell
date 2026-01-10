# ssh-shell - Windows OpenSSH Shell Wrapper for MSYS2/MinGW

A lightweight C wrapper to use MSYS2 bash as the default shell for [Win32-OpenSSH](https://github.com/PowerShell/Win32-OpenSSH) without the Ctrl-C batch file issue.

## Problem

When using a batch file as the default shell for Win32-OpenSSH (as described in the [MSYS2 SSHd setup guide](https://www.msys2.org/wiki/Setting-up-SSHd/)), pressing Ctrl-C during an SSH session causes a "Terminate batch job (Y/N)?" prompt when closing the connection. This wrapper eliminates that issue by using a native Windows executable instead of a batch file.

## Features

- Properly handles both terminal (`ssh -t`) and non-terminal (`ssh`) modes.
- Correctly deals with Windows command-line escaping for bash including backslashes, quotes, and spaces.
- Automatically redirects SFTP to MSYS2's `sftp-server` for proper path handling.
- Minimal overhead via a single native Windows executable instead of multiple scripts.

## Prerequisites

Install the UCRT64 build tools in MSYS2:

```bash
make install-deps
```

Or manually:

```bash
pacman -S base-devel mingw-w64-ucrt-x86_64-gcc
```

## Building

```bash
make
```

## Installation

```bash
make install
```

This will:

1. Copy `ssh-shell.exe` to `C:\msys64\`
2. Import `ssh-shell.reg` to set the OpenSSH `DefaultShell` registry key

## License

MIT
