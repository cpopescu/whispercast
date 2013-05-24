#!/usr/bin/python
#
# Copyright (c) 2009, Whispersoft s.r.l.
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
#
# * Redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer.
# * Redistributions in binary form must reproduce the above
# copyright notice, this list of conditions and the following disclaimer
# in the documentation and/or other materials provided with the
# distribution.
# * Neither the name of Whispersoft s.r.l. nor the names of its
# contributors may be used to endorse or promote products derived from
# this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
# Author: Cosmin Tudorache
#
import sys
import os
import filecmp
from string import Template
import datetime
import shutil
import atoms

template_h = Template("""
//
// WARNING !! This is Auto-Generated code by generate_atoms.py !!!
//            do not modify this code
//
#ifndef __MEDIA_F4V_ATOMS_MOVIE_${ATOM}_ATOM_H__
#define __MEDIA_F4V_ATOMS_MOVIE_${ATOM}_ATOM_H__

#include <string>
#include <whisperlib/common/io/buffer/memory_stream.h>
#include <whisperstreamlib/f4v/atoms/base_atom.h>

namespace streaming {
namespace f4v {

class ${Atom}Atom : public BaseAtom {
 public:
  static const AtomType kType = ATOM_${ATOM};
 public:
  ${Atom}Atom();
  ${Atom}Atom(const ${Atom}Atom& other);
  virtual ~${Atom}Atom();

  ///////////////////////////////////////////////////////////////////////////

  // Methods from BaseAtom
  virtual bool EqualsBody(const BaseAtom& other) const;
  virtual void GetSubatoms(vector<const BaseAtom*>& subatoms) const;
  virtual BaseAtom* Clone() const;
  virtual TagDecodeStatus DecodeBody(uint64 size,
                                     io::MemoryStream& in,
                                     Decoder& decoder);
  virtual void EncodeBody(io::MemoryStream& out, Encoder& encoder) const;
  virtual uint64 MeasureBodySize() const;
  virtual string ToStringBody(uint32 indent) const;

 private:
  io::MemoryStream raw_data_;
};
}
}

#endif // __MEDIA_F4V_ATOMS_MOVIE_${ATOM}_ATOM_H__
""")

template_cc = Template("""
//
// WARNING !! This is Auto-Generated code by generate_atoms.py !!!
//            do not modify this code
//
#include <whisperstreamlib/f4v/atoms/movie/auto/${atom}_atom.h>

namespace streaming {
namespace f4v {

${Atom}Atom::${Atom}Atom()
  : BaseAtom(kType),
    raw_data_() {
}
${Atom}Atom::${Atom}Atom(const ${Atom}Atom& other)
  : BaseAtom(other),
    raw_data_() {
  raw_data_.AppendStreamNonDestructive(&other.raw_data_);
}
${Atom}Atom::~${Atom}Atom() {
}

bool ${Atom}Atom::EqualsBody(const BaseAtom& other) const {
  const ${Atom}Atom& a = static_cast<const ${Atom}Atom&>(other);
  return raw_data_.Equals(a.raw_data_);
}
void ${Atom}Atom::GetSubatoms(vector<const BaseAtom*>& subatoms) const {
}
BaseAtom* ${Atom}Atom::Clone() const {
  return new ${Atom}Atom(*this);
}
TagDecodeStatus ${Atom}Atom::DecodeBody(uint64 size,
                                        io::MemoryStream& in,
                                        Decoder& decoder) {
  if ( in.Size() < size ) {
    DATOMLOG << "Not enough data in stream: " << in.Size()
             << " is less than expected: " << size;
    return TAG_DECODE_NO_DATA;
  }
  raw_data_.AppendStream(&in, size);
  return TAG_DECODE_SUCCESS;
}
void ${Atom}Atom::EncodeBody(io::MemoryStream& out, Encoder& encoder) const {
  out.AppendStreamNonDestructive(&raw_data_);
}
uint64 ${Atom}Atom::MeasureBodySize() const {
  return raw_data_.Size(); 
}
string ${Atom}Atom::ToStringBody(uint32 indent) const {
  ostringstream oss;
  oss << "raw_data_: " << raw_data_.Size() << " bytes";
  return oss.str();
}
}
}
""")

cache = "cache"
backup = "backup"

# [IN] path: string, directory to list
# [OUT] out_dirs: list, receives the list of directories
# [OUT] out_files: list, receives the list of files
def ListDir(path, out_dirs, out_files):
  items = os.listdir(path);
  for i in items:
    f = os.path.join(path,i)
    if os.path.isfile(f):
      out_files.append(f)
      continue
    if os.path.isdir(f):
      out_dirs.append(f)
      continue
  out_dirs.sort()
  out_files.sort()

