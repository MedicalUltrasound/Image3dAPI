#include "Image3dSource.hpp"
#include "LinAlg.hpp"


static const uint8_t OUTSIDE_VAL = 0;   // black outside image volume
static const uint8_t PROBE_PLANE = 127; // gray value for plane closest to probe


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
        // flat gray scale
        for (size_t i = 0; i < m_color_map.size(); ++i)
            m_color_map[i] = R8G8B8A8(static_cast<unsigned char>(i), static_cast<unsigned char>(i), static_cast<unsigned char>(i), 0xFF);
    }
    {
        // image geometry    X     Y       Z
        Cart3dGeom geom = { -0.1f, 0,     -0.075f,// origin
                             0.20f,0,      0,     // dir1 (width)
                             0,    0.10f,  0,     // dir2 (depth)
                             0,    0,      0.15f};// dir3 (elevation)
        m_img_geom = geom;
    }
    {
        // checker board image data
        unsigned short dims[] = { 20, 15, 10 }; // matches length of dir1, dir2 & dir3, so that the image squares become quadratic
        std::vector<byte> img_buf(dims[0] * dims[1] * dims[2]);
        for (size_t frameNumber = 0; frameNumber < numFrames; ++frameNumber) {
            for (unsigned int z = 0; z < dims[2]; ++z) {
                for (unsigned int y = 0; y < dims[1]; ++y) {
                    for (unsigned int x = 0; x < dims[0]; ++x) {
                        bool even_f = (frameNumber / 2 % 2) == 0;
                        bool even_x = (x / 2 % 2) == 0;
                        bool even_y = (y / 2 % 2) == 0;
                        bool even_z = (z / 2 % 2) == 0;
                        byte & out_sample = img_buf[x + y*dims[0] + z*dims[0] * dims[1]];
                        if (even_f ^ even_x ^ even_y ^ even_z)
                            out_sample = 255;
                        else
                            out_sample = 0;
                    }
                }
            }

            // special grayscale value for plane closest to probe
            for (unsigned int z = 0; z < dims[2]; ++z) {
                for (unsigned int x = 0; x < dims[0]; ++x) {
                    unsigned int y = 0;
                    byte & out_sample = img_buf[x + y*dims[0] + z*dims[0] * dims[1]];
                    out_sample = PROBE_PLANE;
                }
            }

            m_frames.push_back(CreateImage3d(frameNumber*(duration/numFrames) + startTime, FORMAT_U8, dims, img_buf));
        }
    }
}

Image3dSource::~Image3dSource() {
}


HRESULT Image3dSource::GetFrameCount(/*out*/unsigned int *size) {
    if (!size)
        return E_INVALIDARG;

    *size = static_cast<unsigned int>(m_frames.size());
    return S_OK;
}

HRESULT Image3dSource::GetFrameTimes(/*out*/SAFEARRAY * *frame_times) {
    if (!frame_times)
        return E_INVALIDARG;

    const unsigned int N = static_cast<unsigned int>(m_frames.size());
    CComSafeArray<double> result(N);
    if (N > 0) {
        double * time_arr = &result.GetAt(0);
        for (unsigned int i = 0; i < N; ++i)
            time_arr[i] = m_frames[i].time;
    }

    *frame_times = result.Detach();
    return S_OK;
}


/** Convert from normalized voxel pos in [0,1) to (x,y,z) coordinate. */
static vec3f PosToCoord (vec3f origin, vec3f dir1, vec3f dir2, vec3f dir3, const vec3f pos) {
    mat33f M;
    col_assign(M, 0, dir1);
    col_assign(M, 1, dir2);
    col_assign(M, 2, dir3);

    return prod(M, pos) + origin;
}


/** Convert from (x,y,z) coordinate to normalized voxel pos in [0,1). */
static vec3f CoordToPos (Cart3dGeom geom, const vec3f xyz) {
    const vec3f origin(geom.origin_x, geom.origin_y, geom.origin_z);
    const vec3f dir1(geom.dir1_x, geom.dir1_y, geom.dir1_z);
    const vec3f dir2(geom.dir2_x, geom.dir2_y, geom.dir2_z);
    const vec3f dir3(geom.dir3_x, geom.dir3_y, geom.dir3_z);

    mat33f M;
    col_assign(M, 0, dir1);
    col_assign(M, 1, dir2);
    col_assign(M, 2, dir3);

    return prod(inv(M), xyz - origin);
}


