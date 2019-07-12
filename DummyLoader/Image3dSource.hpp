/* Dummy test loader for the "3D API".
Designed by Fredrik Orderud <fredrik.orderud@ge.com>.
Copyright (c) 2016, GE Healthcare, Ultrasound.      */
#pragma once

#include "Image3dStream.hpp"



class ATL_NO_VTABLE Image3dSource :
    public CComObjectRootEx<CComMultiThreadModel>,
    public CComCoClass<Image3dSource, &__uuidof(Image3dSource)>,
    public IImage3dSource {
public:
    Image3dSource();

    /*NOT virtual*/ ~Image3dSource();


    HRESULT STDMETHODCALLTYPE GetStreamCount (/*out*/unsigned int * size) override;

    HRESULT STDMETHODCALLTYPE GetStream (unsigned int index, Cart3dGeom out_geom, unsigned short max_resolution[3], /*out*/IImage3dStream ** stream) override;

    HRESULT STDMETHODCALLTYPE GetBoundingBox(/*out*/Cart3dGeom *geom) override;

    HRESULT STDMETHODCALLTYPE GetColorMap(ColorMapType type, /*out*/ImageFormat * format, /*out*/SAFEARRAY ** map) override;

    HRESULT STDMETHODCALLTYPE GetECG(/*out*/EcgSeries *ecg) override;

    HRESULT STDMETHODCALLTYPE GetProbeInfo(/*out*/ProbeInfo *probe) override;

    HRESULT STDMETHODCALLTYPE GetSopInstanceUID(/*out*/BSTR *uid_str) override;

    DECLARE_REGISTRY_RESOURCEID(IDR_Image3dSource)

    BEGIN_COM_MAP(Image3dSource)
        COM_INTERFACE_ENTRY(IImage3dSource)
    END_COM_MAP()

private:
    ProbeInfo                m_probe;
    EcgSeries                m_ecg;
    std::array<R8G8B8A8,256> m_color_map_tissue;

    Cart3dGeom               m_bbox = {};
    std::vector<ImageType>   m_stream_types;
};

OBJECT_ENTRY_AUTO(__uuidof(Image3dSource), Image3dSource)
