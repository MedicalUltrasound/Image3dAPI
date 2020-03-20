## Sample code to demonstrate how to access Image3dAPI from a python script
import platform
import comtypes
import comtypes.client
import numpy as np
import SimpleITK as sitk


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


def SaveITKImage(imgFrame, bbox, outputFilename):
    array = FrameTo3dArray(imgFrame)
    itk_img = sitk.GetImageFromArray(array.astype("float64"))

    m2mm = 1000

    dir1   = [bbox.dir1_x,   bbox.dir1_y,   bbox.dir1_z]
    dir2   = [bbox.dir2_x,   bbox.dir2_y,   bbox.dir2_z]
    dir3   = [bbox.dir3_x,   bbox.dir3_y,   bbox.dir3_z]

    # all units from Image3dAPI are in meters according to https://github.com/MedicalUltrasound/Image3dAPI/wiki#image-geometry
    spacingX = np.linalg.norm(dir1) / imgFrame.dims[0] * m2mm # convert from meters to millimeters
    spacingY = np.linalg.norm(dir2) / imgFrame.dims[1] * m2mm
    spacingZ = np.linalg.norm(dir3) / imgFrame.dims[2] * m2mm


    dir1 = dir1 / np.linalg.norm(dir1)
    dir2 = dir2 / np.linalg.norm(dir2)
    dir3 = dir3 / np.linalg.norm(dir3)

    itk_img.SetSpacing((spacingX,spacingY,spacingZ))
    itk_img.SetOrigin((bbox.origin_x*m2mm, bbox.origin_y*m2mm, bbox.origin_z*m2mm))
    itk_img.SetDirection((dir1[0], dir2[0], dir3[0], dir1[1], dir2[1], dir3[1], dir1[2], dir2[2], dir3[2]))

    sitk.WriteImage(itk_img, outputFilename)

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
    err_type, err_msg = loader.LoadFile("dummy.dcm")
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
    dir2   = [bbox.dir2_x,   bbox.dir2_y,   bbox.dir2_z]
    dir3   = [bbox.dir3_x,   bbox.dir3_y,   bbox.dir3_z]

    color_map = source.GetColorMap()
    print("Color-map length: "+str(len(color_map)))

    frame_count = source.GetFrameCount()
    for i in range(frame_count):
        max_res = np.ctypeslib.as_ctypes(np.array([64, 64, 64], dtype=np.ushort))
        frame = source.GetFrame(i, bbox, max_res)
        
        print("Frame metadata:")
        print("  time:   "+str(frame.time))
        print("  format: "+str(frame.format))
        print("  dims:  ("+str(frame.dims[0])+", "+str(frame.dims[1])+", "+str(frame.dims[2])+")")

        data = FrameTo3dArray(frame)
        print("  shape: "+str(data.shape))

        SaveITKImage(frame, bbox, "image3DAPIDummOutput" + str(i) + ".mhd")
