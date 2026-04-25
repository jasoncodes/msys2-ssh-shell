#include <windows.h>
#include <wchar.h>

static void delete_first_arg(wchar_t *s) {
    wchar_t *p = s;

    if (*p == L'"') {
        p++;
        while (*p && *p != L'"') p++;
        if (*p == L'"') p++;
    } else {
        while (*p && *p != L' ') p++;
    }

    while (*p == L' ') p++;
    wmemmove(s, p, wcslen(p) + 1);
}

int WINAPI wWinMain(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    PWSTR pCmdLine,
    int nCmdShow
) {
    const wchar_t *sshd_path = L"C:\\Windows\\System32\\OpenSSH\\sshd.exe";
    const wchar_t *raw = GetCommandLineW();

    wchar_t tail[8192];
    wcsncpy(tail, raw, 8191);
    tail[8191] = L'\0';
    delete_first_arg(tail);

    wchar_t cmdline[16384];
    swprintf(cmdline, 16384, L"\"%ls\" -D %ls", sshd_path, tail);

    STARTUPINFOW si;
    PROCESS_INFORMATION pi;
    JOBOBJECT_EXTENDED_LIMIT_INFORMATION job_info;
    ZeroMemory(&si, sizeof(si));
    ZeroMemory(&pi, sizeof(pi));
    ZeroMemory(&job_info, sizeof(job_info));
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;

    HANDLE job = NULL;
    job = CreateJobObjectW(NULL, NULL);
    if (job == NULL) {
        return 255;
    }

    job_info.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
    if (!SetInformationJobObject(
        job,
        JobObjectExtendedLimitInformation,
        &job_info,
        sizeof(job_info)
    )) {
        CloseHandle(job);
        return 255;
    }

    if (!CreateProcessW(
        sshd_path,
        cmdline,
        NULL,
        NULL,
        FALSE,
        CREATE_NO_WINDOW | CREATE_SUSPENDED,
        NULL,
        NULL,
        &si,
        &pi
    )) {
        CloseHandle(job);
        return 255;
    }

    if (!AssignProcessToJobObject(job, pi.hProcess)) {
        TerminateProcess(pi.hProcess, 255);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        CloseHandle(job);
        return 255;
    }

    ResumeThread(pi.hThread);

    WaitForSingleObject(pi.hProcess, INFINITE);

    DWORD exitCode = 0;
    if (!GetExitCodeProcess(pi.hProcess, &exitCode)) {
        exitCode = 255;
    }

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    CloseHandle(job);

    return (int)exitCode;
}
