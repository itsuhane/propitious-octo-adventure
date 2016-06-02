#pragma once

#include <Windows.h>

inline void set_crash_dialog_enabled(bool enabled) {
    if (enabled) {
        UINT mode = GetErrorMode();
        SetErrorMode(mode & (~SEM_NOGPFAULTERRORBOX));
    }
    else {
        UINT mode = GetErrorMode();
        SetErrorMode(mode | SEM_NOGPFAULTERRORBOX);
    }
}
