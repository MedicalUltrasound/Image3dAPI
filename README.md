# Image3dAPI
Interfaces for inter-vendor exchange of 3D ultrasound data, together with test code.

## Description
Application Programming Interface (API) to load any vendor’s proprietary ultrasound data, commonly referred to as “raw” data, using a well-defined data structures that can be used to render, display and manipulate 3D data within any vendor’s review and analysis tools.

Each ultrasound vendor will need to provide an implementation of this API specific to their proprietary data format. Any software or analysis tool using the API to load data can be agnostic to the specifics of the data loader, as only the common API needs to be considered.

### Content
* [DummyLoader](DummyLoader/) - Example loader library
* [Image3dAPI](Image3dAPI/)   - API definitions
* [PackagingGE](PackagingGE/) - NuGet packaging configuration
* [RegFreeTest](RegFreeTest/) - Example of how to leverage manifest files to avoid COM registration
* [SandboxTest](SandboxTest/) - Example of how to sandbox a loader in a separate process
* [TestPython](TestPython/)   - Python-based sample code
* [TestViewer](TestViewer/)   - Simple .NET-based image viewer

## Benefits
This will allow analysis tools to support data from multiple vendors, rather than having to use “plugins”, e.g. GE EchoPAC or Philips QLab. This approach also allows the vendors to simplify their commercial offerings to end-users since it will no longer be required to purchase multiple plugins from multiple vendors and coordinate multiple installs.

Furthermore, companies outside the ultrasound system vendors (such as 3D printing, Virtual Reality, Surgical Planning, researchers, etc.) will be able to load 3D echo data seamlessly from all participating vendors.

## Relationship to DICOM
The DICOM 3D standard is an alternative solution to sharing of 3D ultrasound data. However, DICOM 3D does not allow access to studies that are already stored in existing 3D proprietary formats. Furthermore, the limited vendor adoption of DICOM 3D means that it will take years for it to truly impact the market.

This approach has advantages over the traditional DICOM data standard technique including complete seamless backward compatibility to existing stored studies, smaller files sizes, faster network transfers, and faster loading of studies (since no conversion from vendor raw data to DICOM format is required). This new approach also allows the vendors to continually innovate as well, instead of being locked into a frozen data specification.

## Getting started
Install [Visual Studio](https://visualstudio.microsoft.com/), either professional or community edition. Any version >= 2015 is supported.
Enable the following workloads during installation:
* .Net desktop development,
* Desktop development with C++, and
* Python development.

In addition, Python.exe need to be registered as default app for running .py scripts. This can be done by right-clicking on a .py file, then selecting "Open with", "Choose another app", locate Python.exe and enable "Always use this app to open .py files".


Build instructions:
* Open Visual Studio with administrative privileges.
* Open `Image3dAPI.sln`.
* Build the solution.

## Documentation
* [API license](LICENSE.txt)
* [Guidelines](Guidelines.md) for implementing the interface
