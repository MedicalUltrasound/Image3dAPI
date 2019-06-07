#include "../Image3dAPI/ComSupport.hpp"
#include "../Image3dAPI/IImage3d.h"
#include "../Image3dAPI/RegistryCheck.hpp"
#include <chrono>
#include <iostream>
#include <fstream>
#include <sddl.h>


class PerfTimer {
    using clock = std::chrono::high_resolution_clock;
public:
    PerfTimer(const char * prefix, bool enable) : m_prefix(prefix) {
        if (enable)
            m_start = clock::now();

    }
    ~PerfTimer() {
        if (m_start == clock::time_point())
            return; // profiling disabled

        auto stop = clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(stop - m_start).count();
        std::cout << m_prefix << " took " << duration << "ms\n";
    }
private:
    const char *      m_prefix;
    clock::time_point m_start;
};


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


void ParseSource (IImage3dSource & source, bool verbose, bool profile) {
    Cart3dGeom geom = {};
    CHECK(source.GetBoundingBox(&geom));

    if (verbose) {
        std::cout << "Bounding box:\n";
        std::cout << "  Origin: " << geom.origin_x << ", " << geom.origin_y << ", " << geom.origin_z << "\n";
        std::cout << "  Dir1:   " << geom.dir1_x   << ", " << geom.dir1_y   << ", " << geom.dir1_z   << "\n";
        std::cout << "  Dir2:   " << geom.dir2_x   << ", " << geom.dir2_y   << ", " << geom.dir2_z   << "\n";
        std::cout << "  Dir3:   " << geom.dir3_x   << ", " << geom.dir3_y   << ", " << geom.dir3_z   << "\n";
    }

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

    if (verbose) {
        std::cout << "Color-map:\n";
        for (unsigned int i = 0; i < color_map.GetCount(); i++) {
            unsigned int color = color_map[(int)i];
            uint8_t *rgbx = reinterpret_cast<uint8_t*>(&color);
            std::cout << "  [" << (int)rgbx[0] << "," << (int)rgbx[1] << "," << (int)rgbx[2] << "," << (int)rgbx[3] << "]\n";
        }
    }

    for (unsigned int frame = 0; frame < frame_count; ++frame) {
        unsigned short max_res[] = { 64, 64, 64 };
        if (profile) {
            max_res[0] = 128;
            max_res[1] = 128;
            max_res[2] = 128;
        }

        // retrieve frame data
        Image3d data;
        PerfTimer timer("GetFrame", profile);
        CHECK(source.GetFrame(frame, geom, max_res, &data));
    }
}


int wmain (int argc, wchar_t *argv[]) {
    if (argc < 3) {
        std::wcout << L"Usage:\n";
        std::wcout << L"SandboxTest.exe <loader-progid> <filename> [-verbose|-profile]" << std::endl;
        return -1;
    }

    CComBSTR progid = argv[1];  // e.g. "DummyLoader.Image3dFileLoader"
    CComBSTR filename = argv[2];

    bool verbose = false; // more extensive logging
    bool profile = false; // profile loader performance
    if (argc >= 4) {
        if (std::wstring(argv[3]) == L"-verbose")
            verbose = true;
        else if (std::wstring(argv[3]) == L"-profile")
            profile = true;
    }

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
    // first check loader with matching bitness
    REGSAM bitness = WOW_SAME_AS_CLIENT;
    if (FAILED(CheckImage3dAPIVersion(clsid, bitness))) {
        // then check loader with non-matching bitness
#ifdef _WIN64
        bitness = KEY_WOW64_32KEY;
#else
        bitness = KEY_WOW64_64KEY;
#endif
        if (FAILED(CheckImage3dAPIVersion(clsid, bitness))) {
            std::wcerr << L"ERRORR: Loader " << progid.m_str << L" not compatible with current API version.\n";
            return -1;
        }
    }

    // create loader in a separate "low integrity" dllhost.exe process
    CComPtr<IImage3dFileLoader> loader;
    {
        std::wcout << L"Creating loader " << progid.m_str << L" in low-integrity mode...\n";
        LowIntegrity low_integrity;
        PerfTimer timer("CoCreateInstance", profile);
        CHECK(loader.CoCreateInstance(clsid, nullptr, CLSCTX_LOCAL_SERVER | CLSCTX_ENABLE_CLOAKING));
    }

    {
        // load file
        Image3dError err_type = {};
        CComBSTR err_msg;
        PerfTimer timer("LoadFile", profile);
        HRESULT hr = loader->LoadFile(filename, &err_type, &err_msg);
        if (FAILED(hr)) {
            std::wcerr << L"LoadFile failed: code=" << err_type << L", message="<< err_msg.m_str << std::endl;
            return -1;
        }
    }

    CComPtr<IImage3dSource> source;
    {
        PerfTimer timer("GetImageSource", profile);
        CHECK(loader->GetImageSource(&source));
    }

    ProbeInfo probe;
    CHECK(source->GetProbeInfo(&probe));
    std::wcout << "Probe name: " << probe.name.m_str << L"\n";

    EcgSeries ecg;
    CHECK(source->GetECG(&ecg));

    {
        PerfTimer timer("ParseSource", profile);
        ParseSource(*source, verbose, profile);
    }

    return 0;
}
