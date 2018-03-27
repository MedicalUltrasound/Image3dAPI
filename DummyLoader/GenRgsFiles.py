import re

text = """HKCR
{
	PROG-NAME.CLASS-NAME.1 = s 'CLASS-NAME Object'
	{
		CLSID = s '{CLASS-GUID}'
	}
	PROG-NAME.CLASS-NAME = s 'CLASS-NAME Object'
	{		
		CLSID = s '{CLASS-GUID}'
		CurVer = s 'PROG-NAME.CLASS-NAME.1'
	}
	NoRemove CLSID
	{
		ForceRemove {CLASS-GUID} = s 'CLASS-NAME Object'
		{
			ProgID = s 'PROG-NAME.CLASS-NAME.1'
			VersionIndependentProgID = s 'PROG-NAME.CLASS-NAME'
			InprocServer32 = s '%MODULE%'
			{
				val ThreadingModel = s 'THREAD-MODEL'
			}
			TypeLib = s '{TYPE-LIB}'
			Version = s 'VERSION_MAJOR.VERSION_MINOR'
		}
	}
}
"""

def GenRgsFiles(progname, typelib, version, classes, threadmodel, concat_filename=None):
    all_rgs_content = ''
    for cls in classes:
        content = text
        content = content.replace('PROG-NAME', progname)
        content = content.replace('TYPE-LIB', typelib)
        content = content.replace('VERSION_MAJOR', str(version[0]))
        content = content.replace('VERSION_MINOR', str(version[1]))
        content = content.replace('THREAD-MODEL', threadmodel)
        content = content.replace('CLASS-NAME', cls[0])
        content = content.replace('CLASS-GUID', cls[1])
        
        #print(content)
        filename = cls[0]+'.rgs'
        with open(filename, 'w') as f:
            f.write(content)
            all_rgs_content += content
        print('Written '+filename)
    
    # write concatenation of all RGS files into a separate file (needed for reg-free COM)
    if concat_filename:
        with open(concat_filename, 'w') as f:
            f.write(all_rgs_content)
        print('Written '+concat_filename)


def ParseUuidString (str):
    uuid = str[str.find('uuid(')+5:]
    return uuid[:uuid.find(')')]


def ParseIdl (filename):
    '''Parse IDL file to determine library name, typelib GUID and classes with associated GUIDs'''
    with open(filename, 'r') as f:
        source = f.read()

    attribs = '\[[^[]+\]\s*'     # detect [...] attribute blocks
    name    = '\s+[a-zA-Z0-9_]+' # detect coclass/interface name
        
    # find "library" name and associated typelib uuid
    for lib_hit in re.findall(attribs+'library'+name, source):
        tokens = lib_hit.split()
        for token in tokens:
            if 'uuid(' in token:
                typelib = ParseUuidString(token)
                break
        libname = tokens[-1]

    # find "coclass" names with associated uuids
    classes = [] 
    for cls_hit in re.findall(attribs+'coclass'+name, source):
        tokens = cls_hit.split()
        for token in tokens:
            if 'uuid(' in token:
                uuid = ParseUuidString(token)
                break
        classes.append([tokens[-1], uuid])
    
    return libname, typelib, classes

def ParseImage3dAPIVersion (filename):
    with open(filename, 'r') as f:
        for line in f:
            if "IMAGE3DAPI_VERSION_MAJOR =" in line:
                major = int(line.split()[-1][:-1])
            elif "IMAGE3DAPI_VERSION_MINOR =" in line:
                minor = int(line.split()[-1][:-1])
    return [major,minor]


if __name__ == "__main__":
    libname, typelib, classes = ParseIdl('DummyLoader.idl')
    version = ParseImage3dAPIVersion("../Image3dAPI/IImage3d.idl")
    GenRgsFiles(libname, typelib, version, classes, 'Both')
