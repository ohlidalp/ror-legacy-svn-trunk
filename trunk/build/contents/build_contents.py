#!/bin/env python
# thomas fischer 08/16/08
import sys, os, os.path, platform, subprocess, zipfile, glob, shutil, time, fnmatch

# this sets the architecture thats used to find tool binaries
ARCH = os.uname()[-1]

if platform.system() == 'Windows':
    NVDXT_EXECUTABLE = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', 'tools', 'dxt', 'nvdxt')
else:
    NVDXT_EXECUTABLE = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', 'tools', 'dxt', 'nvcompress_'+ARCH)
    if not os.path.isfile(NVDXT_EXECUTABLE):
        print "tool exetubale file not found: %s" % NVDXT_EXECUTABLE
        print "please download and compile this: http://code.google.com/p/nvidia-texture-tools/"
        print "and put the nvcompress into the deirectory %s with the name %s." % (os.path.join(os.path.dirname(os.path.abspath(__file__)), 'tools', 'dxt'), 'nvcompress_'+ARCH)
        sys.exit(-1)

RELEASEDIR = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'release')
SOURCEDIR = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'source')

if sys.platform.startswith('linux'):
  NVDXT_SETTINGS={'noalpha':'-bc1', 'alphasharp':'-bc2', 'alphasmooth':'-bc3'}
else:
  NVDXT_SETTINGS={'noalpha':'-dxt1c', 'alphasharp':'-dxt3', 'alphasmooth':'-dxt5'}

def copyIfNewer(srcfile, dstfile):
  if os.path.isdir(dstfile):
    # second argument is a directory, so find the destination filename
    dstfile = os.path.join(dstfile, os.path.basename(srcfile))
  if os.path.isfile(dstfile) and os.stat(srcfile)[8] < os.stat(dstfile)[8]: # mod time = 8, creation time = 9
      # do not copy if the file is not newer
      return
  shutil.copy(srcfile, dstfile)

def ignoreDir(path):
  dirpair = os.path.split(path)
  if dirpair[1] == '':
    return False
  for ignores in ['.svn', 'noalpha', 'alphasharp', 'alphasmooth', 'copy']:
    if fnmatch.fnmatch(dirpair[1], ignores):
      #print "ignored ", dirpair[1], ignores
      return True
  return ignoreDir(dirpair[0])

def packDirectory(dname):
  print "=== processing '%s' ..." % dname
  srcdir = os.path.join(SOURCEDIR, dname)
  dfile = "%s.zip" % os.path.join(RELEASEDIR, dname)
  ts = time.time()
  counter = 0

  # remove old file if existing
  if os.path.isfile(dfile):
    os.remove(dfile)

  # texture handling
  tempdir = os.path.join(srcdir, 'temp')
  # texture directories
  fc = 0
  for cdir in ['noalpha', 'alphasharp', 'alphasmooth']:
    tdir = os.path.join(srcdir, cdir)
    if os.path.isdir(tdir):
      files = glob.glob(os.path.join(tdir, '*'))
      for fn in files:
        fc += 1
        #print cdir, os.path.basename(fn)
        filename = os.path.basename(fn)
        (shortname, extension) = os.path.splitext(filename)
        dstfile = os.path.join(tempdir, shortname+'.dds')
        if not os.path.isdir(tempdir):
          os.mkdir(tempdir)
        # workaround for linux, the nvcompress uses different command line arguments!
        if sys.platform.startswith('linux'):
          cmd = [NVDXT_EXECUTABLE, NVDXT_SETTINGS[cdir], '-fast', fn, dstfile]
        else:
          cmd = [NVDXT_EXECUTABLE, '-file', fn, '-outdir', tempdir, '-quality_highest', '-rescale', 'lo', '-timestamp', NVDXT_SETTINGS[cdir]]
        subprocess.call(cmd, shell=False, bufsize=4096, stdout=subprocess.PIPE)

  # 'copy' directory: just copy the files in there, do NOT convert!
  files = glob.glob(os.path.join(srcdir, 'copy', '*'))
  for fn in files:
    copyIfNewer(fn, tempdir)

  # getting file list
  zfiles = []
  for root, dirs, files in os.walk(srcdir, topdown=False):
    if not ignoreDir(root):
      for fname in files:
        zfiles.append(os.path.join(root, fname))

  if fc > 0:
    print " found and processed %d images." % fc
  # creating zip file
  zip = zipfile.ZipFile(dfile, 'w', zipfile.ZIP_STORED)
  for fn in zfiles:
    prefix = os.path.commonprefix([srcdir, fn])
    fn2 = fn[len(prefix):]
    if fn2.startswith('/temp/'): # put files from temp directory into the root of the zip
      fn2 = fn2[len('/temp/'):]
    if fn2.startswith('\\temp\\'): # put files from temp directory into the root of the zip
      fn2 = fn2[len('\\temp\\'):]
    zip.write(fn, fn2)
    counter += 1
  zip.close()

  #if textureHandling:
  #	for fn in glob.glob(os.path.join(SOURCEDIR, dname, 'temp', '*')):
  #		os.remove(fn)

  print "=== done processing '%s' : %d files in %0.2f seconds." % (dname, counter,  time.time()-ts)
  return counter

def main():
  ts = time.time()
  counter = 0
  entries = os.listdir(SOURCEDIR)
  for entry in entries:
    if os.path.isdir(os.path.join(SOURCEDIR, entry)) and entry != '.svn':
      #print entry
      counter += packDirectory(entry)
  print "done with all. processed %d files in %0.2f seconds." % (counter,  time.time()-ts)
  wait=True
  if len(sys.argv) > 1 and sys.argv[1] == 'nowait':
    wait=False
  if wait:
    raw_input("Press enter to continue.")

if __name__ == '__main__':
  main()
