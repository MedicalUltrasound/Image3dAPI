#include "Image3dSource.hpp"
#include "Image3dFileLoader.hpp"

#include "resource.h"
#include "DummyLoader.h"
#include "DummyLoader_i.c"


class DummyLoaderModule :
    public ATL::CAtlDllModuleT<DummyLoaderModule>
{
public:
    DECLARE_LIBID(LIBID_DummyLoader)
    DECLARE_REGISTRY_APPID_RESOURCEID(IDR_AppID, "{AE03BF33-C065-4DB2-94E3-8167CF9B1E23}")
};

DummyLoaderModule _AtlModule;



// DLL Entry Point
extern "C" BOOL WINAPI DllMain(HINSTANCE /*hInstance*/, DWORD dwReason, LPVOID lpReserved)
{
    return _AtlModule.DllMain(dwReason, lpReserved);
}

// Used to determine whether the DLL can be unloaded by OLE.
STDAPI DllCanUnloadNow()
{
    return _AtlModule.DllCanUnloadNow();
}

// Returns a class factory to create an object of the requested type.
_Check_return_
STDAPI DllGetClassObject(_In_ REFCLSID rclsid, _In_ REFIID riid, _Outptr_ LPVOID* ppv)
{
    return _AtlModule.DllGetClassObject(rclsid, riid, ppv);
}

// DllRegisterServer - Adds entries to the system registry.
STDAPI DllRegisterServer()
{
    // registers object, typelib and all interfaces in typelib
    return _AtlModule.DllRegisterServer();
}

// DllUnregisterServer - Removes entries from the system registry.
STDAPI DllUnregisterServer()
{
    return _AtlModule.DllUnregisterServer();
}

// DllInstall - Adds/Removes entries to the system registry per user per machine.
STDAPI DllInstall(BOOL bInstall, _In_opt_  LPCWSTR pszCmdLine)
{
    static const wchar_t szUserSwitch[] = L"user";

    if (pszCmdLine != NULL) {
        if (_wcsnicmp(pszCmdLine, szUserSwitch, _countof(szUserSwitch)) == 0)
            ATL::AtlSetPerUserRegistration(true);
    }

    HRESULT hr = E_FAIL;
    if (bInstall) {
        hr = DllRegisterServer();
        if (FAILED(hr))
            DllUnregisterServer();
    }
    else {
        hr = DllUnregisterServer();
    }

    return hr;
}
