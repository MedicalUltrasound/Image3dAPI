#include "../Image3dAPI/ComSupport.hpp"
#include "../Image3dAPI/IImage3d.h"
#include "../Image3dAPI/RegistryCheck.hpp"
#include <chrono>
#include <iostream>
#include <fstream>
#include <set>
#include <thread>
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

    Cart3dGeom bbox = {};
    CHECK(source.GetBoundingBox(&bbox));

    if (verbose) {
        std::cout << "Bounding box:\n";
        std::cout << "  Origin: " << bbox.origin_x << ", " << bbox.origin_y << ", " << bbox.origin_z << "\n";
        std::cout << "  Dir1:   " << bbox.dir1_x   << ", " << bbox.dir1_y   << ", " << bbox.dir1_z   << "\n";
        std::cout << "  Dir2:   " << bbox.dir2_x   << ", " << bbox.dir2_y   << ", " << bbox.dir2_z   << "\n";
        std::cout << "  Dir3:   " << bbox.dir3_x   << ", " << bbox.dir3_y   << ", " << bbox.dir3_z   << "\n";
    }

    unsigned int frame_count = 0;
    CHECK(source.GetFrameCount(&frame_count));
    std::wcout << L"Frame count: " << frame_count << L"\n";

    CComSafeArray<double> frame_times;
    {
        SAFEARRAY * data = nullptr;
        CHECK(source.GetFrameTimes(&data));
        frame_times.Attach(data);
        data = nullptr;
    }

    if (verbose)
    {
        std::cout << "Frame times:\n";
        for (unsigned int i = 0; i < frame_times.GetCount(); i++) {
            double time = frame_times[(int)i];
            std::cout << "  " << time << "\n";
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
        CHECK(source.GetFrame(frame, bbox, max_res, &data));

        if (frame == 0)
            std::cout << "First frame time: " << data.time << "\n";
        if (frame == frame_count-1)
            std::cout << "Last frame time: " << data.time << "\n";
    }
}

CComPtr<IImage3dFileLoader> CreateLoader(const CComBSTR &progid, const CLSID clsid, const bool profile)
{
    CComPtr<IImage3dFileLoader> loader;
    std::wcout << L"Creating loader " << progid.m_str << L" in low-integrity mode...\n";
    LowIntegrity low_integrity;
    PerfTimer timer("CoCreateInstance", profile);
    HRESULT hr = loader.CoCreateInstance(clsid, nullptr, CLSCTX_LOCAL_SERVER | CLSCTX_ENABLE_CLOAKING);
    if (FAILED(hr)) {
        _com_error err(hr);
        std::wcerr << L"CoCreateInstance failed: code=" << hr << L", message=" << err.ErrorMessage() << std::endl;
        exit(-1);
    }

    return loader;
}

CComPtr<IImage3dSource> LoadFileAndGetImageSource(const CComPtr<IImage3dFileLoader>& loader, const CComBSTR& filename, const bool profile)
{
    {
        // load file
        Image3dError err_type = {};
        CComBSTR err_msg;
        PerfTimer timer("LoadFile", profile);
        HRESULT hr = loader->LoadFile(filename, &err_type, &err_msg);
        if (FAILED(hr)) {
            std::wcerr << L"LoadFile failed: code=" << err_type << L", message=" << err_msg.m_str << std::endl;
            exit(-1);
        }
    }

    CComPtr<IImage3dSource> source;
    {
        PerfTimer timer("GetImageSource", profile);
        HRESULT hr = loader->GetImageSource(&source);
        if (FAILED(hr)) {
            _com_error err(hr);
            std::wcerr << L"GetImageSource failed: code=" << hr << L", message=" << err.ErrorMessage() << std::endl;
            exit(-1);
        }
    }

    return source;
}

int wmain(int argc, wchar_t *argv[]) {
    if (argc < 3) {
        std::wcout << L"Usage:\n";
        std::wcout << L"SandboxTest.exe <loader-progid> <filename> [-verbose|-profile] [-threading]" << std::endl;
        return -1;
    }

    CComBSTR progid = argv[1];  // e.g. "DummyLoader.Image3dFileLoader"
    CComBSTR filename = argv[2];

    std::set<std::wstring> options;
    for (int i = 3; i < argc; ++i) {
        options.insert(argv[i]);
    }

    bool verbose = options.find(L"-verbose") != options.end(); // more extensive logging
    bool profile = options.find(L"-profile") != options.end(); // profile loader performance
    bool test_threading = options.find(L"-threading") != options.end(); // instantiate loader, load file and get image source in a separate thread

    bool test_locked_input = true;

    std::ifstream locked_file;
    if (test_locked_input) {
        std::wcout << L"Open file: " << argv[2] << " to test read-locked input file in loader." << std::endl;
        locked_file.open(filename);
    }

    ComInitialize com(COINIT_APARTMENTTHREADED);

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
    CComPtr<IImage3dSource> source;

    if (!test_threading) {
        loader = CreateLoader(progid, clsid, profile);
        source = LoadFileAndGetImageSource(loader, filename, profile);
    } else {
        std::wcout << L"Instantiate loader, load file and get image source in a separate thread" << std::endl;
        CComPtr<IStream> stream;

        std::thread loader_thread([&]() {
            ComInitialize com_thread(COINIT_APARTMENTTHREADED);
            loader = CreateLoader(progid, clsid, profile);

            CComPtr<IImage3dSource> internal_source = LoadFileAndGetImageSource(loader, filename, profile);

            CHECK(CoMarshalInterThreadInterfaceInStream(__uuidof(IImage3dSource), internal_source, &stream));
        });

        loader_thread.join();

        CHECK(CoGetInterfaceAndReleaseStream(stream.Detach(), __uuidof(IImage3dSource), (void**)&source));
    }

    {
        CComBSTR sopInstanceUID;
        CHECK(source->GetSopInstanceUID(&sopInstanceUID));
        std::wcout << L"SOP Instance UID: " << sopInstanceUID.m_str << L"\n";
    }

    ProbeInfo probe;
    CHECK(source->GetProbeInfo(&probe));
    std::wcout << L"Probe name: " << probe.name.m_str << L"\n";

    EcgSeries ecg;
    CHECK(source->GetECG(&ecg));

    if (verbose) {
        CComSafeArray<float> samples;
        samples.Attach(ecg.samples); // transfer ownership
        ecg.samples = nullptr;

        std::wcout << L"ECG sample count: " << samples.GetCount() << L"\n";
        std::wcout << L"First ECG sample time: " << ecg.start_time << L"\n";
        std::wcout << L"Last ECG sample time:  " << ecg.start_time + (samples.GetCount()-1)*ecg.delta_time << L"\n";

        CComSafeArray<double> trig_times;
        trig_times.Attach(ecg.trig_times); // transfer ownership
        ecg.trig_times = nullptr;

        std::wcout << L"ECG QRS trig times:\n";
        for (unsigned int i = 0; i < trig_times.GetCount(); ++i)
            std::wcout << L"  " << trig_times[(int)i] << "\n";
    }

    {
        PerfTimer timer("ParseSource", profile);
        ParseSource(*source, verbose, profile);
    }

    return 0;
}
