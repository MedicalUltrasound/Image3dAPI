/* C++ RAII wrappers of IIMage3d structs.
Designed by Fredrik Orderud <fredrik.orderud@ge.com>.
Copyright (c) 2016, GE Healthcare, Ultrasound.      */
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

/** C++ RAII wrapper of Image3d. */
struct Image3dObj : public Image3d {
    Image3dObj () {
        time    = 0;
        format  = FORMAT_INVALID;
        dims[0] = dims[1] = dims[2] = 0;
        stride0 = 0;
        stride1 = 0;
        data    = nullptr;
    }

    Image3dObj (double _time, ImageFormat _format, unsigned short _dims[3], const byte * in_buf, unsigned int _stride0, unsigned int  _stride1) {
        time    = _time;
        format  = _format;
        dims[0] = _dims[0];
        dims[1] = _dims[1];
        dims[2] = _dims[2];
        stride0 = _stride0;
        stride1 = _stride1;
        assert(stride0 >= static_cast<unsigned int>(dims[0])*ImageFormatSize(_format));
        assert(stride1 >= stride0*dims[1]);

        CComSafeArray<byte> tmp = ConvertToSafeArray(in_buf, stride1*dims[2]);
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

    const byte * DataPtr () const {
        return (const byte*)data->pvData;
    }

    Image3d Detach () {
        Image3d img = {time, format, dims[0], dims[1], dims[2], stride0, stride1, data};
        data = nullptr;
        return img;
    }

private:
    Image3dObj & operator = (const Image3dObj &); ///< non-assignable
};
static_assert(sizeof(Image3dObj) == sizeof(Image3d), "Image3dObj size mismatch");
