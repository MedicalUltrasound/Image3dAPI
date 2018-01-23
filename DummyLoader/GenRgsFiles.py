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
			Version = s '1.0'
		}
	}
}
"""

def GenRgsFiles(progname, typelib, classes, threadmodel, concat_filename=None):
    all_rgs_content = ''
    for cls in classes:
        content = text
        content = content.replace('PROG-NAME', progname)
        content = content.replace('TYPE-LIB', typelib)
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


if __name__ == "__main__":
    # the GUID values here must be unique and in sync with GUIDs in DummyLoader.idl
    # Use guidgen.exe to generate them (included with Visual Studio)
    classes = [['Image3dFileLoader', '8E754A72-0067-462B-9267-E84AF84828F1'],
               ['Image3dSource',     '6FA82ED5-6332-4344-8417-DEA55E72098C'],
              ]
    
    GenRgsFiles('DummyLoader', '67E59584-3F6A-4852-8051-103A4583CA5E', classes, 'Both')
