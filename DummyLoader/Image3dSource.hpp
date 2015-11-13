#pragma once
#include "../Image3dAPI/ComSupport.hpp"
#include "../Image3dAPI/IImage3d.h"



[coclass,
default(IImage3dSource),                ///< default interface
threading(both),                        ///< "both" enables direct thread access without marshaling
vi_progid("DummyLoader.Image3dSource"), ///< version-independent name
progid("DummyLoader.Image3dSource.1"),  ///< class name
version(1.0),
uuid(6FA82ED5-6332-4344-8417-DEA55E72098C), ///< class ID (must be unique)
helpstring("3D image source")]
class Image3dSource : public IImage3dSource {
public:
    Image3dSource() {
    }

    ~Image3dSource() {
    }

    HRESULT STDMETHODCALLTYPE GetFrameCount(/*[out]*/ unsigned int *size) {
        return E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE GetFrameTimes(/*[out]*/ SAFEARRAY * *frame_times) {
        return E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE GetFrame(unsigned int index, Cart3dGeom geom, /*[out]*/ Image3d *data) {
        return E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE GetBoundingBox(/*[out]*/ Cart3dGeom *geom) {
        return E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE GetColorMap(/*[out]*/ unsigned int __MIDL__IImage3dSource0000[256]) {
        return E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE GetECG(/*[out]*/ EcgSeries *ecg) {
        return E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE GetProbeInfo(/*[out]*/ ProbeInfo *probe) {
        return E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE GetSopInstanceUID(/*[out] */ BSTR *uid_str) {
        return E_NOTIMPL;
    }
};
