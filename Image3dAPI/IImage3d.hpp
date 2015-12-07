/* C++ RAII wrappers of IIMage3d structs. */
#pragma once
#include <vector>
#include "ComSupport.hpp"
#include "IImage3d.h"


/** Returns the element size [bytes] of a sample, given a specific format. */
static unsigned short ImageFormatSize (ImageFormat format) {
    if (format == FORMAT_U8)
        return 1;
    else
        throw std::logic_error("ImageFormatSize: Unknown type.");
}


struct ProbeInfoObj : public ProbeInfo {
    ProbeInfoObj () {
        type = PROBE_UNKNOWN;
        name = nullptr;
    }

    ProbeInfoObj (ProbeType _type, const std::wstring & _name) {
        type = _type;
        name = CComBSTR(static_cast<int>(_name.length()), _name.c_str()).Detach();
    }

    ProbeInfoObj (const ProbeInfoObj & other) {
        type = other.type;
        name = CComBSTR(other.name).Detach();
    }

    ~ProbeInfoObj () {
        if (name) {
            CComBSTR tmp;
            tmp.Attach(name);
            name = nullptr;
        }
    }

    ProbeInfo Detatch () {
        ProbeInfo info = {type, name};
        type = PROBE_UNKNOWN;
        name = nullptr;
        return info;
    }

private:
    ProbeInfoObj & operator = (const ProbeInfoObj &); ///< non-assignable
};


struct Image3dObj : public Image3d {
    Image3dObj () {
        time    = 0;
        format  = FORMAT_INVALID;
        dims[0] = dims[1] = dims[2] = 0;
        stride0 = 0;
        stride1 = 0;
        data    = nullptr;
    }

    Image3dObj (double _time, ImageFormat _format, unsigned short _dims[3], const std::vector<byte> & packed_input) {
        time    = _time;
        format  = _format;
        dims[0] = _dims[0];
        dims[1] = _dims[1];
        dims[2] = _dims[2];
        stride0 = dims[0]*ImageFormatSize(_format);
        stride1 = stride0*dims[1];

        CComSafeArray<byte> tmp = ConvertToSafeArray(packed_input);
        data = tmp.Detach();
    }

    Image3dObj (const Image3dObj & other, bool deep_copy = true) {
        time    = other.time;
        format  = other.format;
        dims[0] = other.dims[0];
        dims[1] = other.dims[1];
        dims[2] = other.dims[2];
        stride0 = other.stride0;
        stride1 = other.stride1;

        if (deep_copy) {
            // do a full copy of the image data
            data = CComSafeArray<byte>(other.data).Detach();
        } else {
            // make shallow copy that accesses the same data
            // this means that object lifetime is controlled by the parent "other" object
            CHECK(SafeArrayAllocDescriptorEx(VT_UI1, 1, &data));
            data = other.data;
            // prevent data from being deleted
            data->fFeatures |= (FADF_AUTO | FADF_FIXEDSIZE);
        }
    }

    ~Image3dObj () {
        if (data) {
            CComSafeArray<byte> tmp;
            CHECK(tmp.Attach(data));
            data = nullptr;
        }
    }

    Image3d Detach () {
        Image3d img = {time, format, dims[0], dims[1], dims[2], stride0, stride1, data};
        data = nullptr;
        return img;
    }

private:
    Image3dObj & operator = (const Image3dObj &); ///< non-assignable
};


struct EcgSeriesObj : public EcgSeries {
    EcgSeriesObj () {
        start_time = 0;
        delta_time = 0;
        samples    = nullptr;
        trig_times = nullptr;
    }

    EcgSeriesObj (const EcgSeriesObj & other) {
        samples    = nullptr;
        trig_times = nullptr;
        operator = (other);
    }

    EcgSeriesObj (double start, double delta, const std::vector<float> & _samples, const std::vector<double> & _trigs) {
        start_time = start;
        delta_time = delta;
        
        {
            CComSafeArray<float> tmp = ConvertToSafeArray(_samples);
            samples = tmp.Detach();
        }
        {
            CComSafeArray<double> tmp = ConvertToSafeArray(_trigs);
            trig_times = tmp.Detach();
        }
    }

    ~EcgSeriesObj () {
        if (samples) {
            CComSafeArray<float> tmp;
            CHECK(tmp.Attach(samples));
            samples = nullptr;
        }
        if (trig_times) {
            CComSafeArray<double> tmp;
            CHECK(tmp.Attach(trig_times));
            trig_times = nullptr;
        }
    }

    EcgSeriesObj & operator = (const EcgSeriesObj & other) {
        EcgSeriesObj::~EcgSeriesObj();

        start_time = other.start_time;
        delta_time = other.delta_time;
        samples    = CComSafeArray<float>(other.samples).Detach();
        trig_times = CComSafeArray<double>(other.trig_times).Detach();

        return *this;
    }

    EcgSeries Detach() {
        EcgSeries ecg = {start_time, delta_time, samples, trig_times};
        samples    = nullptr;
        trig_times = nullptr;
        return ecg;
    }
};
