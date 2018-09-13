/* Dummy test loader for the "3D API".
Designed by Fredrik Orderud <fredrik.orderud@ge.com>.
Copyright (c) 2016, GE Healthcare, Ultrasound.      */
#pragma once

#include "../Image3dAPI/ComSupport.hpp"
#include "../Image3dAPI/IImage3d.h"
#include "Image3dSource.hpp"

#include "DummyLoader.h"
#include "Resource.h"

class ATL_NO_VTABLE Image3dFileLoader :
    public CComObjectRootEx<CComMultiThreadModel>,
    public CComCoClass<Image3dFileLoader, &__uuidof(Image3dFileLoader)>,
    public IImage3dFileLoader {
public:
    Image3dFileLoader();

    /*NOT virtual*/ ~Image3dFileLoader();

    HRESULT STDMETHODCALLTYPE GetSupportedManufacturerModels(SAFEARRAY **, SAFEARRAY **) override;

    HRESULT STDMETHODCALLTYPE LoadFile(BSTR file_name, /*out*/BSTR *error_message) override;

    HRESULT STDMETHODCALLTYPE GetImageSource(/*out*/IImage3dSource **img_src) override;

    DECLARE_REGISTRY_RESOURCEID(IDR_Image3dFileLoader)
    
    BEGIN_COM_MAP(Image3dFileLoader)
        COM_INTERFACE_ENTRY(IImage3dFileLoader)
    END_COM_MAP()
};

OBJECT_ENTRY_AUTO(__uuidof(Image3dFileLoader), Image3dFileLoader)
