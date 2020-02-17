/* Dummy test loader for the "3D API".
Designed by Fredrik Orderud <fredrik.orderud@ge.com>.
Copyright (c) 2016, GE Healthcare, Ultrasound.      */
#pragma once

#define _USE_MATH_DEFINES // for M_PI
#include <cmath>
#include <memory>
#include <array>
#include <vector>
#include "../Image3dAPI/ComSupport.hpp"
#include "../Image3dAPI/IImage3d.h"

#include "DummyLoader.h"
#include "Resource.h"


/** RGBA color struct that matches DXGI_FORMAT_R8G8B8A8_UNORM.
Created due to the lack of such a class/struct in the Windows or Direct3D SDKs.
Please remove this class if a more standardized alternative is available. */
struct R8G8B8A8 {
    R8G8B8A8 () : r(0), g(0), b(0), a(0) {
    }

    R8G8B8A8 (unsigned char _r, unsigned char _g, unsigned char _b, unsigned char _a) : r(_r), g(_g), b(_b), a(_a) {
    }

    operator unsigned int () const {
        return *reinterpret_cast<const unsigned int*>(this);
    }

    unsigned char r, g, b, a;  ///< color channels
};


class ATL_NO_VTABLE Image3dSource :
    public CComObjectRootEx<CComMultiThreadModel>,
    public CComCoClass<Image3dSource, &__uuidof(Image3dSource)>,
    public IImage3dSource {
public:
    Image3dSource();

    /*NOT virtual*/ ~Image3dSource();

    HRESULT STDMETHODCALLTYPE GetFrameCount(/*out*/unsigned int *size) override;

    HRESULT STDMETHODCALLTYPE GetFrameTimes(/*out*/SAFEARRAY * *frame_times) override;

    HRESULT STDMETHODCALLTYPE GetFrame(unsigned int index, Cart3dGeom out_geom, unsigned short max_res[3], /*out*/Image3d *data) override;

    HRESULT STDMETHODCALLTYPE GetBoundingBox(/*out*/Cart3dGeom *geom) override;

    HRESULT STDMETHODCALLTYPE GetColorMap(/*out*/SAFEARRAY ** map) override;

    HRESULT STDMETHODCALLTYPE GetECG(/*out*/EcgSeries *ecg) override;

    HRESULT STDMETHODCALLTYPE GetProbeInfo(/*out*/ProbeInfo *probe) override;

    HRESULT STDMETHODCALLTYPE GetSopInstanceUID(/*out*/BSTR *uid_str) override;

    Image3d SampleFrame (const Image3d & frame, Cart3dGeom out_geom, unsigned short max_res[3]);

    DECLARE_REGISTRY_RESOURCEID(IDR_Image3dSource)

    BEGIN_COM_MAP(Image3dSource)
        COM_INTERFACE_ENTRY(IImage3dSource)
    END_COM_MAP()

private:
    ProbeInfo                m_probe;
    EcgSeries                m_ecg;
    std::array<R8G8B8A8,256> m_color_map;
    Cart3dGeom               m_img_geom = {};
    std::vector<Image3d>     m_frames;
};

OBJECT_ENTRY_AUTO(__uuidof(Image3dSource), Image3dSource)
