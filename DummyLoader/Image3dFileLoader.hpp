/* Dummy test loader for the "3D API".
Designed by Fredrik Orderud <fredrik.orderud@ge.com>.
Copyright (c) 2016, GE Healthcare, Ultrasound.      */
#pragma once

#include "../Image3dAPI/ComSupport.hpp"
#include "../Image3dAPI/IImage3d.hpp"


[coclass,
default(IImage3dFileLoader),                ///< default interface
threading(both),                            ///< "both" enables direct thread access without marshaling
vi_progid("DummyLoader.Image3dFileLoader"), ///< version-independent name
progid("DummyLoader.Image3dFileLoader.1"),  ///< class name
version(1.0),
uuid(8E754A72-0067-462B-9267-E84AF84828F1), ///< class ID (must be unique)
helpstring("3D image file loader")]
class Image3dFileLoader : public IImage3dFileLoader {
public:
    Image3dFileLoader() {
    }

    /*NOT virtual*/ ~Image3dFileLoader() {
    }

    HRESULT STDMETHODCALLTYPE LoadFile(BSTR file_name, /*[out]*/ BSTR *error_message) {
        return S_OK; // no operation
    }

    HRESULT STDMETHODCALLTYPE GetImageSource(/*[out]*/ IImage3dSource **img_src) {
        if (!img_src)
            return E_INVALIDARG;
        
        CComPtr<Image3dSource> obj = CreateLocalInstance<Image3dSource>();
        *img_src = obj.Detach();
        return S_OK;
    }
};
