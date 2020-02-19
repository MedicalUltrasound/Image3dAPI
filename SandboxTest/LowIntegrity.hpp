#pragma once
#include <Windows.h>
#include <sddl.h>


/** RAII class for temporarily impersonating low-integrity level for the current thread.
    Intended to be used together with CLSCTX_ENABLE_CLOAKING when creating COM objects.
    Based on "Designing Applications to Run at a Low Integrity Level" https://docs.microsoft.com/en-us/previous-versions/dotnet/articles/bb625960(v=msdn.10) */
struct LowIntegrity {
    LowIntegrity() {
        HANDLE cur_token = nullptr;
        if (!OpenProcessToken(GetCurrentProcess(), TOKEN_DUPLICATE | TOKEN_ADJUST_DEFAULT | TOKEN_QUERY | TOKEN_ASSIGN_PRIMARY, &cur_token))
            abort();

        if (!DuplicateTokenEx(cur_token, 0, NULL, SecurityImpersonation, TokenPrimary, &m_token))
            abort();

        CloseHandle(cur_token);

        PSID li_sid = nullptr;
        if (!ConvertStringSidToSid(L"S-1-16-4096", &li_sid)) // low integrity SID
            abort();

        // reduce process integrity level
        TOKEN_MANDATORY_LABEL TIL = {};
        TIL.Label.Attributes = SE_GROUP_INTEGRITY;
        TIL.Label.Sid = li_sid;
        if (!SetTokenInformation(m_token, TokenIntegrityLevel, &TIL, sizeof(TOKEN_MANDATORY_LABEL) + GetLengthSid(li_sid)))
            abort();

        if (!ImpersonateLoggedOnUser(m_token)) // change current thread integrity
            abort();

        LocalFree(li_sid);
        li_sid = nullptr;
    }

    ~LowIntegrity() {
        if (!RevertToSelf())
            abort();

        CloseHandle(m_token);
        m_token = nullptr;
    }

private:
    HANDLE m_token = nullptr;
};
