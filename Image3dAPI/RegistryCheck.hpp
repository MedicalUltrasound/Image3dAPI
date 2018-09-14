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

/** Read list of suported (manufacturer,model)-pairs for a given plugin. */ 
struct SupportedManufacturerModels {
    static std::vector<SupportedManufacturerModels> ReadList(CLSID clsid) {
        // build registry path")
        CComBSTR reg_path(L"CLSID\\");
        reg_path.Append(clsid);
        reg_path.Append(L"\\SupportedManufacturerModels");

        std::vector<SupportedManufacturerModels> list;

        // extract COM class version
        CRegKey cls_reg;
        if (cls_reg.Open(HKEY_CLASSES_ROOT, reg_path, KEY_READ) != ERROR_SUCCESS)
            return list;


        for (DWORD idx = 0;; ++idx) {
            // read key name and value length
            // registry key names are guaranteed to not exceed 16k characters (https://docs.microsoft.com/nb-no/windows/desktop/SysInfo/registry-element-size-limits)
            DWORD name_len = 16384; // key name length (excluding null-termination)
            std::wstring name(name_len, L'\0');
            DWORD type = 0;
            DWORD value_len = 0; // in bytes
            auto res = RegEnumValue(cls_reg, idx, /*out*/const_cast<wchar_t*>(name.data()), &name_len, NULL, &type, nullptr/*out*/, &value_len);
            if (res != ERROR_SUCCESS)
                break;
            name.resize(name_len); // shrink to fit

            if (type != REG_SZ)
                continue; // value not a string

            // read key value
            value_len /= 2; // convert lenght from bytes to #chars (including null-termination)
            std::wstring value(value_len, L'\0');
            res = cls_reg.QueryStringValue(name.c_str(), const_cast<wchar_t*>(value.data()), &value_len);
            if (res != ERROR_SUCCESS)
                break;
            value.resize(value_len-1); // remove null-termination

            // split key value on ';'
            std::vector<std::wstring> models;
            for (size_t pos = 0;;) {
                auto prev = pos;
                pos = value.find(L';', pos);

                if (pos == std::wstring::npos) {
                    // no more separators found
                    models.push_back(value.substr(prev));
                    break;
                } else {
                    models.push_back(value.substr(prev, pos - prev));
                    pos += 1; // skip separator
                }
            }

            list.push_back({name, models});
        }

        return list;
    }

    std::wstring              manufacturer; ///< corresponds to Manufacturer tag, DICOM (0008,0070)
    std::vector<std::wstring> models;       ///< associated Manufacturer's Model Name tags, DICOM (0008,1090). The strings might contain '*' as wildcard
};
