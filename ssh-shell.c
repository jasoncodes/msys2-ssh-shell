#include <windows.h>
#include <wchar.h>
#include <stdio.h>

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

int wmain(int argc, wchar_t *argv[]) {
    STARTUPINFOW si = { sizeof(si) };
    PROCESS_INFORMATION pi;

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

    // escape for Windows
    wchar_t escaped[9000];
    escape_for_windows(escaped, clean);

    // build final command
    wchar_t cmdline[9000];
    if (clean[0] == L'\0') {
        wcsncpy(cmdline, L"bash.exe -l", 8999);
        cmdline[8999] = L'\0';
    } else {
        swprintf(cmdline, 9000, L"bash.exe -lc %ls", escaped);
    }

    // launch bash
    if (!CreateProcessW(
            L"C:\\msys64\\usr\\bin\\bash.exe",
            cmdline,
            NULL, NULL, FALSE,
            0, NULL, NULL,
            &si, &pi
        )) {
        fwprintf(stderr, L"Failed to launch bash.exe (error %lu)\n", GetLastError());
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
