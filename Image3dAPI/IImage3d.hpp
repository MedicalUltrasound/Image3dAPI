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
