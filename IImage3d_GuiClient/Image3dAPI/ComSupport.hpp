/* "Plugin API" convenience functions.
Designed by Fredrik Orderud <fredrik.orderud@ge.com>.
Copyright (c) 2016, GE Healthcare, Ultrasound.           */
#pragma once

#include <vector>
#include <stdexcept>
#include <cassert>
#include <comdef.h>

#define _ATL_ATTRIBUTES // must be defined before all ATL includes
#include <atlbase.h>
#include <atlsafe.h> // for CComSafeArray
#include <atlcom.h>


/** Converts unicode string to ASCII */
static inline std::string ToAscii (const std::wstring& w_str) {
#pragma warning(push)
#pragma warning(disable: 4996) // function or variable may be unsafe
    size_t N = w_str.size();
    std::string s_str;
    s_str.resize(N);

    wcstombs((char*)s_str.c_str(), w_str.c_str(), N);

    return s_str;
#pragma warning(pop)
}


/** Translate COM HRESULT failure into exceptions. */
static void CHECK (HRESULT hr) {
    if (FAILED(hr)) {
        _com_error err(hr);
#ifdef _UNICODE
        const wchar_t * msg = err.ErrorMessage(); // weak ptr.
        throw std::runtime_error(ToAscii(msg));
#else
        const char * msg = err.ErrorMessage(); // weak ptr.
        throw std::runtime_error(msg);
#endif
    }
}


/* Disable BSTR caching to ease memory management debugging.
REF: http://msdn.microsoft.com/en-us/library/windows/desktop/ms644360.aspx */
extern "C" void SetOaNoCache();



/** RAII class for COM initialization. */
class ComInitialize {
public:
    ComInitialize (COINIT apartment /*= COINIT_MULTITHREADED*/) : m_initialized(false) {
        // REF: https://msdn.microsoft.com/en-us/library/windows/desktop/ms695279.aspx
        HRESULT hr = CoInitializeEx(NULL, apartment);
        if (SUCCEEDED(hr))
            m_initialized = true;

#ifdef _DEBUG
        SetOaNoCache();
#endif
    }

    ~ComInitialize() {
        if (m_initialized)
            CoUninitialize();
    }

private:
    bool m_initialized; ///< must uninitialize in dtor
};


/** Convenience function to create a locally implemented COM instance without the overhead of CoCreateInstance.
The COM class does not need to be registred for construction to succeed. However, lack of registration can
cause problems if transporting the class out-of-process. */
template <class T>
static CComPtr<T> CreateLocalInstance() {
    // create an object (with ref. count zero)
    CComObject<T> * tmp = nullptr;
    CHECK(CComObject<T>::CreateInstance(&tmp));

    // move into smart-ptr (will incr. ref. count to one)
    return CComPtr<T>(tmp);
}


/** Convert SafeArray to a std::vector. */
template <class T>
static std::vector<T> ConvertToVector (const CComSafeArray<T> & input) {
    std::vector<T> result(input.GetCount());
    if (result.size() > 0) {
        const T * in_ptr = &input.GetAt(0); // will fail if empty
        for (size_t i = 0; i < result.size(); ++i)
            result[i] = in_ptr[i];
    }
    return result;
}

/** Convert raw array to SafeArray. */
template <class T>
static CComSafeArray<T> ConvertToSafeArray (const T * input, size_t element_count) {
    CComSafeArray<T> result(static_cast<unsigned long>(element_count));
    if (element_count > 0) {
        T * out_ptr = &result.GetAt(0); // will fail if empty
        for (size_t i = 0; i < element_count; ++i)
            out_ptr[i] = input[i];
    }
    return result;
}
