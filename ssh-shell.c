#include <windows.h>
#include <wchar.h>
#include <stdio.h>

static int is_console_handle(HANDLE h) {
    DWORD mode = 0;
    return h != NULL && h != INVALID_HANDLE_VALUE && GetConsoleMode(h, &mode);
}

static int stdio_uses_console(void) {
    return is_console_handle(GetStdHandle(STD_INPUT_HANDLE)) ||
           is_console_handle(GetStdHandle(STD_OUTPUT_HANDLE)) ||
           is_console_handle(GetStdHandle(STD_ERROR_HANDLE));
}

static void delete_first_arg(wchar_t *s) {
    wchar_t *p = s;

    if (*p == L'"') {
        // skip quoted argument
        p++;
        while (*p && *p != L'"') p++;
        if (*p == L'"') p++;
    } else {
        // skip unquoted argument
        while (*p && *p != L' ') p++;
    }

    // skip trailing space
    if (*p == L' ') p++;

    wmemmove(s, p, wcslen(p) + 1);
}

static void escape_for_windows(wchar_t *dst, const wchar_t *src)
{
    *dst++ = L'"';

    while (*src) {
        // count backslashes
        int backslash_count = 0;
        const wchar_t *p = src;
        while (*p == L'\\') {
            backslash_count++;
            p++;
        }

        if (backslash_count > 0) {
            if (*p == L'"') {
                // backslashes followed by quote: double them + one more to escape the quote
                for (int i = 0; i < backslash_count * 2 + 1; i++) {
                    *dst++ = L'\\';
                }
                *dst++ = L'"';
                src = p + 1;
            } else {
                // backslashes not followed by quote: double them
                for (int i = 0; i < backslash_count * 2; i++) {
                    *dst++ = L'\\';
                }
                src = p;
            }
        } else if (*src == L'"') {
            // quote with no preceding backslashes: escape it
            *dst++ = L'\\';
            *dst++ = L'"';
            src++;
        } else {
            // normal character
            *dst++ = *src++;
        }
    }

    *dst++ = L'"';
    *dst++ = L'\0';
}

static const wchar_t *first_arg_basename(const wchar_t *s, wchar_t *buf, size_t buflen)
{
    const wchar_t *arg_start = s;
    const wchar_t *arg_end;
    const wchar_t *base_start;
    size_t len;

    while (*arg_start == L' ') arg_start++;

    if (*arg_start == L'"') {
        arg_start++;
        arg_end = arg_start;
        while (*arg_end && *arg_end != L'"') arg_end++;
    } else {
        arg_end = arg_start;
        while (*arg_end && *arg_end != L' ') arg_end++;
    }

    base_start = arg_end;
    while (base_start > arg_start &&
           base_start[-1] != L'\\' &&
           base_start[-1] != L'/') {
        base_start--;
    }

    len = (size_t)(arg_end - base_start);
    if (len >= buflen) len = buflen - 1;
    wcsncpy(buf, base_start, len);
    buf[len] = L'\0';

    return buf;
}

static int is_powershell_command(const wchar_t *cmd)
{
    wchar_t exe[260];
    first_arg_basename(cmd, exe, sizeof(exe) / sizeof(exe[0]));

    return _wcsicmp(exe, L"powershell") == 0 ||
           _wcsicmp(exe, L"powershell.exe") == 0 ||
           _wcsicmp(exe, L"pwsh") == 0 ||
           _wcsicmp(exe, L"pwsh.exe") == 0;
}

int wmain(int argc, wchar_t *argv[]) {
    STARTUPINFOW si = { sizeof(si) };
    PROCESS_INFORMATION pi;

    HANDLE hIn = GetStdHandle(STD_INPUT_HANDLE);
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    HANDLE hErr = GetStdHandle(STD_ERROR_HANDLE);

    int hadAttachedConsole = (GetConsoleWindow() != NULL);
    int stdioIsConsole = stdio_uses_console();

    // Detach as early as possible in non-terminal sessions.
    if (!stdioIsConsole) {
        FreeConsole();
    }

    const wchar_t *raw = GetCommandLineW();

    // args are in one of two formats depending on if terminal mode is enabled
    //   c:\msys64\ssh-shell.exe -c date "+%s"
    //   "c:\msys64\ssh-shell.exe" -c "date \"+%s\""
    //
    // If raw starts with a quote, OpenSSH has already Windows-escaped
    // the command and we can use argv[2]. Otherwise, we need to do work.
    int quoted = (raw[0] == L'"');

    wchar_t clean[8192];

    if (quoted) {
        // use parsed arguments as they are already properly unescaped by Windows
        // argv[0] = wrapper path, argv[1] = "-c", argv[2] = command
        wcsncpy(clean, (argc >= 3) ? argv[2] : L"", 8191);
        clean[8191] = L'\0';
    } else {
        // manually parse the raw command line
        wcsncpy(clean, raw, 8191);
        clean[8191] = L'\0';
        delete_first_arg(clean); // remove wrapper path
        delete_first_arg(clean); // remove -c
    }

    // trim trailing spaces
    size_t len = wcslen(clean);
    while (len > 0 && clean[len - 1] == L' ') {
        clean[--len] = L'\0';
    }

    // Replace sftp-server.exe with MSYS2 version for proper path handling
    if (wcscmp(clean, L"sftp-server.exe") == 0) {
        wcscpy(clean, L"/c/msys64/usr/lib/ssh/sftp-server");
    }

    // PowerShell commands are already full Windows command lines. Running them
    // through bash -lc first would consume PowerShell metacharacters like $ and ;.
    wchar_t escaped[9000];
    wchar_t cmdline[9000];
    const wchar_t *app = L"C:\\msys64\\usr\\bin\\bash.exe";
    int directWindowsCommand = 0;

    if (is_powershell_command(clean)) {
        wcsncpy(cmdline, clean, 8999);
        cmdline[8999] = L'\0';
        app = NULL;
        directWindowsCommand = 1;
    } else if (clean[0] == L'\0') {
        wcsncpy(cmdline, L"bash.exe -l", 8999);
        cmdline[8999] = L'\0';
    } else {
        // escape for Windows
        escape_for_windows(escaped, clean);

        // build final command
        swprintf(cmdline, 9000, L"bash.exe -lc %ls", escaped);
    }

    DWORD creationFlags = 0;
    if (!stdioIsConsole) {
        if (hadAttachedConsole && !directWindowsCommand) {
            creationFlags |= DETACHED_PROCESS;
        } else {
            creationFlags |= CREATE_NO_WINDOW;
        }
    }

    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdInput = hIn;
    si.hStdOutput = hOut;
    si.hStdError = hErr;

    // launch selected command processor
    if (!CreateProcessW(
            app,
            cmdline,
            NULL, NULL, TRUE,
            creationFlags, NULL, NULL,
            &si, &pi
        )) {
        fwprintf(stderr, L"Failed to launch command processor (error %lu)\n", GetLastError());
        return 255;
    }

    WaitForSingleObject(pi.hProcess, INFINITE);

    DWORD exitCode = 0;
    if (!GetExitCodeProcess(pi.hProcess, &exitCode)) {
        exitCode = 255;
    }

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    return (int)exitCode;
}
