#!/usr/bin/python

# import sets

# hand modified atoms:

modified_atoms = [
  "FTYP",
  "MOOV",
  "MVHD",
  "FREE",
  "TRAK",
  "TKHD",
  "MDIA",
  "MINF",
  "HDLR",
  "MDHD",
  "VMHD",
  "SMHD",
  "STBL",

  "CO64",
  "CTTS",
  "STCO",
  "STSC",
  "STSD",
  "STSS",
  "STSZ",
  "STTS",

  "MDAT",

  "AVC1",
  "AVCC",
  "MP4A",
  "ESDS",
]

atoms = [
  # section1: atoms defined by Adobe and ISO
  "FTYP",
  "MOOV",
  "MVHD",
  "TRAK",
  "UDTA",
  "MDIA",
  "MINF",
  "STBL",
  "TKHD",
  "MDHD",
  "STSD",
  "STSC",
  "STTS",
  "CTTS",
  "STCO",
  "CO64",
  "STSS",
  "STSZ",
  "CHPL",
  "PDIN",
  "MDAT",
  # section2: atoms defined by Apple and ISO, but not by Adobe
  "HDLR",
  "EDTS",
  "ELTS",
  "VMHD",
  "SMHD",
  "DINF",
  "SDTP",
  "FREE",
  # section3: old atoms defined only by Apple
  "LOAD",
  # section3: the "meta" atom is defined by Adobe. (ISO has a different version of 'meta', bearing the same name)
  "META",
  # section4: metadata atoms defined only by Adobe
  "ILST",
  "STYL",
  "HLIT",
  "HCLR",
  "KROK",
  "DLAY",
  "DRPO",
  "DRPT",
  "HREF",
  "TBOX",
  "BLNK",
  "TWRP",
  "UUID",
  # section5: codec specific atoms
  "AVCC", #for h264
  "AVC1", #for h264
  "ESDS", #for aac
  "MP4A"] 

#sort
atoms.sort()

#check for duplicates
dups = filter(lambda x: atoms.count(x) > 1, atoms)
if len(dups) > 0:
  raise Exception("Duplicate atom: %s" % dups)

# remove duplicates
#atoms=list(sets.Set(atoms))

autogen_atoms = filter(lambda x: x not in modified_atoms, atoms)
if __name__ == "__main__":
  print "atoms: ", atoms
