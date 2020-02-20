#include "Image3dStream.hpp"

#include "LinAlg.hpp"


static const uint8_t OUTSIDE_VAL = 0;   // black outside image volume
static const uint8_t PROBE_PLANE = 127; // gray value for plane closest to probe


Image3dStream::Image3dStream() {
}

void Image3dStream::Initialize (ImageType type, Cart3dGeom img_geom, Cart3dGeom out_geom, unsigned short max_resolution[3]) {
    m_type = type;
    m_img_geom = img_geom;
    m_out_geom = out_geom;

    for (size_t i = 0; i < 3; ++i)
        m_max_res[i] = max_resolution[i];

    // One second loop starting at t = 10
    const size_t numFrames = 25;
    const double duration = 1.0; // Seconds
    const double startTime = 10.0;

    if (type == IMAGE_TYPE_TISSUE) {
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

            m_frames.push_back(CreateImage3d(frameNumber*(duration/numFrames) + startTime, IMAGE_FORMAT_U8, dims, img_buf));
        }
    } else if (type == IMAGE_TYPE_BLOOD_VEL) {
        // velocity & bandwidth scale color-flow data
        unsigned short dims[] = { 20, 15, 10 }; // matches length of dir1, dir2 & dir3, so that the image squares become quadratic
        std::vector<byte> img_buf(2 * dims[0] * dims[1] * dims[2]);
        for (size_t frameNumber = 0; frameNumber < numFrames; ++frameNumber) {
            for (unsigned int z = 0; z < dims[2]; ++z) {
                for (unsigned int y = 0; y < dims[1]; ++y) {
                    for (unsigned int x = 0; x < dims[0]; ++x) {
                        int8_t & freq = reinterpret_cast<int8_t&>(img_buf[0 + 2*(x + y*dims[0] + z*dims[0] * dims[1])]);
                        byte & bw   = img_buf[1 + 2*(x + y*dims[0] + z*dims[0] * dims[1])];

                        freq = static_cast<int8_t>(255*(0.5f - y*1.0f/dims[1])); // [+127, -128] along Y axis
                        bw   = static_cast<uint8_t>(256*(x*1.0f/dims[0]));       // [0,255] along X axis
                    }
                }
            }
            m_frames.push_back(CreateImage3d(frameNumber*(duration/numFrames) + startTime, IMAGE_FORMAT_FREQ8POW8, dims, img_buf));
        }
    } else {
        abort(); // should never be reached
    }
}

Image3dStream::~Image3dStream() {
}


HRESULT Image3dStream::GetFrameCount(/*out*/unsigned int *size) {
    if (!size)
        return E_INVALIDARG;

    *size = static_cast<unsigned int>(m_frames.size());
    return S_OK;
}

HRESULT Image3dStream::GetFrameTimes(/*out*/SAFEARRAY * *frame_times) {
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


HRESULT Image3dStream::GetFrame(unsigned int index, /*out*/Image3d *data) {
    if (!data)
        return E_INVALIDARG;
    if (index >= m_frames.size())
        return E_BOUNDS;

    ImageFormat format = m_frames[index].format;
    if (format == IMAGE_FORMAT_U8) {
        Image3d result = SampleFrame<uint8_t>(m_frames[index], m_img_geom, m_out_geom, m_max_res);
        *data = std::move(result);
        return S_OK;
    } else if (format == IMAGE_FORMAT_FREQ8POW8) {
        Image3d result = SampleFrame<uint16_t>(m_frames[index], m_img_geom, m_out_geom, m_max_res);
        *data = std::move(result);
        return S_OK;
    }

    return E_NOTIMPL;
}
