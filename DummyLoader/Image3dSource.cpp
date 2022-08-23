#include "Image3dSource.hpp"
#include "LinAlg.hpp"




Image3dSource::Image3dSource() {
    m_probe.type = PROBE_External;
    m_probe.name = L"4V";

    // One second loop starting at t = 10
    const size_t numFrames = 25;
    const double duration = 1.0; // ECG duration in seconds (the sum of the duration of individual ECG samples)
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
        // red to blue flow scale with green at high BW
        assert(m_color_map_flow.size() == 256*256);
        for (size_t bw = 0; bw < 256; ++bw) {
            // increasing green for high bandwidth
            uint8_t green = (bw >= 192) ? static_cast<unsigned char>(bw) : 0;

            for (size_t freq = 0; freq < 256; ++freq) { // unsigned counter, so freq>127 corresponds to negative velocities
                // increasing red for positive velocities
                uint8_t red  = (freq < 128) ? 128+static_cast<unsigned char>(freq) : 0;
                if (green)
                    red = 0;

                // increasing blue for negative velocities
                uint8_t blue = (freq >= 128) ? 128+static_cast<unsigned char>(255-freq) : 0;
                if (green)
                    blue = 0;

                m_color_map_flow[freq + bw*256] = R8G8B8A8(red, green, blue, 0xFF);
            }
        }
    }
    {
        // flow arbitration scale
        assert(m_flow_arb.size() == 256*256);
        for (size_t bw = 0; bw < 256; ++bw) {
            for (size_t freq = 0; freq < 256; ++freq) { // unsigned counter, so freq>127 corresponds to negative velocities
                // show flow when |freq| >= 16
                bool show_flow = std::abs(static_cast<int>(freq)) >= 16;
                m_flow_arb[freq + bw*256] = show_flow ? 0xFF : 0x00;
            }
        }
    }
    {
        // image geometry    X     Y       Z
        Cart3dGeom geom = { -0.1f, 0,     -0.075f,// origin
                             0.20f,0,      0,     // dir1 (width)
                             0,    0.10f,  0,     // dir2 (depth)
                             0,    0,      0.15f};// dir3 (elevation)
        m_bbox = geom;
    }

    // simulate tissue + color-flow data
    m_stream_types.push_back(IMAGE_TYPE_TISSUE);
    m_stream_types.push_back(IMAGE_TYPE_BLOOD_VEL);
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
        Cart3dGeom bbox = m_bbox;
        if (m_stream_types[index] == IMAGE_TYPE_BLOOD_VEL) {
            // shrink color-flow sector to make it more realistic
            vec3f origin, dir1, dir2, dir3;
            std::tie(origin,dir1,dir2,dir3) = FromCart3dGeom(bbox);

            float SCALE_FACTOR = 0.8f;
            origin += 0.5f*(1-SCALE_FACTOR)*(dir1 + dir2 + dir3);
            dir1 *= SCALE_FACTOR;
            dir2 *= SCALE_FACTOR;
            dir3 *= SCALE_FACTOR;

            bbox = ToCart3dGeom(origin, dir1, dir2, dir3);
        }

        auto stream_cls = CreateLocalInstance<Image3dStream>();
        stream_cls->Initialize(m_stream_types[index], bbox, out_geom, max_resolution);
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
    } else if (type == TYPE_FLOW_COLOR) {
        *format = IMAGE_FORMAT_R8G8B8A8;
        // copy to new buffer
        CComSafeArray<uint8_t> color_map(4*static_cast<unsigned int>(m_color_map_flow.size()));
        memcpy(&color_map.GetAt(0), m_color_map_flow.data(), sizeof(m_color_map_flow));
        *map = color_map.Detach(); // transfer ownership
        return S_OK;
    } else if (type = TYPE_FLOW_ARB) {
        *format = IMAGE_FORMAT_U8;
        // copy to new buffer
        CComSafeArray<uint8_t> color_map(static_cast<unsigned int>(m_flow_arb.size()));
        memcpy(&color_map.GetAt(0), m_flow_arb.data(), sizeof(m_flow_arb));
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
