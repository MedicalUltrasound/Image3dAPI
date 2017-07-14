#include "../Image3dAPI/ComSupport.hpp"
#include "../Image3dAPI/IImage3d.h"


void ParseSource (IImage3dSource & source) {
    Cart3dGeom geom = {};
    CHECK(source.GetBoundingBox(&geom));

    unsigned int frame_count = 0;
    CHECK(source.GetFrameCount(&frame_count));

    CComSafeArray<unsigned int> color_map;
    {
        SAFEARRAY * tmp = nullptr;
        CHECK(source.GetColorMap(&tmp));
        color_map.Attach(tmp);
        tmp = nullptr;
    }

    for (unsigned int frame = 0; frame < frame_count; ++frame) {
        unsigned short max_res[] = {128, 128, 128};

        // retrieve frame data
        Image3d data;
        CHECK(source.GetFrame(frame, geom, max_res, &data));
    }
}


int main () {
    ComInitialize com(COINIT_MULTITHREADED);

    GUID dummy_loader_guid = { 0x8E754A72,0x0067,0x462B, {0x92, 0x67, 0xE8, 0x4A, 0xF8, 0x48, 0x28, 0xF1} }; // DummyLoader.Image3dFileLoader
    CComPtr<IImage3dFileLoader> loader;
    CHECK(loader.CoCreateInstance(dummy_loader_guid));

    {
        // load file
        CComBSTR filename = L"dummy.dcm";
        CComBSTR error;
        CHECK(loader->LoadFile(filename, &error));
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
