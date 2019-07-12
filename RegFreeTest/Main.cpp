#include "../Image3dAPI/ComSupport.hpp"
#include "../Image3dAPI/IImage3d.h"
#include "../Image3dAPI/RegistryCheck.hpp"
#include <iostream>


void ParseSource (IImage3dSource & source) {
    CComSafeArray<uint8_t> color_map;
    {
        ImageFormat img_format = IMAGE_FORMAT_INVALID;
        SAFEARRAY * tmp = nullptr;
        CHECK(source.GetColorMap(TYPE_TISSUE_COLOR, &img_format, &tmp));
        if (img_format != IMAGE_FORMAT_R8G8B8A8) {
            std::wcerr << "ERROR: Unexpected color-map format.\n";
            std::exit(-1);
        }
        color_map.Attach(tmp);
        tmp = nullptr;
    }

    Cart3dGeom bbox = {};
    CHECK(source.GetBoundingBox(&bbox));

    unsigned int stream_count = 0;
    CHECK(source.GetStreamCount(&stream_count));
    if (stream_count == 0) {
        std::wcerr << "ERROR: No image streams found.\n";
        std::exit(-1);
    }

    unsigned short max_res[] = {64, 64, 64};
    CComPtr<IImage3dStream> stream;
    CHECK(source.GetStream(0, bbox, max_res, &stream));

    ImageType stream_type = IMAGE_TYPE_INVALID;
    CHECK(stream->GetType(&stream_type));

    unsigned int frame_count = 0;
    CHECK(stream->GetFrameCount(&frame_count));
    std::wcout << L"Frame count: " << frame_count << L"\n";

    for (unsigned int frame = 0; frame < frame_count; ++frame) {
        // retrieve frame data
        Image3d data;
        CHECK(stream->GetFrame(frame, &data));
    }
}


int wmain (int argc, wchar_t *argv[]) {
    if (argc < 3) {
        std::wcout << L"Usage:\n";
        std::wcout << L"RegFreeTest.exe <loader-progid> <filename>" << std::endl;
        return -1;
    }

    CComBSTR progid = argv[1];  // e.g. "DummyLoader.Image3dFileLoader"
    CComBSTR filename = argv[2];

    ComInitialize com(COINIT_MULTITHREADED);

    CLSID clsid = {};
    if (FAILED(CLSIDFromProgID(progid, &clsid))) {
        std::wcerr << L"ERRORR: Unknown progid " << progid.m_str << L"\n";
        return -1;
    }

    // verify that loader library is compatible
    //CHECK(CheckImage3dAPIVersion(clsid)); // disabled, since it's not compatible with reg-free COM

    // create loader without accessing the registry (possible since the DummyLoader manifest is linked in)
    std::wcout << L"Creating loader " << progid.m_str << L"...\n";
    CComPtr<IImage3dFileLoader> loader;
    CHECK(loader.CoCreateInstance(clsid));

    {
        // load file
        Image3dError err_type = {};
        CComBSTR err_msg;
        HRESULT hr = loader->LoadFile(filename, &err_type, &err_msg);
        if (FAILED(hr)) {
            std::wcerr << L"LoadFile failed: code=" << err_type << L", message=" << err_msg.m_str << std::endl;
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
