#! /usr/bin/python

import sys
import os
import errno
import shutil
import glob
import tarfile
import zipfile
import platform

def mkdir_p(path):
    try:
        os.makedirs(path)
    except OSError as exc: # Python >2.5
        if exc.errno == errno.EEXIST and os.path.isdir(path):
            pass
        else: raise

MAJOR_VERSION = '@CM_MAJOR_VERSION@'
MINOR_VERSION = '@CM_MINOR_VERSION@'
PATCH_VERSION = '@CM_PATCH_VERSION@'
CM_VERSION = "%s.%s.%s" % (MAJOR_VERSION, MINOR_VERSION, PATCH_VERSION)
ARNOLD_VERSION = '@ARNOLD_VERSION@'

subdirs = [] 

# pattern for files to include in the source distribution
name_src = 'alShaders-src-%s.%s.%s' % (MAJOR_VERSION,MINOR_VERSION,PATCH_VERSION)
ptrn_src = ['*.cpp', '*.h', '*.txt', '*.py', '*.ui', '*.cmake']

# Binary distribution
files_src = ['INSTALL', 'CMakeLists.txt', 'package.in.py', 'README', 'uigen.py']

name_osx = 'alShaders-osx-%s-ai%s' % (CM_VERSION, ARNOLD_VERSION)
name_win = 'alShaders-win-%s-ai%s' % (CM_VERSION, ARNOLD_VERSION)
name_linux = 'alShaders-linux-%s-ai%s' % (CM_VERSION, ARNOLD_VERSION)


def copyPatternsToDistDir(subDirs, subDirPrefix, filePatterns, distDir):
    for d in subDirs:
        destdir = os.path.join(distDir, d)
        mkdir_p(destdir)
        subdir = os.path.join(subDirPrefix, d)
        for ptrn in filePatterns:
            sdir = subdir
            for fn in glob.iglob(os.path.join(sdir, ptrn)):
                shutil.copy(fn, destdir)
                        
def copyFilesToDistDir(files, distDir):
    for fn in files:
        shutil.copy(fn, distDir)
        
def createArchive(distDir, name, isSrc=False):
    if not isSrc and platform.system() == "Windows":
        f = zipfile.ZipFile(os.path.join('..', '%s.zip' % name), 'w')
        for fn in glob.iglob(os.path.join(distDir, '*')):
            f.write(fn, arcname=os.path.join(name, os.path.basename(fn)))
        f.close()
    else:
        f = tarfile.open(os.path.join('..', '%s.tar.gz' % name), 'w:gz')
        f.add(distDir, arcname = name)
        f.close()
    
def createSourceDistribution(name, ptrn_src, files):
    distdir = 'build/%s' % name
    shutil.rmtree(distdir, ignore_errors=True)
    os.mkdir(distdir)
    copyPatternsToDistDir(subdirs, '.', ptrn_src, distdir)
    copyFilesToDistDir(files_src, distdir)
    createArchive(distdir, name, True)

def createBinaryDistribution(name, droot):
    if platform.system() == "Windows":
        zf = zipfile.ZipFile(os.path.join('..', '%s.zip' % name), 'w')
        for root, dirs, files in os.walk(droot):
            rroot = os.path.relpath(root, droot)
            for f in files:
                rfile = os.path.join(rroot, f)
                zf.write(os.path.join(droot, rfile), arcname=rfile)
    else:
        f = tarfile.open(os.path.join('..', '%s.tar.gz' % name), 'w:gz')
        f.add(droot, arcname = name)
        f.close()

# source distribution
createSourceDistribution(name_src, ptrn_src, files_src)

# Binary distribution
droot = 'build/dist/%s/ai%s' % (CM_VERSION, ARNOLD_VERSION)

if platform.system() == "Darwin":
    # OS X distribution
    createBinaryDistribution(name_osx, droot)
elif platform.system() == "Windows":
    # Windows
    createBinaryDistribution(name_win, droot)
elif platform.system() == "Linux":
    # Linux
    createBinaryDistribution(name_linux, droot)
else:
    print 'Warning: unknown system "%s", not creating binary package' % platform.system()

    
