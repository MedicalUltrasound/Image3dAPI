#include "Image3dFileLoader.hpp"


Image3dFileLoader::Image3dFileLoader() {
}

Image3dFileLoader::~Image3dFileLoader() {
}

HRESULT Image3dFileLoader::GetSupportedManufacturerModels(SAFEARRAY ** _manufacturer, SAFEARRAY ** _model) {
    if (!_manufacturer || !_model)
        return E_INVALIDARG;

    CComSafeArray<BSTR> manufacturer((ULONG)0), model((ULONG)0);
    // 1st pair
    manufacturer.Add(L"Dummy medical systems");
    model.Add(L"Super scanner *"); // matches "Super scanner 1", "Super scanner 2" etc.
    
    // 2nd pair
    manufacturer.Add(L"Dummy healthcare");
    model.Add(L"Some scanner 1");

    // 3rd pair
    manufacturer.Add(L"Dummy healthcare");
    model.Add(L"Some scanner 2");

    *_manufacturer = manufacturer.Detach();
    *_model = model.Detach();
    return S_OK;
}


HRESULT Image3dFileLoader::LoadFile(BSTR file_name, /*out*/BSTR *error_message) {
    return S_OK; // no operation
}

HRESULT Image3dFileLoader::GetImageSource(/*out*/IImage3dSource **img_src) {
    if (!img_src)
        return E_INVALIDARG;

    CComPtr<Image3dSource> obj = CreateLocalInstance<Image3dSource>();
    *img_src = obj.Detach();
    return S_OK;
}
