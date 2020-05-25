## Sample code to demonstrate how to access Image3dAPI from a python script
import platform
import comtypes
import comtypes.client
import numpy as np


def TypeLibFromObject (object):
    """Loads the type library associated with a COM class instance"""
    import winreg

    with winreg.OpenKey(winreg.HKEY_CLASSES_ROOT, "CLSID\\"+object.__clsid+"\\TypeLib", 0, winreg.KEY_READ) as key:
        typelib = winreg.EnumValue(key, 0)[1]
    with winreg.OpenKey(winreg.HKEY_CLASSES_ROOT, "CLSID\\"+object.__clsid+"\\Version", 0, winreg.KEY_READ) as key:
        version = winreg.EnumValue(key, 0)[1]

    try:
        major_ver, minor_ver = version.split(".")
        return comtypes.client.GetModule([typelib, int(major_ver), int(minor_ver)])
    except OSError as err:
        # API 1.2-only compatibility fallback to avoid breaking existing loaders
        if (version != "1.2") or (err.winerror != -2147319779): # Library not registered
            raise # rethrow
        # Fallback to TypeLib version 1.0
        return comtypes.client.GetModule([typelib, 1, 0])


def SafeArrayToNumpy (safearr_ptr, copy=True):
    """Convert a SAFEARRAY buffer to its numpy equivalent"""
    import ctypes

    # only support 1D data for now
    assert(comtypes._safearray.SafeArrayGetDim(safearr_ptr) == 1)

    # access underlying pointer
    data_ptr = ctypes.POINTER(safearr_ptr._itemtype_)()
    comtypes._safearray.SafeArrayAccessData(safearr_ptr, ctypes.byref(data_ptr))
    
    upper_bound = comtypes._safearray.SafeArrayGetUBound(safearr_ptr, 1) + 1 # +1 to go from inclusive to exclusive bound
    lower_bound = comtypes._safearray.SafeArrayGetLBound(safearr_ptr, 1)
    array_size = upper_bound - lower_bound

    # wrap pointer in numpy array
    arr = np.ctypeslib.as_array(data_ptr,shape=(array_size,))
    return np.copy(arr) if copy else arr


def FrameTo3dArray (frame):
    """Convert Image3d data into a numpy 3D array"""
    arr_1d = SafeArrayToNumpy(frame.data, copy=False)
    assert(arr_1d.dtype == np.uint8) # only tested with 1byte/elm

    arr_3d = np.lib.stride_tricks.as_strided(arr_1d, shape=frame.dims, strides=(1, frame.stride0, frame.stride1))
    return np.copy(arr_3d) 

