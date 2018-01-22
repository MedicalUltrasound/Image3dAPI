from __future__ import print_function
import fileinput
import os
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
    
    return dep_dict


def UpdateAutopkg (filename, version, url, dependencies):
    '''Set the minor version in an autopkg or nuspec file'''
    IS_NUSPEC = os.path.splitext(filename)[-1].lower() == '.nuspec'
    
    for line in fileinput.input(filename, inplace=True):
        if '<<GIT-VERSION>>' in line:
            # use current git version
            print(line.replace('<<GIT-VERSION>>', version), end='')
        elif '<<SOURCE-URL>>' in line:
            print(line.replace('<<SOURCE-URL>>', url), end='')
        elif '<<VERSION>>' in line:
            if IS_NUSPEC:
                # line on "<dependency id="xxx" version="<<VERSION>>" />" format
                tokens = line.split()
                for token in tokens:
                    if token[:4] == 'id="':
                        package = token[4:-1]
                    elif token[:6] == 'version="':
                        pass # assume that tag contains "<<VERSION>>" string
                line = '        <dependency id="'+package+'" version="'+dependencies[package]+'" />'
                print(line, end='\n')
            else:
                # use version from nuget config
                idx = line.find('/<<VERSION>>')
                package = line[:idx].split()[-1]
                print(line.replace('<<VERSION>>', dependencies[package]), end='')
        elif '<<DEPENDENCIES>>' in line:
            if IS_NUSPEC:
                # convert dependency dict to nuspec XML representation
                dep_str = ''
                for key in dependencies:
                    dep_str += '<dependency id="'+key+'" version="'+dependencies[key]+'" />\n        '
                print(line.replace('<<DEPENDENCIES>>', dep_str), end='')
            else:
                # convert dependency dict to string representation on 'dep-name/version'-format
                dep_str = ''
                for key in dependencies:
                    dep_str += key+'/'+dependencies[key]+', '
                dep_str = dep_str[:-2] # rm trailing ', '
                print(line.replace('<<DEPENDENCIES>>', dep_str), end='')
        else:
            print(line, end='')


if __name__ == "__main__":
    filename = sys.argv[1] # autopkg file
    version  = sys.argv[2] # version number
    url      = sys.argv[3] # release URL
    
    # parse dependencies from NuGet config file arguments
    deps = {}
    for i in range(4, len(sys.argv)):
        print('Parsing dependencies from '+sys.argv[i])
        tmp = IdentifyDependencies(sys.argv[i])
        for key in tmp:
            if key in deps:
                if deps[key] != tmp[key]:
                    raise BaseException('Version mismatch in package '+key+': '+deps[key]+' vs. '+tmp[key])
            else:
                deps[key] = tmp[key]
    
    print('Setting version and URL of '+filename+' to '+version+' and '+url)
    UpdateAutopkg(filename, version, url, deps)
