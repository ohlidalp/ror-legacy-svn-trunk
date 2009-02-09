#!/bin/env python
# thomas fischer 08/16/08
import sys, os, os.path, platform, subprocess, zipfile, glob, shutil, time, platform

if platform.system() == 'Windows':
    NVDXT_EXECUTABLE = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'tools', 'dxt', 'nvdxt')
else:
    NVDXT_EXECUTABLE = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'tools', 'dxt', 'nvcompress')

RELEASEDIR = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'release')
SOURCEDIR = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'source')
TEMPDIR = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'temp')

sys.path.append(os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', "tools", "modtool"))
from rortester import *
from uniquifier import *
from gameversion import *



FOREGROUND_BLUE = 0x01 # text color contains blue.
FOREGROUND_GREEN= 0x02 # text color contains green.
FOREGROUND_RED  = 0x04 # text color contains red.
FOREGROUND_INTENSITY = 0x08 # text color is intensified.
BACKGROUND_BLUE = 0x10 # background color contains blue.
BACKGROUND_GREEN= 0x20 # background color contains green.
BACKGROUND_RED  = 0x40 # background color contains red.
BACKGROUND_INTENSITY = 0x80 # background color is intensified.
if platform.system() == 'Windows':
    # See http://msdn.microsoft.com/library/default.asp?url=/library/en-us/winprog/winprog/windows_api_reference.asp
    # for information on Windows APIs.
    STD_INPUT_HANDLE = -10
    STD_OUTPUT_HANDLE= -11
    STD_ERROR_HANDLE = -12
    import ctypes
    std_out_handle = ctypes.windll.kernel32.GetStdHandle(STD_OUTPUT_HANDLE)

def set_color(color):
    """(color) -> BOOL
    
    Example: set_color(FOREGROUND_GREEN | FOREGROUND_INTENSITY)
    """
    if platform.system() == 'Windows':
        return ctypes.windll.kernel32.SetConsoleTextAttribute(std_out_handle, color)
    return False

def reset_color():
    set_color(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE)
    
def clearTempDir(path=TEMPDIR):
    for fn in glob.glob(os.path.join(path, '*')):
        os.remove(fn)

def centerPrint(msg, mode=0):
    if mode == 0:
        set_color(FOREGROUND_BLUE | FOREGROUND_GREEN)
    elif mode == 1:
        set_color(FOREGROUND_GREEN)
    sys.stdout.write('\n'+("  %s  "%msg.upper()).center(80,'-')+'\n\n')
    reset_color()

def main():
    tddeffilename='release-temp.ddef'
    centerPrint('initialization')
    print "creating dependency file ..."
    createGameVersioninfo(os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'build', 'contents', 'source'), tddeffilename)
    ts = time.time()
    counter = 0
    
    for streamdir_name in os.listdir(SOURCEDIR):
        streamdir=os.path.join(SOURCEDIR, streamdir_name)
        if not os.path.isdir(streamdir) or os.path.basename(streamdir) in ['.svn', 'release']:
            continue

        stream_info = os.path.join(RELEASEDIR, streamdir_name, 'stream.info')
        # clear stream info file
        streamversion=0
        if os.path.isfile(stream_info):
            f = open(stream_info, "r")
            for line in f.readlines():
                if line.strip().startswith('version='):
                    streamversion = int(line.strip()[8:])
                    streamversion += 1
                    break
            f.close()
        centerPrint('processing stream \'%s\', version %d'% (streamdir_name, streamversion), 1)
        for dir_name in os.listdir(streamdir):
            stream_type = dir_name
            dir=os.path.join(streamdir, dir_name)

            stream_releasedir = os.path.join(RELEASEDIR, streamdir_name, stream_type)
            try:
                os.makedirs(stream_releasedir)
            except:
                pass
            
            if not os.path.isdir(dir) or os.path.basename(dir) in ['.svn', 'release']:
                continue
                

            f = open(stream_info, "w")
            f.write("""stream_info
{
    generationtime=%(time)d
    name=%(name)s
    version=%(version)d
    streamtype=%(type)d
}

""" % {
    'time':time.time(),
    'version':streamversion,
    'type':0,
    'name':streamdir_name,
        })
            f.close()
                
            centerPrint(os.path.basename(dir))
            entries = os.listdir(dir)
            
            # create options
            uoptions = UTaskOptions()
            uoptions.ddeffile = tddeffilename
            uoptions.generatetruckimage = False
            uoptions.checktr_intoResult = False
            uoptions.failOnErrors = True
            uoptions.printErrors = True
            uoptions.offlineMode = False
            uoptions.verbosity = 3
            uoptions.opmesh = False
            uoptions.checktr_writeCorrected = False # this will update the truck file with the fixed version
            uoptions.tools = {
                'Linux':{
                    'OgreXMLConverter'    : os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "tools", "linux", "OgreXMLConverter"),
                    'OgreMeshUpgrade'     : os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "tools", "linux", "OgreMeshUpgrade"),
                    'OgreMaterialUpgrade' : os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "tools", "linux", "OgreMaterialUpgrade"),
                    'MeshMagick'          : "echo ",
                    'convert'             : "/bin/nice -n 19 /usr/bin/convert",
                },
                'Windows':{
                    'OgreXMLConverter'    : os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "tools", "windows", "OgreXMLConverter.exe"),
                    'OgreMeshUpgrade'     : os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "tools", "windows", "OgreMeshUpgrade.exe"),
                    'OgreMaterialUpgrade' : os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "tools", "windows", "OgreMaterialUpgrade.exe"),
                    'MeshMagick'          : os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "tools", "windows", "MeshMagick.exe"),
                    'convert'             : os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "tools", "windows", "convert.exe"),
                    }
            }
            
            for entry in entries:
                # important:
                uoptions.uid = None
                # otherwise we use the same ID again!
                clearTempDir()
                path = os.path.join(dir, entry)
                tempdir = os.path.join(dir, entry, 'temp')
                if not os.path.isdir(tempdir) and not os.path.isfile(tempdir):
                    os.mkdir(tempdir)
                clearTempDir(tempdir)
                
                if os.path.isdir(path) and entry != '.svn':
                    set_color(FOREGROUND_BLUE | FOREGROUND_RED | FOREGROUND_INTENSITY)
                    sys.stdout.write(" %-14s " % entry)
                    reset_color()
                    sys.stdout.write("... ")
                    
                    sys.stdout.write("textures")
                    texture_dir = os.path.join(path, 'textures')
                    texture_out_dir = os.path.join(texture_dir, 'temp')
                    if not os.path.isdir(texture_dir):
                        os.mkdir(texture_dir)
                    if platform.system() == 'Windows':
                        settings={'noalpha':'-dxt1c', 'alphasharp':'-dxt3', 'alphasmooth':'-dxt5'}
                    else:
                        settings={'noalpha':'-bc1', 'alphasharp':'-bc2', 'alphasmooth':'-bc3'}
                    fcount=0
                    for cdir in ['noalpha', 'alphasharp', 'alphasmooth', 'copy']:
                        tdir = os.path.join(texture_dir, cdir)
                        if os.path.isdir(tdir):
                            files = glob.glob(os.path.join(tdir, '*'))
                            for fn in files:
                                wstr = " (%2d)"%fcount
                                estr = ''
                                if fcount > 0:
                                    estr = '\b'*len(wstr)
                                sys.stdout.write(estr + wstr)
                                fcount+=1
                                if cdir == 'copy':
                                    shutil.copy(fn, texture_out_dir)
                                    continue
                                if platform.system() == 'Windows':
                                    cmd = [NVDXT_EXECUTABLE, '-file', fn, '-outdir', texture_out_dir, '-quality_highest', '-rescale', 'lo', '-timestamp', settings[cdir]]
                                    subprocess.call(cmd, shell=False, bufsize=4096, stdout=subprocess.PIPE)
                                else:
                                    # working on linux
                                    #print "Converting %s ..." % (os.path.basename(fn))
                                    
                                    # fix broken stuff
                                    if os.path.isfile(texture_out_dir):
                                        os.unlink(texture_out_dir)
                                    if not os.path.isdir(texture_out_dir):
                                        os.mkdir(texture_out_dir)
                                        
                                    tfilename, text = os.path.splitext(os.path.basename(fn))
                                    ddsFilename=tfilename+".dds"
                                    srcf=os.path.join(os.path.dirname(fn), ddsFilename)
                                    dstf=os.path.join(texture_out_dir, ddsFilename)
                                    if not os.path.isfile(dstf):
                                        cmd = [NVDXT_EXECUTABLE, '-fast', settings[cdir], fn]
                                        subprocess.call(cmd, shell=False, bufsize=4096, stdout=subprocess.PIPE)
                                        if not os.path.isfile(srcf):
                                            print "error while converting texture '%s' to .dds" % (os.path.basename(fn))
                                        else:
                                            shutil.copy(srcf, texture_out_dir)
                    if fcount == 0:
                        sys.stdout.write(" ( 0)")
                    sys.stdout.write(", syntax")
                    #counter += checkDirectory(path)
                    
                    set_color(FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_INTENSITY | BACKGROUND_RED)
                    u = Uniquifier(path, tempdir, taskoptions=uoptions)
                    reset_color()
                    
                    outfile = os.path.join(tempdir, 'content.zip')
                    if os.path.isfile(outfile):
                        shutil.move(outfile, os.path.join(stream_releasedir, '%s.zip'%entry))
                        sys.stdout.write(" ... DONE%s%s " %(' '*20, '\b'*20))
                    
                    if len(u.testers) > 0:
                        for tester in u.testers:
                            # we will add the zip to the global stream info
                            f = open(stream_info, "a")
                            f.write("""stream_mod
{
    minitype=%(minitype)s
    type=Zip
    dirname=%(zippath)s
    fname=%(uid)s-%(fname)s
    fname_without_uid=%(fname_without_uid)s
    fext=%(fext)s
    dname=%(dname)s
    categoryid=%(categoryid)s
    uniqueid=%(uid)s
    version=%(version)s
}

""" % {
                                'minitype' : tester.minitype,
                                'zippath' : os.path.join(stream_type, '%s.zip' % entry).replace('\\', '/'),
                                'fname' : tester.fname,
                                'fname_without_uid' : tester.fname_without_uid,
                                'fext' : tester.fext,
                                'dname' : tester.dname,
                                'uid' : u.uid,
                                'categoryid' : tester.categoryid,
                                'version' : tester.version,
                            })
                            f.close()
                            
                            severity = {SEC_WARN:0,SEC_FATAL:0,SEC_PERF:0}
                            for err in tester.errors:
                                severity[err.severity] += 1
                            
                            
                        if severity[SEC_WARN] > 0 or severity[SEC_FATAL] > 0 or severity[SEC_PERF] > 0:
                            sys.stdout.write(" (")
                            if severity[SEC_FATAL] > 0:
                                estr = 'errors'
                                if severity[SEC_FATAL] == 1:
                                    estr = 'error'
                                set_color(FOREGROUND_RED | FOREGROUND_INTENSITY)
                                sys.stdout.write("%d fatal %s" % (severity[SEC_FATAL], estr))
                                reset_color()
                            if severity[SEC_FATAL] > 0 and severity[SEC_WARN] > 0:
                                sys.stdout.write(", ")
                            if severity[SEC_WARN] > 0:
                                estr = 'warnings'
                                if severity[SEC_WARN] == 1:
                                    estr = 'warning'
                                set_color(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY)
                                sys.stdout.write("%d %s" % (severity[SEC_WARN], estr))
                                reset_color()
                            sys.stdout.write(")")
                            
                        
                    sys.stdout.write("\n")
                    # we already print out an error, so dont do that twice
                    #else:
                    #	sys.stdout.write("FAILED, see log file for errors\n")
                    counter += 1
                    
    centerPrint('done')
    clearTempDir()
    print "done with all. processed %d directories in %0.2f seconds." % (counter,  time.time()-ts)
    wait=True
    if len(sys.argv) > 1 and sys.argv[1] == 'nowait':
      wait=False
    if wait:
      raw_input("Press enter to continue.")

if __name__ == '__main__':
    main()
