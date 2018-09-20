#include "Image3dFileLoader.hpp"


Image3dFileLoader::Image3dFileLoader() {
}

Image3dFileLoader::~Image3dFileLoader() {
}


HRESULT Image3dFileLoader::LoadFile(BSTR file_name, /*out*/Image3dError *err_type, /*out*/BSTR *err_msg) {
    if (!err_type || !err_msg)
        return E_INVALIDARG;

    *err_type = Image3d_SUCCESS;
    *err_msg  = CComBSTR().Detach();
    return S_OK; // no operation
}

HRESULT Image3dFileLoader::GetImageSource(/*out*/IImage3dSource **img_src) {
    if (!img_src)
        return E_INVALIDARG;

    CComPtr<Image3dSource> obj = CreateLocalInstance<Image3dSource>();
    *img_src = obj.Detach();
    return S_OK;
}
