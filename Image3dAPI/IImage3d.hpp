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

    ~ProbeInfoObj () {
        if (name) {
            CComBSTR tmp;
            tmp.Attach(name);
            name = nullptr;
        }
    }

private:
    ProbeInfoObj (const ProbeInfoObj &);              ///< non-copyable
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
        time   = _time;
        format = _format;
        dims[0] = _dims[0];
        dims[1] = _dims[1];
        dims[2] = _dims[2];
        stride0 = dims[0]*ImageFormatSize(_format);
        stride1 = stride0*dims[1];

        CComSafeArray<byte> tmp(static_cast<unsigned long>(packed_input.size()));
        byte * buf = &tmp.GetAt(0);
        for (size_t i = 0; i < packed_input.size(); ++i)
            buf[0] = packed_input[i];
        data = tmp.Detach();
    }

    ~Image3dObj () {
        if (data) {
            CComSafeArray<byte> tmp;
            CHECK(tmp.Attach(data));
            data = nullptr;
        }
    }

private:
    Image3dObj (const Image3dObj &);              ///< non-copyable
    Image3dObj & operator = (const Image3dObj &); ///< non-assignable
};


struct EcgSeriesObj : public EcgSeries {
    EcgSeriesObj () {
        start_time = 0;
        delta_time = 0;
        samples    = nullptr;
        trig_times = nullptr;
    }

    EcgSeriesObj (double start, double delta, const std::vector<float> & _samples, const std::vector<double> & _trigs) {
        start_time = start;
        delta_time = delta;
        
        {
            CComSafeArray<float> tmp(static_cast<unsigned int>(_samples.size()));
            float * buf = &tmp.GetAt(0);
            for (size_t i = 0; i < _samples.size(); ++i)
                buf[i] = _samples[i];
            samples = tmp.Detach();
        }
        {
            CComSafeArray<double> tmp(static_cast<unsigned int>(_trigs.size()));
            double * buf = &tmp.GetAt(0);
            for (size_t i = 0; i < _trigs.size(); ++i)
                buf[i] = _trigs[i];
            trig_times = tmp.Detach();
        }
    }

    ~EcgSeriesObj () {
        if (samples) {
            CComSafeArray<byte> tmp;
            CHECK(tmp.Attach(samples));
            samples = nullptr;
        }
        if (trig_times) {
            CComSafeArray<byte> tmp;
            CHECK(tmp.Attach(trig_times));
            trig_times = nullptr;
        }
    }

private:
    EcgSeriesObj (const EcgSeriesObj &);              ///< non-copyable
    EcgSeriesObj & operator = (const EcgSeriesObj &); ///< non-assignable
};
