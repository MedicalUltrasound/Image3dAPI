#include "../Image3dAPI/ComSupport.hpp"
#include "../Image3dAPI/IImage3d.h"
#include "../Image3dAPI/VersionCheck.hpp"


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
        unsigned short max_res[] = {64, 64, 64};

        // retrieve frame data
        Image3d data;
        CHECK(source.GetFrame(frame, geom, max_res, &data));
    }
}


int main () {
    ComInitialize com(COINIT_MULTITHREADED);

    CComBSTR progid(L"DummyLoader.Image3dFileLoader");
    CLSID clsid = {};
    CHECK(CLSIDFromProgID(progid, &clsid));

    // verify that loader library is compatible
    //CHECK(CheckImage3dAPIVersion(clsid)); // disabled, since it's not compatible with reg-free COM

    CComPtr<IImage3dFileLoader> loader;
    CHECK(loader.CoCreateInstance(clsid));

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
