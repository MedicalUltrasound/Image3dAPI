/* Dummy test loader for the "3D API".
Designed by Fredrik Orderud <fredrik.orderud@ge.com>.
Copyright (c) 2020, GE Healthcare, Ultrasound.      */
#pragma once

#define _USE_MATH_DEFINES // for M_PI
#include <cmath>
#include <memory>
#include <array>
#include <vector>
#include "../Image3dAPI/ComSupport.hpp"
#include "../Image3dAPI/IImage3d.h"

#include "DummyLoader.h"
#include "Resource.h"


/** RGBA color struct that matches DXGI_FORMAT_R8G8B8A8_UNORM.
    Created due to the lack of such a class/struct in the Windows or Direct3D SDKs.
    Please remove this class if a more standardized alternative is available. */
struct R8G8B8A8 {
    R8G8B8A8 () {
    }

    R8G8B8A8 (unsigned char _r, unsigned char _g, unsigned char _b, unsigned char _a) : r(_r), g(_g), b(_b), a(_a) {
    }

    operator unsigned int () const {
        return *reinterpret_cast<const unsigned int*>(this);
    }

    uint8_t r = 0; ///< color channels
    uint8_t g = 0;
    uint8_t b = 0;
    uint8_t a = 0;
};

/** Determine the sample size [bytes] for a given image format. */
static unsigned int ImageFormatSize(ImageFormat format) {
    switch (format) {
    case IMAGE_FORMAT_U8: return 1;
    }

    abort(); // should never be reached
}

/** Create a Image3d object from a std::vector buffer. */
static Image3d CreateImage3d (double time, ImageFormat format, const unsigned short dims[3], const std::vector<byte> &img_buf) {
    assert(img_buf.size() == ImageFormatSize(format)*dims[0]*dims[1]*dims[2]);

    Image3d img;
    img.time = time;
    img.format = format;
    for (size_t i = 0; i < 3; ++i)
        img.dims[i] = dims[i];

    CComSafeArray<BYTE> data(static_cast<unsigned int>(img_buf.size()));
    memcpy(data.m_psa->pvData, img_buf.data(), img_buf.size());
    img.data = data.Detach();

    // assume packed storage
    img.stride0 = dims[0] * ImageFormatSize(format);
    img.stride1 = dims[1] * img.stride0;

    return img;
}
