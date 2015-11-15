#pragma once
#include <memory>
#include <vector>
#include "../Image3dAPI/ComSupport.hpp"
#include "../Image3dAPI/IImage3d.hpp"



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
    Image3dSource () : m_probe(PROBE_THORAX, L"4V") {
        {
            std::vector<float> samples;
            std::vector<double> trig_times;
            EcgSeriesObj ecg(0.0, 1e-3, samples, trig_times);
            m_ecg = ecg;
        }
        {
            unsigned short dims[] = { 12, 14, 16 };
            std::vector<byte> img_buf(dims[0]*dims[1]*dims[2], 0);
            Image3dObj tmp(3.14, FORMAT_U8, dims, img_buf);
            m_frames.push_back(std::move(tmp));
        }
    }

    /*NOT virtual*/ ~Image3dSource () {
    }

    HRESULT STDMETHODCALLTYPE GetFrameCount (/*[out]*/ unsigned int *size) {
        if (!size)
            return E_INVALIDARG;

        *size = static_cast<unsigned int>(m_frames.size());
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE GetFrameTimes (/*[out]*/ SAFEARRAY * *frame_times) {
        if (!frame_times)
            return E_INVALIDARG;

        const unsigned int N = static_cast<unsigned int>(m_frames.size());
        CComSafeArray<double> result(N);
        double * time_arr = &result.GetAt(0);
        for (unsigned int i = 0; i < N; ++i)
            time_arr[i] = m_frames[i].time;

        *frame_times = result.Detach();
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE GetFrame (unsigned int index, Cart3dGeom geom, /*[out]*/ Image3d *data) {
        return E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE GetBoundingBox (/*[out]*/ Cart3dGeom *geom) {
        return E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE GetColorMap (/*[out]*/ unsigned int __MIDL__IImage3dSource0000[256]) {
        return E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE GetECG (/*[out]*/ EcgSeries *ecg) {
        if (!ecg)
            return E_INVALIDARG;

        // return a copy
        *ecg = EcgSeriesObj(m_ecg).Detach();
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE GetProbeInfo (/*[out]*/ ProbeInfo *probe) {
        if (!probe)
            return E_INVALIDARG;

        // return a copy
        *probe = ProbeInfoObj(m_probe).Detatch();
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE GetSopInstanceUID (/*[out] */ BSTR *uid_str) {
        return E_NOTIMPL;
    }

private:
    ProbeInfoObj            m_probe;
    EcgSeriesObj            m_ecg;
    std::vector<Image3dObj> m_frames;
};
