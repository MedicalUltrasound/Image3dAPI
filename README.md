# Image3dAPI
Interfaces for inter-vendor exchange of 3D ultrasound data, together with test code.

## Description
Application Programming Interface (API) to load any vendor’s proprietary ultrasound data, commonly referred to as “raw” data, using a well-defined data structures that can be used to render, display and manipulate 3D data within any vendor’s review and analysis tools.

Each ultrasound vendor will need to provide an implementation of this API specific to their proprietary data format. Any software or analysis tool using the API to load data can be agnostic to the specifics of the data loader, as only the common API needs to be considered.

## Benefits
This will allow analysis tools to support data from multiple vendors, rather than having to use “plugins”, e.g. GE EchoPAC or Philips QLab. This approach also allows the vendors to simplify their commercial offerings to end-users since it will no longer be required to purchase multiple plugins from multiple vendors and coordinate multiple installs.

Furthermore, companies outside the ultrasound system vendors (such as 3D printing, Virtual Reality, Surgical Planning, researchers, etc.) will be able to load 3D echo data seamlessly from all participating vendors.

## Relationship to DICOM
The DICOM 3D standard is an alternative solution to sharing of 3D ultrasound data. However, DICOM 3D does not allow access to studies that are already stored in existing 3D proprietary formats. Furthermore, the limited vendor adoption of DICOM 3D means that it will take years for it to truly impact the market.

This approach has advantages over the traditional DICOM data standard technique including complete seamless backward compatibility to existing stored studies, smaller files sizes, faster network transfers, and faster loading of studies (since no conversion from vendor raw data to DICOM format is required). This new approach also allows the vendors to continually innovate as well, instead of being locked into a frozen data specification.

## Getting started
* Open Visual Studio with administrative privileges. Any version >= 2015 is supported.
* Open `Image3dAPI.sln`.
* Build the solution.

## Documentation
* [API license](LICENSE.txt)
* [Guidelines](Guidelines.md) for implementing the interface
