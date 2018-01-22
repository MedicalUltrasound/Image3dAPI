## Sample code to demonstrate how to access Image3dAPI from a python script
import platform
import comtypes
import comtypes.client
import numpy as np


def SafeArrayToNumpy(safearr_obj):
    """Convert a SAFEARRAY buffer to its numpy equivalent"""
    import ctypes

    # only support 1D data for now
    assert(comtypes._safearray.SafeArrayGetDim(safearr_obj) == 1)

    data_ptr = ctypes.POINTER(safearr_obj._itemtype_)()
    comtypes._safearray.SafeArrayAccessData(safearr_obj, ctypes.byref(data_ptr))
    
    upper_bound = comtypes._safearray.SafeArrayGetUBound(safearr_obj, 1) + 1 # +1 to go from inclusive to exclusive bound
    lower_bound = comtypes._safearray.SafeArrayGetLBound(safearr_obj, 1)
    array_size = upper_bound - lower_bound

    return np.ctypeslib.as_array(data_ptr,shape=(array_size,))


if __name__=="__main__":
    # load type library
    if "32" in platform.architecture()[0]:
        Image3dAPI = comtypes.client.GetModule("../Win32/Image3dAPI.tlb")
    else:
        Image3dAPI = comtypes.client.GetModule("../x64/Image3dAPI.tlb")

    # create loader object
    loader = comtypes.client.CreateObject("DummyLoader.Image3dFileLoader")
    loader = loader.QueryInterface(Image3dAPI.IImage3dFileLoader)

    # load file
    loader.LoadFile("dummy.dcm")
    source = loader.GetImageSource()

    # retrieve probe info
    probe = source.GetProbeInfo()
    print("Probe name: "+probe.name)
    print("Probe type: "+str(probe.type))

    # retrieve ECG info
    ecg = source.GetECG()
    print("ECG start: "+str(ecg.start_time))
    print("ECG delta: "+str(ecg.delta_time))
    samples = SafeArrayToNumpy(ecg.samples)
    trig_times = SafeArrayToNumpy(ecg.trig_times)

    # get bounding box
    bbox = source.GetBoundingBox()
    origin = [bbox.origin_x, bbox.origin_y, bbox.origin_z]
    dir1   = [bbox.dir1_x,   bbox.dir1_y,   bbox.dir1_z]
    dir1   = [bbox.dir2_x,   bbox.dir2_y,   bbox.dir2_z]
    dir1   = [bbox.dir3_x,   bbox.dir3_y,   bbox.dir3_z]

    color_map = source.GetColorMap()

    frame_count = source.GetFrameCount()
    for i in range(frame_count):
        max_res = np.ctypeslib.as_ctypes(np.array([128, 128, 128], dtype=np.ushort))
        frame = source.GetFrame(0, bbox, max_res)
        
        print("Frame time: "+str(frame.time))
        print("Frame format: "+str(frame.format))
        dims = frame.dims
        print("Frame dims: ("+str(dims[0])+", "+str(dims[1])+", "+str(dims[2])+")")
        frame.stride0
        frame.stride1
        data = SafeArrayToNumpy(frame.data)
        print("Frame data shape: "+str(data.shape))
        # TODO: Reconstruct a 3D image matrix based on dims & stride info
