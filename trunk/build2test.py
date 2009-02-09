#!/bin/env python
# pricorde 6/12/2008
import sys, os, os.path, platform, shutil, time

SOURCE=os.path.join(os.path.dirname(os.path.abspath(__file__)), 'build')
TARGET=os.path.join(os.path.dirname(os.path.abspath(__file__)), 'test')
STREAMS=os.path.join(os.path.dirname(os.path.abspath(__file__)), 'streams', 'release')

class Error(EnvironmentError):
    pass

def copyAll(src, dst):
    #copies recursively all files in srcdir to dstdir
    #both dirs are supposed to exist
    #shutil.copytree(srcdir, dstdir)
    # print "copy from %s to %s" % (src, dst)
    # files beginning with . should not be copied (for .svn)
    # also some extensions should be filtered out

    names = os.listdir(src)
    if not os.path.isdir(dst):
        os.mkdir(dst)
    errors = []
    for name in names:
    	if (name.startswith('.')):
    	    continue
    	if (name.endswith('.pdb') or name.endswith('.exp') or name.endswith('.lib')):
    	    continue
        srcname = os.path.join(src, name)
        dstname = os.path.join(dst, name)
        try:
            if os.path.isdir(srcname):
                copyAll(srcname, dstname)
            else:
                shutil.copy2(srcname, dstname)
        except (IOError, os.error), why:
            errors.append((srcname, dstname, str(why)))
        # catch the Error from the recursive copytree so that we can
        # continue with other files
        except Error, err:
            errors.extend(err.args[0])
    try:
        shutil.copystat(src, dst)
    except WindowsError:
        # can't copy file access times on Windows
        pass
    except OSError, why:
        errors.extend((src, dst, str(why)))
    if errors:
        raise Error, errors
  
def deleteAll(dir):
  if os.path.isdir(dir):
    #print "delete %s" % (dir)
    shutil.rmtree(dir, True)
  
def mkdir(dir):
  if not os.path.isdir(dir):
    os.mkdir(dir)

def main():
  ts = time.time()
  
  #cleanup
  print "cleanup"
  deleteAll(TARGET)
  #creating base dirs
  print "copying"
  mkdir(TARGET)
  CURRENT=os.path.join(TARGET, 'current')
  mkdir(CURRENT)
  UPDATER=os.path.join(TARGET, 'updater')
  mkdir(UPDATER)
  print "copying programs"
  #copy program files
  if sys.platform.startswith('linux'):
  	PSOURCE=os.path.join(SOURCE, 'bin', 'release', 'linux')
  if sys.platform.startswith('win32'):
  	PSOURCE=os.path.join(SOURCE, 'bin', 'release', 'windows')
  if sys.platform.startswith('mac'):
  	PSOURCE=os.path.join(SOURCE, 'bin', 'release', 'macos')
  copyAll(PSOURCE, CURRENT)
  #copy configurator files
  print "copying configurator data"
  CFT=os.path.join(CURRENT, 'languages')
  mkdir(CFT)
  copyAll(os.path.join(SOURCE, 'languages'), CFT)
  #copy built-in game resources
  print "copying buit-in resources"
  REST=os.path.join(CURRENT, 'resources')
  mkdir(REST)
  copyAll(os.path.join(SOURCE, 'contents', 'release'), REST)
  #copy skeleton
  print "copying skeleton"
  SKT=os.path.join(CURRENT, 'skeleton')
  mkdir(SKT)
  copyAll(os.path.join(SOURCE, 'skeleton'), SKT)
  #copy streams
  print "copying streams"
  STT=os.path.join(CURRENT, 'streams')
  mkdir(STT)
  copyAll(STREAMS, STT)
  #updater will be copied later...
  
  print "done with all. processed in %0.2f seconds." % (time.time()-ts)
  wait=True
  if len(sys.argv) > 1 and sys.argv[1] == 'nowait':
    wait=False
  if wait:
    raw_input("Press enter to continue.")

if __name__ == '__main__':
  main()