static unsigned char SampleVoxel (const Image3d & frame, const vec3f pos) {
    // out-of-bounds checking
    if ((pos.x < 0) || (pos.y < 0) || (pos.z < 0))
        return OUTSIDE_VAL;

    auto x = static_cast<unsigned int>(frame.dims[0] * pos.x);
    auto y = static_cast<unsigned int>(frame.dims[1] * pos.y);
    auto z = static_cast<unsigned int>(frame.dims[2] * pos.z);

    // out-of-bounds checking
    if ((x >= frame.dims[0]) || (y >= frame.dims[1]) || (z >= frame.dims[2]))
        return OUTSIDE_VAL;

    return static_cast<unsigned char*>(frame.data->pvData)[x + y*frame.stride0 + z*frame.stride1];
}


Image3d Image3dSource::SampleFrame (const Image3d & frame, Cart3dGeom out_geom, unsigned short max_res[3]) {
    if (max_res[2] == 0)
        max_res[2] = 1; // require at least one plane to to retrieved

    vec3f out_origin(out_geom.origin_x, out_geom.origin_y, out_geom.origin_z);
    vec3f out_dir1(out_geom.dir1_x, out_geom.dir1_y, out_geom.dir1_z);
    vec3f out_dir2(out_geom.dir2_x, out_geom.dir2_y, out_geom.dir2_z);
    vec3f out_dir3(out_geom.dir3_x, out_geom.dir3_y, out_geom.dir3_z);

    // allow 3rd axis to be empty if only retrieving a single slice
    if ((out_dir3 == vec3f(0, 0, 0)) && (max_res[2] < 2))
        out_dir3 = cross_prod(out_dir1, out_dir2);

    // sample image buffer
    std::vector<unsigned char> img_buf(max_res[0] * max_res[1] * max_res[2], 127);
    for (unsigned short z = 0; z < max_res[2]; ++z) {
        for (unsigned short y = 0; y < max_res[1]; ++y) {
            for (unsigned short x = 0; x < max_res[0]; ++x) {
                // convert from input texture coordinate to output texture coordinate
                vec3f pos_in(x*1.0f/max_res[0], y*1.0f/max_res[1], z*1.0f/max_res[2]);
                vec3f xyz = PosToCoord(out_origin, out_dir1, out_dir2, out_dir3, pos_in);
                vec3f pos_out = CoordToPos(m_img_geom, xyz);

                unsigned char val = SampleVoxel(frame, pos_out);
                img_buf[x + y*max_res[0] + z*max_res[0] * max_res[1]] = val;
            }
        }
    }

    return CreateImage3d(frame.time, FORMAT_U8, max_res, img_buf);
}

HRESULT Image3dSource::GetFrame(unsigned int index, Cart3dGeom out_geom, unsigned short max_res[3], /*out*/Image3d *data) {
    if (!data)
        return E_INVALIDARG;
    if (index >= m_frames.size())
        return E_BOUNDS;

    Image3d result = SampleFrame(m_frames[index], out_geom, max_res);
    *data = std::move(result);
    return S_OK;
}

HRESULT Image3dSource::GetBoundingBox(/*out*/Cart3dGeom *geom) {
    if (!geom)
        return E_INVALIDARG;

    *geom = m_img_geom;
    return S_OK;
}

HRESULT Image3dSource::GetColorMap(/*out*/SAFEARRAY ** map) {
    if (!map)
        return E_INVALIDARG;
    if (*map)
        return E_INVALIDARG;

    // copy to new buffer
    CComSafeArray<uint32_t> color_map(static_cast<unsigned int>(m_color_map.size()));
    memcpy(&color_map.GetAt(0), m_color_map.data(), sizeof(m_color_map));
    *map = color_map.Detach(); // transfer ownership
    return S_OK;
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
