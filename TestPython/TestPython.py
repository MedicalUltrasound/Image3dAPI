## Sample code to demonstrate how to access Image3dAPI from a python script
import platform
import comtypes
import comtypes.client
import numpy as np
from utils import SafeArrayToNumpy
from utils import FrameTo3dArray
from utils import TypeLibFromObject


if __name__=="__main__":
    # create loader object
    loader = comtypes.client.CreateObject("DummyLoader.Image3dFileLoader")
    # cast to IImage3dFileLoader interface
    Image3dAPI = TypeLibFromObject(loader)
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

    color_format, color_map = source.GetColorMap(Image3dAPI.TYPE_TISSUE_COLOR)
    print("Color-map length: "+str(len(color_map)/4))

    max_res = np.ctypeslib.as_ctypes(np.array([64, 64, 64], dtype=np.ushort))
    stream = source.GetStream(0, bbox, max_res)

    stream_type = stream.GetType()
    print("Stream type "+str(stream_type))

    frame_count = stream.GetFrameCount()
    for i in range(frame_count):
        frame = stream.GetFrame(i)
        
        print("Frame metadata:")
        print("  time:   "+str(frame.time))
        print("  format: "+str(frame.format))
        print("  dims:  ("+str(frame.dims[0])+", "+str(frame.dims[1])+", "+str(frame.dims[2])+")")

        data = FrameTo3dArray(frame)
        print("  shape: "+str(data.shape))

