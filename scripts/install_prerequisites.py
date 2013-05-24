#!/usr/bin/python

import sys
import os
import urllib2
import shutil

def RemoveDir(path):
  print "Remove dir:", path
  if os.path.isdir(path):
    shutil.rmtree(path)

def ShellExecute(cmd):
  print "Executing:", cmd
  result = os.system(cmd)
  if result != 0:
    print "Execution exit code:", result
    sys.exit(result)
  print ""
  return result == 0

def AptGetInstall(libname):
  return ShellExecute('apt-get -y install ' + libname)
def AptGetInstallAll(libnames):
  return all(map(AptGetInstall, libnames))

def LibraryExists(libname, search_filename, search_dirs):
  print "Looking for library:", libname
  found = False
  for d in search_dirs:
    f = os.path.join(d, search_filename)
    found = os.path.exists(f)
    print "Check Existence:", f, "=>", (found and "FOUND" or "NOT-FOUND")
    if found: break
  print ""
  return found

# e.g. "abc.tar.gz" => "abc"
#      "abc.tgz" => "abc"
#      "abc.zip" => "abc"
def ArchiveLibName(arch_name):
  if arch_name.endswith('.tar.gz'): return arch_name[:-7]
  if arch_name.endswith('.zip'):    return arch_name[:-4]
  if arch_name.endswith('.tgz'):    return arch_name[:-4]
  print "Unrecognized archive extension: [" + arch_name + "]"
  return ""

# download url, save to filename
def Download(url, filename):
  try:
    f = urllib2.urlopen(url) 
  except urllib2.URLError as e:
    print "Cannot retrieve URL:", url, " , exception:", e
    return False

  if f.getcode() != 200:
    print "Cannot retrieve URL:", url, " , HTTP result code:", g.getcode()
    return False

  with open(filename, "wb") as output:
    while True:
      data = f.read(1000000) # read 1MB chunks
      if data == "": break # EOF
      output.write(data)
  print "Saved to:", filename
  return True

# download library, unzip, install under name <libname>
# You can remove it with: apt-get remove <libname>
def DownloadAndInstall(libname, http_filename_targz):
  print "DownloadAndInstall:", http_filename_targz
  arch_basename = os.path.basename(http_filename_targz)
  arch_name = os.path.join("/tmp", arch_basename)
  if not Download(http_filename_targz, arch_name): return False
  print ""

  libdir = ArchiveLibName(arch_name)
  if libdir == "": return False
  RemoveDir(libdir)
  print "Unzipping:", arch_name, " into:", libdir
  ShellExecute('tar -xf ' + arch_name + ' -C /tmp')
  print ""

  cwd = os.getcwd()
  os.chdir(libdir)
  ShellExecute('./configure  --prefix=/usr')
  ShellExecute('make')
  ShellExecute('checkinstall --pkgname=' + libname + ' -y')
  os.chdir(cwd)
  print "Library: '"+ libname + "' successfully installed"
  print ""

  return True

#########################################

def InstallGFlags():
  libname = 'gflags'
  if LibraryExists(libname, 'gflags/gflags.h', ['/opt/local/include', '/usr/local/include', '/usr/include']):
    return True
  return DownloadAndInstall(libname, 'http://gflags.googlecode.com/files/gflags-2.0.tar.gz')

def InstallGLog():
  libname = 'glog'
  if LibraryExists(libname, 'glog/logging.h', ['/opt/local/include', '/usr/local/include', '/usr/include']):
    return True
  return DownloadAndInstall(libname, 'http://google-glog.googlecode.com/files/glog-0.3.3.tar.gz')

def main():
  if os.geteuid() != 0:
    print "Script not started as root."
    return 1
  if not AptGetInstallAll(['build-essential', 'checkinstall', 'cmake',
    'libglib2.0-dev', 'libicu-dev', 'zlib1g-dev', 'libssl-dev', 'libaio-dev',
    'flex', 'libfaad-dev', 'libjpeg8-dev', 'libx264-dev', 'libmysql++-dev',
    'libavcodec-dev', 'libavformat-dev', 'libavutil-dev',
    'libswscale-dev']): return 1
  if not InstallGFlags() or not InstallGLog(): return 1
  print "Done"
  return 0

if __name__ == "__main__":
  sys.exit(main())

