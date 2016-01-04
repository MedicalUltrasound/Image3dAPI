/* Dummy test loader for the "3D API".
Designed by Fredrik Orderud <fredrik.orderud@ge.com>.
Copyright (c) 2016, GE Healthcare, Ultrasound.      */

#include "Image3dSource.hpp"
#include "Image3dFileLoader.hpp"

[module(
#ifdef _WINDLL
    dll,
#else
    exe,
#endif
    name = "DummyLoader", version = "1.0", uuid = "67E59584-3F6A-4852-8051-103A4583CA5E", helpstring = "DummyLoader module")];