def Cache(filename):
  return os.path.join(os.path.dirname(filename),
                      cache, os.path.basename(filename))

def CacheDir(output_dir):
  return os.path.join(output_dir, cache)

def Backup(filename):
  return os.path.join(os.path.dirname(filename),
                      backup, os.path.basename(filename))

def BackupDir(output_dir):
  return os.path.join(output_dir, backup)


def IsModified(filename):
  if not os.path.exists(filename):
    return False
  cache_file = Cache(filename)
  if not os.path.exists(cache_file):
    print "Cache file: [%s] not found" % cache_file
    return True
  if not filecmp.cmp(filename, cache_file):
    print "File [%s] has been modified" % filename
    return True
  return False

def Mkdir(dirname):
  if not os.path.isdir(dirname):
    print "Creating directory: %s" % dirname
    os.mkdir(dirname)
  else:
    print "Directory: [%s] already exists" % dirname

def BackupFile(filename):
  if not os.path.exists(filename):
    return
  today = datetime.datetime.today()
  backfile = Backup(filename) + "." + today.strftime("%Y-%m-%d_%H-%M-%S")
  shutil.copyfile(filename, backfile)
  print "Backup file: %s" % backfile

def DeleteOldBackup(output_dir, keep_last_n_versions):
  baks = []
  bakdirs = []
  ListDir(BackupDir(output_dir), bakdirs, baks)

  baks.sort()
  last_file_base = ''
  same_file = 0
  removed_count = 0

  for b in baks:
    bbase = os.path.basename(b)
    orig = os.path.splitext(bbase)[0]
    if ( last_file_base != orig ):
      last_file_base = orig
      same_file = 1
      continue
    same_file += 1
    if ( same_file > keep_last_n_versions ):
      removed_count += 1
      os.unlink(b)
  print "DeleteOldBackup removed %s files" % removed_count

def CacheFile(filename):
  cachefile = Cache(filename)
  shutil.copyfile(filename, cachefile)
  print "Cache file: %s" % cachefile

def GenerateF4vAtoms(output_dir, all_atoms, autogen):
  print "Generating atoms in dir: %s" % output_dir
  if not autogen:
    Mkdir(CacheDir(output_dir))
    Mkdir(BackupDir(output_dir))
  src = atoms.autogen_atoms
  if all_atoms:
    src = atoms.atoms
  for a in src:
    print "Checking atom: %s" % a
    atomname = a.lower()
    AtomName = a[0].upper() + a[1:].lower()
    ATOMNAME = a.upper()
    file_dict = { 'atom' : atomname,
                  'Atom' : AtomName,
                  'ATOM' : ATOMNAME
                  }
    data_h = template_h.substitute(file_dict)
    data_cc = template_cc.substitute(file_dict)

    filename_h = os.path.join(output_dir, a.lower() + "_atom.h")
    filename_cc = os.path.join(output_dir, a.lower() + "_atom.cc")

    if autogen:
      open(filename_h, "w").write(data_h)
      open(filename_cc, "w").write(data_cc)
    else:
      if not os.path.exists(Cache(filename_h)):
        open(Cache(filename_h), "w").write(data_h)
      if not os.path.exists(Cache(filename_cc)):
        open(Cache(filename_cc), "w").write(data_cc)

      if IsModified(filename_h) or IsModified(filename_cc):
         print "Skip atom %s because of existing modified file" % a
         continue

      BackupFile(filename_h)
      open(filename_h, "w").write(data_h)
      CacheFile(filename_h)
      BackupFile(filename_cc)
      open(filename_cc, "w").write(data_cc)
      CacheFile(filename_cc)
    print "Generated files for atom: %s" % a
  if not autogen:
    DeleteOldBackup(output_dir, 10)

def main():
  all_atoms = False
  autogen = False
  if len(sys.argv) > 1:
    output_dir = sys.argv[1]
    if len(sys.argv) > 2 and sys.argv[2] == 'all_atoms':
      all_atoms = True
    if len(sys.argv) > 2 and sys.argv[2] == 'autogen':
      autogen = True
  else:
    print "Usage %s <output_dir>" % sys.argv[0]
    return
  if not os.path.exists(output_dir):
    os.mkdir(output_dir)
  GenerateF4vAtoms(output_dir, all_atoms, autogen)

if __name__ == "__main__":
  main()
