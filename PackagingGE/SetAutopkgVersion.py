from __future__ import print_function
import fileinput
import sys
from xml.dom import minidom


def IdentifyDependencies (filename):
    '''Parse NuGet dependencies from a 'packages.config'-style file'''
    
    # parse dependencies into a dict
    xml_doc = minidom.parse(filename)
    dep_dict = {}
    for package in xml_doc.childNodes[0].childNodes:
        if package.nodeName == 'package':
            id      = package.attributes['id'].value
            version = package.attributes['version'].value
            dep_dict[id] = version
    
    # convert dict to string representation on 'dep-name/version'-format
    dep_str = ''
    for key in dep_dict:
        dep_str += key+'/'+dep_dict[key]+', '
    
    return dep_str


def UpdateAutopkg (filename, version, url, dependencies):
    '''Set the minor version in an autopkg file'''
    
    for line in fileinput.input(filename, inplace=True):
        if '<<VERSION>>' in line:
            print(line.replace('<<VERSION>>', version))
        elif '<<SOURCE-URL>>' in line:
            print(line.replace('<<SOURCE-URL>>', url))
        elif '<<DEPENDENCIES>>' in line:
            print(line.replace('<<DEPENDENCIES>>', dependencies))
        else:
            print(line, end='')


if __name__ == "__main__":
    filename = sys.argv[1]
    version  = sys.argv[2]
    url      = sys.argv[3]
    
    if len(sys.argv) >= 5:
        print('Parsing dependency information')
        deps = IdentifyDependencies(sys.argv[4])
    else:
        deps = ''
    
    print('Setting version and URL of '+filename+' to '+version+' and '+url)
    UpdateAutopkg(filename, version, url, deps)
