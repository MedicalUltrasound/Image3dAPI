# Image3dAPI
Interfaces for inter-vendor exchange of 3D ultrasound data, together with test code.


## Getting started
* Open Visual Studio with administrative privileges. Any version >= 2015 is supported.
* Open `Image3dAPI.sln`.
* Build the solution.

## Guidelines for implementing the interface

### Performance

* Data loading of a typical image should take less than 3 seconds, including fetching all frames (from local HDD).
* IImage3dFileLoader::LoadFile must be fast (1-10 ms). I.e. This is needed to permit scanning many DICOM files to see if they can be loaded. LoadFile should not do scan conversion (i.e., from ultrasound scan-lines to a 3D Cartesian volume) by itself. This conversion should be done only when the volume data is actually requested.
* It is strongly recommended to avoid caching of frames inside the loader. It should be up to the host to perform necessary caching. This allows the host to limit memory usage depending on available resources. Hence, loading image frame data from file and scan conversion/image processing should be delayed until Iimage3dSource::GetFrame is called.

### Robustness

* IImage3dFileLoader::LoadFile should return success only if the file can be read as 3D data.
* If IImage3dFileLoader::LoadFile returns success, the file must be readable with the loader. 
* IImage3dFileLoader::LoadFile should be well behaved when and after provided with non-conforming DICOM files or non-DICOM files. 
* API methods shall not throw exceptions. If it is possible for exceptions to be thrown in the internal implementation, they should be caught within the component, and translated into a failure return value for the IImage3D API method call.
* IImage3D objects should expect themselves to be launched as out-of-process COM servers, because the client software will want to prevent itself from failing if there is a failure in the IImage3D object.

### Environment

* The loader should be compiled both in 32 and 64 bits.
* A DirectX 11 compliant GPU is required for interactive review of 3D images. The loader should still be well behaved when running without a DirectX 11 compliant GPU, but without any performance guarantees. 
* The minimum requirement for CPU is 2GHz Intel Core i-series CPU or better, with 2 GB RAM or more. 
* The loader should support all resolutions (as specified by Image3dSource::GetFrame parameter max_resolution). It is up to the workstation integration team to assess memory consumption/performance and determine suitable limits for max_resolution upon integration. The loader should be well behaved even though it cannot support the max_resolution requested by the client.
