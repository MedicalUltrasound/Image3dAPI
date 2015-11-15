/* C++ RAII wrappers of IIMage3d structs. */
#pragma once
#include "ComSupport.hpp"
#include "IImage3d.h"


struct ProbeInfoObj : public ProbeInfo {
    ProbeInfoObj () {
        type = PROBE_UNKNOWN;
        name = nullptr;
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
        stride0 = 0;
        stride1 = 0;
        data    = nullptr;
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
