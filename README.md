> [!WARNING]
> This project is deprecated. Use
> [jasoncodes/msys2-user-sshd](https://github.com/jasoncodes/msys2-user-sshd)
> instead.

# ssh-shell - Windows OpenSSH Shell Wrapper for MSYS2/MinGW

`msys2-user-sshd` leaves the Windows SSH server on port 22, using either its
default shell or PowerShell, and runs a separate SSH server inside the MSYS2
environment on port 2222. Connections to the MSYS2 server are much faster,
have better PTY emulation, and can access the interactive Windows session.

There is no reason to use this project for new installations unless you
specifically want the Windows SSH server on port 22 to use MSYS2 as its default
shell. Most users do not need that configuration.

A lightweight C wrapper to use MSYS2 bash as the default shell for [Win32-OpenSSH](https://github.com/PowerShell/Win32-OpenSSH) without the Ctrl-C batch file issue.

## Problem

When using a batch file as the default shell for Win32-OpenSSH (as described in the [MSYS2 SSHd setup guide](https://www.msys2.org/wiki/Setting-up-SSHd/)), pressing Ctrl-C during an SSH session causes a "Terminate batch job (Y/N)?" prompt when closing the connection. This wrapper eliminates that issue by using a native Windows executable instead of a batch file.

## Features

- Properly handles both terminal (`ssh -t`) and non-terminal (`ssh`) modes.
- Correctly deals with Windows command-line escaping for bash including backslashes, quotes, and spaces.
- Passes explicit `powershell`, `powershell.exe`, `pwsh`, and `pwsh.exe` commands directly to Windows so PowerShell metacharacters are not consumed by bash.
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

1. Copy `ssh-shell.exe` and `ssh-server.exe` to `C:\msys64\`
2. Import `ssh-shell.reg` to set the OpenSSH `DefaultShell` registry key

## Running GUI apps

OpenSSH runs as background service by default meaning you cannot use it run GUI apps.
Some applications will run okay headless but most will run into issues and not work.
The easiest way is to run OpenSSH via Task Scheduler at startup, optionally on a second port.

1. Open "Task Scheduler" and select "Task Scheduler Library".
2. Click "Create Task…" and set the following:
  * General:
    * Name: `SSH server`
  * Triggers, New:
    * Begin the task: `At log on`
  * Actions, New:
    * Program/script: `C:\msys64\ssh-server.exe`
    * Add arguments: `-o Port=2222`
3. Log off and on or "Start" the task manually.
4. Test with `ssh -p 2222 windows-host notepad`.

## License

MIT
