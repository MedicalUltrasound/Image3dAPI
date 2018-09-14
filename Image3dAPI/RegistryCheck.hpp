#pragma once
#include <string>
#include "ComSupport.hpp"


/** Compare the version of a COM class against the current Image3dAPI version.
    NOTE: Not compatible with reg-free COM, since the COM class version is not included in the manifest. */
static HRESULT CheckImage3dAPIVersion (CLSID clsid) {
    // build registry path")
    CComBSTR reg_path(L"CLSID\\");
    reg_path.Append(clsid);
    reg_path.Append(L"\\Version");

    // extract COM class version
    CRegKey cls_reg;
    if (cls_reg.Open(HKEY_CLASSES_ROOT, reg_path, KEY_READ) != ERROR_SUCCESS)
        return E_NOT_SET;
    ULONG    ver_str_len = 0;
    if (cls_reg.QueryStringValue(nullptr, nullptr, &ver_str_len) != ERROR_SUCCESS)
        return E_NOT_SET;
    std::wstring ver_str(ver_str_len, L'\0');
    if (cls_reg.QueryStringValue(nullptr, const_cast<wchar_t*>(ver_str.data()), &ver_str_len) != ERROR_SUCCESS)
        return E_NOT_SET;
    ver_str.resize(ver_str_len-1); // remove extra zero-termination

    // compare against current Image3dAPI version
    std::wstring cur_ver = std::to_wstring(Image3dAPIVersion::IMAGE3DAPI_VERSION_MAJOR)
        +L"."+std::to_wstring(Image3dAPIVersion::IMAGE3DAPI_VERSION_MINOR);
    if (ver_str == cur_ver)
        return S_OK;
    else
        return E_INVALIDARG; // version mismatch
}
