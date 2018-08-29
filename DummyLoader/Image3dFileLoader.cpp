#include "Image3dFileLoader.hpp"


Image3dFileLoader::Image3dFileLoader() {
}

Image3dFileLoader::~Image3dFileLoader() {
}


HRESULT Image3dFileLoader::LoadFile(BSTR file_name, /*out*/BSTR *error_message) {
    return S_OK; // no operation
}

HRESULT Image3dFileLoader::IsHighPerfMode(BOOL * high_perf) {
    if (!high_perf)
        return E_INVALIDARG;

    *high_perf = true; // always CPU-based implementation
    return S_OK;
}

HRESULT Image3dFileLoader::GetImageSource(/*out*/IImage3dSource **img_src) {
    if (!img_src)
        return E_INVALIDARG;

    CComPtr<Image3dSource> obj = CreateLocalInstance<Image3dSource>();
    *img_src = obj.Detach();
    return S_OK;
}
