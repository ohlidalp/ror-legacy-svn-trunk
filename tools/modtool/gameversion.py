#!/bin/env python
import sys, os.path, pickle, fnmatch

def ignoreDir(path):
  dirpair = os.path.split(path)
  if dirpair[1] == '':
    return False
  for ignores in ['.svn', 'noalpha', 'alphasharp', 'alphasmooth', 'copy']:
    if fnmatch.fnmatch(dirpair[1], ignores):
      #print "ignored ", dirpair[1], ignores
      return True
  return ignoreDir(dirpair[0])

def createGameVersioninfo(maindir, dstfn):
  materialfiles = []
  materials = []
  allfiles = []

  for root, dirs, files in os.walk(maindir):
    if not ignoreDir(root):
      for name in files:
        rootstr = root
        if rootstr.startswith(maindir):
          rootstr = rootstr[len(maindir):]
        fn = os.path.join(rootstr, name)
        basename, filename = os.path.split(name)
        rootfn, ext = os.path.splitext(filename)
        if ext == '.material':
          materialfiles.append(fn)
        allfiles.append(fn)

  for matfile in materialfiles:
    fn = maindir + matfile
    f = open(fn, "r")
    lines = f.readlines()
    f.close()

    for line in lines:
      if line.strip().lower().startswith('material '):
        materials.append(line.strip()[9:])

  print "%d files found and %d materials" % (len(allfiles), len(materials))

  data = {}
  data['files'] = allfiles
  data['materials'] = materials
  output = open(dstfn, 'wb')
  pickle.dump(data, output, -1)
  output.close()

def main():
  if len(sys.argv) < 3:
    print "usage: %s <directory> <target file>" % sys.argv[0]
    return

  maindir = sys.argv[1]
  dstfn = sys.argv[2]

  createGameVersioninfo(maindir, dstfn)

if __name__ == '__main__':
  main()