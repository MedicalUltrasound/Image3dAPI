#include "Image3dSource.hpp"
#include "LinAlg.hpp"




Image3dSource::Image3dSource() {
    m_probe.type = PROBE_External;
    m_probe.name = L"4V";

    // One second loop starting at t = 10
    const size_t numFrames = 25;
    const double duration = 1.0; // Seconds
    const double startTime = 10.0;

    {
        // simulate sine-wave ECG
        const int N = 128;
        CComSafeArray<float> samples(N);
        for (int i = 0; i < N; ++i)
            samples[i] = static_cast<float>(sin(4 * i*M_PI / N));

        CComSafeArray<double> trig_times;
        trig_times.Add(startTime); // trig every 1/2 sec
        trig_times.Add(startTime + duration / 2);
        trig_times.Add(startTime + duration);

        EcgSeries ecg;
        ecg.start_time = startTime;
        ecg.delta_time = duration / N;
        ecg.samples = samples.Detach();
        ecg.trig_times = trig_times.Detach();
        m_ecg = EcgSeries(ecg);
    }
    {
        // flat gray tissue scale
        for (size_t i = 0; i < m_color_map_tissue.size(); ++i)
            m_color_map_tissue[i] = R8G8B8A8(static_cast<unsigned char>(i), static_cast<unsigned char>(i), static_cast<unsigned char>(i), 0xFF);
    }
    {
        // image geometry    X     Y       Z
        Cart3dGeom geom = { -0.1f, 0,     -0.075f,// origin
                             0.20f,0,      0,     // dir1 (width)
                             0,    0.10f,  0,     // dir2 (depth)
                             0,    0,      0.15f};// dir3 (elevation)
        m_bbox = geom;
    }

    // a single tissue stream
    m_stream_types.push_back(IMAGE_TYPE_TISSUE);
}

Image3dSource::~Image3dSource() {
}


HRESULT Image3dSource::GetStreamCount(/*out*/unsigned int *size) {
    if (!size)
        return E_INVALIDARG;

    *size = static_cast<unsigned int>(m_stream_types.size());
    return S_OK;
}



HRESULT Image3dSource::GetStream(unsigned int index, Cart3dGeom out_geom, unsigned short max_resolution[3], /*out*/IImage3dStream ** stream) {
    if (!stream)
        return E_INVALIDARG;
    if (index >= m_stream_types.size())
        return E_BOUNDS;

    CComPtr<IImage3dStream> stream_obj;
    {
        // on-demand stream creation
        auto stream_cls = CreateLocalInstance<Image3dStream>();
        stream_cls->Initialize(m_stream_types[index], m_bbox, out_geom, max_resolution);
        stream_obj = stream_cls; // cast class pointer to interface
    }

    *stream = stream_obj.Detach();
    return S_OK;
}

HRESULT Image3dSource::GetBoundingBox(/*out*/Cart3dGeom *geom) {
    if (!geom)
        return E_INVALIDARG;

    *geom = m_bbox;
    return S_OK;
}

HRESULT Image3dSource::GetColorMap(ColorMapType type, /*out*/ImageFormat * format, /*out*/SAFEARRAY ** map) {
    if (!map)
        return E_INVALIDARG;
    if (*map)
        return E_INVALIDARG;

    if (type == TYPE_TISSUE_COLOR) {
        *format = IMAGE_FORMAT_R8G8B8A8;
        // copy to new buffer
        CComSafeArray<uint8_t> color_map(4*static_cast<unsigned int>(m_color_map_tissue.size()));
        memcpy(&color_map.GetAt(0), m_color_map_tissue.data(), sizeof(m_color_map_tissue));
        *map = color_map.Detach(); // transfer ownership
        return S_OK;
    }

    return E_NOTIMPL;
}

HRESULT Image3dSource::GetECG(/*out*/EcgSeries *ecg) {
    if (!ecg)
        return E_INVALIDARG;

    // return a copy
    *ecg = EcgSeries(m_ecg);
    return S_OK;
}

HRESULT Image3dSource::GetProbeInfo(/*out*/ProbeInfo *probe) {
    if (!probe)
        return E_INVALIDARG;

    // return a copy
    *probe = m_probe;
    return S_OK;
}

HRESULT Image3dSource::GetSopInstanceUID(/*out*/BSTR *uid_str) {
    if (!uid_str)
        return E_INVALIDARG;
    if (*uid_str)
        return E_INVALIDARG; // input must be pointer to nullptr

    *uid_str = CComBSTR("DUMMY_UID").Detach();
    return S_OK;
}
