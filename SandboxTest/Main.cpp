#include "../Image3dAPI/ComSupport.hpp"
#include "../Image3dAPI/IImage3d.h"
#include "../Image3dAPI/RegistryCheck.hpp"
#include <iostream>
#include <fstream>
#include <sddl.h>


/** RAII class for temporarily impersonating low-integrity level for the current thread.
    Intended to be used together with CLSCTX_ENABLE_CLOAKING when creating COM objects.
    Based on "Designing Applications to Run at a Low Integrity Level" https://msdn.microsoft.com/en-us/library/bb625960.aspx */
struct LowIntegrity {
    LowIntegrity()
    {
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

    ~LowIntegrity()
    {
        if (!RevertToSelf())
            abort();

        CloseHandle(m_token);
        m_token = nullptr;
    }

private:
    HANDLE m_token = nullptr;
};


void ParseSource (IImage3dSource & source) {
    Cart3dGeom geom = {};
    CHECK(source.GetBoundingBox(&geom));

    unsigned int frame_count = 0;
    CHECK(source.GetFrameCount(&frame_count));
    std::wcout << L"Frame count: " << frame_count << L"\n";

    CComSafeArray<unsigned int> color_map;
    {
        SAFEARRAY * tmp = nullptr;
        CHECK(source.GetColorMap(&tmp));
        color_map.Attach(tmp);
        tmp = nullptr;
    }

    for (unsigned int frame = 0; frame < frame_count; ++frame) {
        unsigned short max_res[] = {64, 64, 64};

        // retrieve frame data
        Image3d data;
        CHECK(source.GetFrame(frame, geom, max_res, &data));
    }
}


int wmain (int argc, wchar_t *argv[]) {
    if (argc < 3) {
        std::wcout << L"Usage:\n";
        std::wcout << L"SandboxTest.exe <loader-progid> <filename>" << std::endl;
        return -1;
    }

    CComBSTR progid = argv[1];  // e.g. "DummyLoader.Image3dFileLoader"
    CComBSTR filename = argv[2];
    bool test_locked_input = true;

    std::ifstream locked_file;
    if (test_locked_input) {
        std::wcout << L"Open file: " << argv[2] << " to test read-locked input file in loader." << std::endl;
        locked_file.open(filename);
    }

    ComInitialize com(COINIT_MULTITHREADED);

    CLSID clsid = {};
    if (FAILED(CLSIDFromProgID(progid, &clsid))) {
        std::wcerr << L"ERRORR: Unknown progid " << progid.m_str << L"\n";
        return -1;
    }

    auto list = SupportedManufacturerModels::ReadList(clsid);

    // verify that loader library is compatible
    if (FAILED(CheckImage3dAPIVersion(clsid))) {
        std::wcerr << L"ERRORR: Loader " << progid.m_str << L" not compatible with current API version.\n";
        return -1;
    }

    // create loader in a separate "low integrity" dllhost.exe process
    CComPtr<IImage3dFileLoader> loader;
    {
        std::wcout << L"Creating loader " << progid.m_str << L" in low-integrity mode...\n";
        LowIntegrity low_integrity;
        CHECK(loader.CoCreateInstance(clsid, nullptr, CLSCTX_LOCAL_SERVER | CLSCTX_ENABLE_CLOAKING));
    }

    {
        // load file
        Image3dError err_type = {};
        CComBSTR err_msg;
        HRESULT hr = loader->LoadFile(filename, &err_type, &err_msg);
        if (FAILED(hr)) {
            std::wcerr << L"LoadFile failed: code=" << err_type << L", message="<< err_msg.m_str << std::endl;
            return -1;
        }
    }

    CComPtr<IImage3dSource> source;
    CHECK(loader->GetImageSource(&source));

    ProbeInfo probe;
    CHECK(source->GetProbeInfo(&probe));

    EcgSeries ecg;
    CHECK(source->GetECG(&ecg));

    ParseSource(*source);

    return 0;
}
