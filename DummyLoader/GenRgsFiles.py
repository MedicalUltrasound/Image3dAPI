import re

progid_template1 = """
	PROG-NAME.CLASS-NAME.1 = s 'CLASS-NAME Object'
	{
		CLSID = s '{CLASS-GUID}'
	}
	PROG-NAME.CLASS-NAME = s 'CLASS-NAME Object'
	{		
		CLSID = s '{CLASS-GUID}'
		CurVer = s 'PROG-NAME.CLASS-NAME.1'
	}"""

clsid_template1 = """
	NoRemove CLSID
	{
		ForceRemove {CLASS-GUID} = s 'CLASS-NAME Object'
		{"""

progid_template2 = """
			ProgID = s 'PROG-NAME.CLASS-NAME.1'
			VersionIndependentProgID = s 'PROG-NAME.CLASS-NAME'"""

clsid_template2 = """
			InprocServer32 = s '%MODULE%'
			{
				val ThreadingModel = s 'THREAD-MODEL'
			}
			TypeLib = s '{TYPE-LIB}'
			Version = s 'VERSION'
			ADDITIONAL-REGISTRY-ENTRIES
		}
	}"""

class ComClass:
    def __init__(self, name, uuid, version, creatable):
        self.name = name
        self.uuid = uuid
        self.version = version
        self.creatable = creatable
        self.additional_entries = '' # default


def GenRgsFiles (progname, typelib, classes, threadmodel, concat_filename=None):
    all_rgs_content = ''
    for cls in classes:
        content = "HKCR\n{"
        if cls.creatable:
            content += progid_template1
        content += clsid_template1
        if cls.creatable:
            content += progid_template2
        content += clsid_template2
        content  += "\n}\n"
        
        content = content.replace('PROG-NAME', progname)
        content = content.replace('TYPE-LIB', typelib)
        content = content.replace('THREAD-MODEL', threadmodel)
        content = content.replace('CLASS-NAME', cls.name)
        content = content.replace('CLASS-GUID', cls.uuid)
        content = content.replace('VERSION',    cls.version)
        content = content.replace('ADDITIONAL-REGISTRY-ENTRIES', cls.additional_entries)
        
        #print(content)
        filename = cls.name+'.rgs'
        with open(filename, 'w') as f:
            f.write(content)
            all_rgs_content += content
        print('Written '+filename)
    
    # write concatenation of all RGS files into a separate file (needed for reg-free COM)
    if concat_filename:
        with open(concat_filename, 'w') as f:
            f.write(all_rgs_content)
        print('Written '+concat_filename)


def ParseUuidString (val):
    uuid = val[val.find('uuid(')+5:]
    return uuid[:uuid.find(')')]

def ParseVersionString (val):
    ver = val[val.find('version(')+8:]
    return ver[:ver.find(')')]


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

    # find "coclass" names with associated uuid and version
    classes = [] 
    for cls_hit in re.findall(attribs+'coclass'+name, source):
        tokens = cls_hit.split()
        for token in tokens:
            if 'uuid(' in token:
                uuid = ParseUuidString(token)
            elif 'version(' in token:
                version = ParseVersionString(token)
        classes.append(ComClass(tokens[-1], uuid, version, creatable=True))
    
    return libname, typelib, classes

def ParseImage3dAPIVersion (filename):
    with open(filename, 'r') as f:
        for line in f:
            if "IMAGE3DAPI_VERSION_MAJOR =" in line:
                major = int(line.split()[-1][:-1])
            elif "IMAGE3DAPI_VERSION_MINOR =" in line:
                minor = int(line.split()[-1][:-1])
    return str(major)+"."+str(minor)


if __name__ == "__main__":
    libname, typelib, classes = ParseIdl('DummyLoader.idl')
    version = ParseImage3dAPIVersion("../Image3dAPI/IImage3d.idl")
    
    # verify that COM class versions matches API version
    for cls in classes:
        if cls.version != version:
            raise Exception("Version mismatch detected for "+cls.name)
        if cls.name == "Image3dFileLoader":
            cls.additional_entries += '''
			val AppID = s '%APPID%'
			SupportedManufacturerModels
			{
				val 'Dummy medical systems' = s 'Super scanner *'
				val 'Dummy healthcare'      = s 'Some scanner 1;Some scanner 2'
			}'''
        else:
            cls.creatable = False
    
    GenRgsFiles(libname, typelib, classes, 'Both')
