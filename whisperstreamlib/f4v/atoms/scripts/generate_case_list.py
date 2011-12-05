#!/usr/bin/python

from atoms import atoms

print "atoms: ", atoms

def GenerateEnum():
  print "Enumeration for base_atom.h:"
  for a in atoms:
    atomname = a.lower()
    AtomName=a[0:1].upper()+a[1:].lower()
    ATOMNAME=a.upper()
    atomchars = list(atomname.rjust(4))
    print "ATOM_" + ATOMNAME + " = MAKE_ATOM_TYPE('" + ("', '".join(atomchars)) + "'),"

def GenerateConsider():
  print "Consider for base_atom.cc:"
  for a in atoms:
    atomname = a.lower()
    AtomName=a[0:1].upper()+a[1:].lower()
    ATOMNAME=a.upper()
    atomchars = list(atomname.rjust(4))
    print "CONSIDER_STR(ATOM_" + ATOMNAME + ");"

def GenerateIncludes():
  print "To include, f4v_decoder.cc:"
  for a in atoms:
    atomname = a.lower()
    AtomName=a[0:1].upper()+a[1:].lower()
    ATOMNAME=a.upper()
    print "#include \"f4v/atoms/movie/" + atomname + "_atom.h\""

def GenerateCases():
  print "Cases for switch, f4v_decoder.cc:"
  for a in atoms:
    atomname = a.lower()
    AtomName=a[0:1].upper()+a[1:].lower()
    ATOMNAME=a.upper()
    print "case ATOM_" + ATOMNAME + ": atom = new " + AtomName + "Atom(); break;"

def GenerateBuild():
  print "To build, CMakeLists.txt:"
  for a in atoms:
    atomname = a.lower()
    AtomName=a[0:1].upper()+a[1:].lower()
    ATOMNAME=a.upper()
    print "f4v/atoms/movie/" + atomname + "_atom.cc"


def main():
  GenerateEnum()
  GenerateConsider()
  GenerateIncludes()
  GenerateCases()
  GenerateBuild()

if __name__ == "__main__":
  main()

