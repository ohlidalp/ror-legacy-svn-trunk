import sys, os, os.path, socket, urllib, shutil, zipfile, pickle, subprocess, platform, re, time, math, random
from rortester import *
import draw

class UTaskOptions:
  uid=None
  ddeffile=None #dependency definition file
  upmesh=True #update meshs?
  opmesh=True #optimize meshs?
  upmat=True #update materials?
  meshinfo=False #print meshinfo?
  htmloutput=False #html output
  verbosity=0 #verbosity level
  checktr=True # check truck files?
  checktr_intoResult=True # should the check result be in the result zip?
  checktr_writeCorrected=True # write back corrected truick file?
  autouid=1
  ufilename='content.zip'
  lineend='win'
  generatetruckimage=1
  #generateidfile=False
  removeFiles=False
  tools=None
  offlineMode=False
  failOnErrors=False
  printErrors=False

  def __str__(self):
    return "uid: %s\nddeffile: %s\nupmesh: %s\nopmesh: %s\nupmat: %s\nmeshinfo: %s\nhtmloutput: %s\nverbosity: %s\ncheck truck: %s\nautouid mode: %s\n" % (self.uid, self.ddeffile, self.upmesh.__str__(), self.opmesh.__str__(), self.upmat.__str__(), self.meshinfo.__str__(), self.htmloutput.__str__(), self.verbosity.__str__(), self.checktr.__str__(), self.autouid.__str__())

class Uniquifier:
  LOGFILENAME = '-log'

  toolset = {
    'Linux':{
      'OgreXMLConverter'    :"/bin/nice -n 19 /usr/bin/OgreXMLConverter",
      'OgreMeshUpgrade'     :"/bin/nice -n 19 /usr/bin/OgreMeshUpgrade",
      'OgreMaterialUpgrade' :"/bin/nice -n 19 /usr/bin/OgreMaterialUpgrade",
      'MeshMagick'          :"/bin/nice -n 19 /usr/local/bin/meshmagick",
      'convert'             :"/bin/nice -n 19 /usr/bin/convert",
      },
    'Windows':{
      'OgreXMLConverter'    :"tools\\OgreXMLConverter.exe",
      'OgreMeshUpgrade'     :"tools\\OgreMeshUpgrade.exe",
      'OgreMaterialUpgrade' :"tools\\OgreMaterialUpgrade.exe",
      'MeshMagick'          :"tools\\MeshMagick.exe",
      'convert'             :"tools\\convert.exe",
      },
  }

  NEWLINE = {
    'win':   '\n',
    'linux': '\n',
    'mac':   '\r',
  }

  SECTIONS = ['description', 'end_description', 'end', 'globals', 'nodes', 'beams', 'cameras', 'cinecam', 'help', 'comment', 'end_comment', 'engine', 'engoption', 'brakes', 'wheels', 'wheels2', 'meshwheels', 'shocks', 'hydros', 'commands', 'commands2', 'rotators', 'contacters', 'ropes', 'fixes', 'minimass', 'ties', 'ropables', 'particles', 'rigidifiers', 'ropables2', 'flares', 'props', 'flexbodies', 'submesh', 'texcoords', 'cab', 'backmesh', 'exhausts', 'guisettings', 'wings', 'turboprops', 'airbrakes', 'fusedrag', 'turboprops2', 'pistonprops', 'turbojets', 'screwprops', 'globeams', 'envmap', 'managedmaterials','section', 'end_section']
  COMMANDS = ['fileformatversion', 'author', 'fileinfo', 'forwardcommands', 'importcommands', 'set_beam_defaults', 'rollon', 'hookgroup', 'set_skeleton_settings','sectionconfig']

  ddef = None
  ddeffile = None
  outstream = None
  outfilestream = None
  missingfiles = []
  uid = None
  src = None
  dst = None
  dbcon = None

  
  def __init__(self, src, dst, wfile=None, taskoptions=None):
    if taskoptions is None:
      # create a default one
      taskoptions = UTaskOptions()
    if not taskoptions.tools is None:
      self.toolset=taskoptions.tools
    self.tester = None
    self.offlinemode = taskoptions.offlineMode
    self.testers = []
    self.outstream = wfile
    self.src = src
    self.dst = dst
    self.missingfiles=[]
    self.uid = taskoptions.uid
    self.options = taskoptions
    #init database
    self.useDB=False
    #try:
    #	import sqlite3
    #	#self.printout(" * using database")
    #except:
    #	self.useDB=False
    #	#self.printout(" ** NOT using database")
    #	#print " ** NOT using database"

    if self.useDB:
      self.printout(" * starting task number %d" %(int(self.queryDB('select count(id) from results').fetchone()[0])))

    if self.options.verbosity > 0:
      self.printout("<h3>Options</h3><pre style='border:1px solid green;padding-left:20px;'>%s</pre>" % self.options.__str__())

    self.ddeffile = taskoptions.ddeffile
    #print " * uniqueifying %s to %s" % (self.src, self.dst)

    if self.uid is None:
      self.uid = self.getUniqueID()
    self.options.uid = self.uid

    logfilename = ''
    if self.outfilestream is None:
      ext = ".txt"
      if self.options.htmloutput:
        ext = ".html"
      #logfilename = os.path.join(self.dst, self.uid + self.LOGFILENAME + ext)
      logfilename = os.path.join(self.dst, 'modtool' + self.LOGFILENAME + ext)
      self.outfilestream = open(logfilename, "w")

    msg = 'generating dependency file'
    sys.stdout.write("%s%s" % (msg, '\b'*len(msg)))

    self.loadDependencyDefinition()
    self.printout(" * using dependency definition file %s" % self.ddeffile)

    msg = 'searching files ...'
    sys.stdout.write("%s%s" % (msg, '\b'*len(msg)))
    truckfiles = self.getFiles(self.src, ['.boat', '.truck', '.car', '.trailer', '.airplane', '.load', '.fixed'])
    materialfiles = self.getFiles(self.src, ['.material'])

    luafiles = self.getFiles(self.src, ['.lua'])
    configfiles = self.getFiles(self.src, ['.cfg'])
    terrainfiles = self.getFiles(self.src, ['.terrn'])
    odeffiles = self.getFiles([self.src, os.path.join(self.src, 'objects')], ['.odef'])
    terrainheightmaps = self.getFiles(self.src, ['.raw'])

    images = self.getFiles([self.src, os.path.join(self.src, 'textures', 'temp')], ['.png', '.dds', '.jpg'])

    meshes = self.getFiles(self.src, ['.mesh'])
    airfoils = self.getFiles(self.src, ['.afl'])

    # reset dependency tracker variables
    self.materialmap = {}
    self.materialmap_prov = {}

    self.requestedODEFs = []
    self.requestedHeightMaps = []
    self.meshmap = {}
    self.aflmap = {}
    self.imagemap = {}


    terrain_mode = False
    if len(terrainfiles) > 0:
      terrain_mode = True
      print "                                       \n### TERRAIN MODE, NO DEPENDENCY CHECKING! ###\n"
      #
      for filen in os.listdir(self.src):
        srcfile = os.path.join(self.src, filen)
        dstfile = os.path.join(self.dst, filen)
        if os.path.isfile(srcfile):
          #print filen
          shutil.copy(srcfile, dstfile)

      # copy textures
      for filen in os.listdir(os.path.join(self.src, 'textures', 'temp')):
        srcfile = os.path.join(os.path.join(self.src, 'textures', 'temp'), filen)
        dstfile = os.path.join(self.dst, filen)
        if os.path.isfile(srcfile):
          #print filen
          shutil.copy(srcfile, dstfile)
          
      # copy objects
      if os.path.isdir(os.path.join(self.src, 'objects')):
          for filen in os.listdir(os.path.join(self.src, 'objects')):
            srcfile = os.path.join(os.path.join(self.src, 'objects'), filen)
            dstfile = os.path.join(self.dst, filen)
            if os.path.isfile(srcfile):
              #print filen
              shutil.copy(srcfile, dstfile)
          
    msg = 'processing files ...'
    sys.stdout.write("%s%s" % (msg, '\b'*len(msg)))
    # trucks
    for file in truckfiles:
      self.processTruckFile(file)

    # meshes
    for file in self.meshmap.keys():
      self.processMeshFile(file)

    # configs
    for file in configfiles:
      self.processConfigFile(file)

    # terrains
    for file in terrainfiles:
      self.processTerrainFile(file)

    #odefs
    for file in odeffiles:
      self.processODEFFile(file)
    # todo: handle :
    '''
    requestedHeightMaps
    requestedODEFs
    '''
    materialmap_req = copy.copy(self.materialmap)
    self.materialmap = {}

    for file in materialfiles:
      self.processMaterialFile(file)

    #print self.materialmap_prov
    # copy all materials into our existing array
    tmp = copy.copy(self.materialmap)
    for key in tmp.keys():
      self.materialmap_prov[key] = tmp[key]

    #print "required materials:", materialmap_req
    #print "provided materials:", self.materialmap_prov

    materials_missing = {}
    for item in materialmap_req.keys():
      if not item in self.materialmap_prov.keys():
        materials_missing[item] = materialmap_req[item]

    materials_unused = {}
    for item in self.materialmap_prov.keys():
      if not item in materialmap_req.keys():
        materials_unused[item] = self.materialmap_prov[item]

    #print "materials_unused: ", materials_unused
    #print "materials_missing: ", materials_missing

    if len(self.materialmap_prov) > 0:
      if self.options.htmloutput:
        self.printout("<h2>material changes</h2>")
      else:
        self.printout("=== material changes ===")
      for m in self.materialmap_prov.keys():
        if m != self.materialmap_prov[m]:
          self.printout(" %s => %s" % (m, self.materialmap_prov[m]))

    if len(materials_missing) > 0:
      if self.options.failOnErrors:
        self.printout("\n .\nERROR, missing materials:", True)
      if self.options.htmloutput:
        self.printout("<h2>missing materials</h2>")
      else:
        self.printout("=== missing materials ===")
      for k in materials_missing.keys():
        m = materials_missing[k]
        self.printout(" * %30s by %-30s" % (k, m[1]), True)
        #self.printout(" %s" % (m), True)
      if self.checkFailOnError():
        return

    if len(materials_unused) > 0:
      if self.options.htmloutput:
        self.printout("<h2>unused materials</h2>")
      else:
        self.printout("=== unused materials ===")
      for m in materials_unused:
          self.printout(" %s" % (m))

    if len(self.aflmap) > 0:
      if self.options.htmloutput:
        self.printout("<h2>airfoil filename changes</h2>")
      else:
        self.printout( "=== airfoil filename changes ===")
      for afl in self.aflmap.keys():
        afl = afl.strip()
        try:
          fn = os.path.join(self.src, afl)
          if not os.path.isfile(fn):
            # do not duplicate ...
            if self.filenameInRoRFiles(afl):
              continue
            #fn = getAlternateSource(afl)
            #if fn == '':
            #self.printout( " *** filename not found: %s" % afl)
            self.missingfiles.append(afl)
            continue
          shutil.copyfile(fn, os.path.join(self.dst, self.aflmap[afl]))
        except:
          self.printout(" *** file not found: %s" % afl)
          self.missingfiles.append(afl)
        self.printout(" %s => %s" % (afl, self.aflmap[afl]))

    if len(self.meshmap) > 0:
      if self.options.htmloutput:
        self.printout("<h2>mesh filename changes</h2>")
      else:
        self.printout("=== mesh filename changes ===")
      for mesh in self.meshmap.keys():
        if self.meshmap[mesh] != mesh:
          self.printout(" %s => %s" % (mesh, self.meshmap[mesh]))

    if len(self.imagemap) > 0:
      #print self.imagemap
      if self.options.htmloutput:
        self.printout("<h2>image filename changes</h2>")
      else:
        self.printout( "=== image filename changes ===")
      for img in self.imagemap.keys():
        img = img.strip()
        try:
          fn = os.path.join(self.src, img)
          if not os.path.isfile(fn):
            fn = os.path.join(self.src, 'textures', 'temp', img)
          #print fn
          if not os.path.isfile(fn):
            if self.filenameInRoRFiles(img):
              continue
            #fn = getAlternateSource(img)
            #if fn == '':
            #self.printout(" *** filename not found: %s" % img, True)
            if img.find("-mini.png") >=0 or img.find("-mini.dds") >=0:
                # ignore missing mini images!
                continue
            self.missingfiles.append((img, self.imagemap[img][1]))
            continue
          fndst = os.path.join(self.dst, self.imagemap[img][0])
          #print fndst
          shutil.copyfile(fn, fndst)
        except:
          #self.printout(" *** file not found: %s" % img)
          self.missingfiles.append((img, self.imagemap[img][1]))
        if self.imagemap[img][0] != img:
          self.printout(" %s => %s" % (img, self.imagemap[img]))


    if len(self.missingfiles) > 0:
      if self.options.failOnErrors:
        self.printout("\n .\nERROR, missing files:", True)
      if self.options.htmloutput:
        self.printout("<h2>missing files</h2>")
      else:
        self.printout("=== missing files ===")
      for f in self.missingfiles:
        self.printout(" * %30s by %-30s" % (f[0], f[1]), True)
      if self.checkFailOnError():
        return

    if self.options.htmloutput:
      self.printout("<h2>creating zip %s</h2>"% (self.options.ufilename))
    else:
      self.printout( "=== creating zip %s ===" % (self.options.ufilename))

    zipfilename = os.path.join(self.dst, self.options.ufilename)
    zip = zipfile.ZipFile(zipfilename, "w", zipfile.ZIP_DEFLATED)
    for file in os.listdir(self.dst):
      fn = os.path.join(self.dst, file)
      if not terrain_mode:
        if self.options.autouid == 1 and not file.startswith(self.uid):
          continue
      else:
        # quite ugly hack to remove paths and stuff that shouldnt be in there
        if not os.path.isfile(fn) or file == '.svn':
          continue
      if fn == zipfilename:
        continue
      self.printout("- "+file)
      zip.write(fn, file)

    if self.useDB:
      sql = 'insert into results values ("%s", "%s", "%d")' % (self.uid, zipfilename, time.time())
      self.queryDB(sql)
      self.closeDB()

    if self.options.removeFiles:
      # remove all files exxcept log and result zip
      for file in os.listdir(self.dst):
        try:
          f = os.path.join(self.dst, file)
          #print 'remove ', f
          if f in [zipfilename, logfilename]:
            continue
          os.remove(f)
        except Exception, e:
          print e

    # close log again
    try:
      self.outfilestream.close()
    except:
      pass

  def checkFailOnError(self):
    if self.options.failOnErrors:
      self.printout("fatal error caught, exiting the tool")
      return True
    return False

  def queryDB(self, sql):
    if not self.useDB:
      return None
    if self.dbcon is None:
      import sqlite3
      self.dbcon = sqlite3.connect("uid.db")
      self.dbcon.row_factory = sqlite3.Row
      if self.dbcon.execute("select count(*) from sqlite_master where name=?", ("results" ,)).fetchone()[0] == 0:
        self.queryDB('create table results (id, filename, INT timestamp)')
      #print self.queryDB('select * from results').fetchall()
    cur = self.dbcon.cursor()
    cur.execute(sql)
    self.dbcon.commit()
    return cur

  def closeDB(self):
    if not self.useDB:
      return None
    if not self.dbcon is None:
      self.dbcon.close()

  def loadDependencyDefinition(self):
    if self.ddeffile == 'nodep':
      self.printout(" * using no dependencies")
      return
    #print self.ddeffile
    if self.ddeffile is None or not os.path.isfile(self.ddeffile):
      self.printout("FATAL: error opening dependency definition file: %s" % self.ddeffile)
      return
    f = open(self.ddeffile, "rb")
    self.ddef = pickle.load(f)
    f.close()

  def getFiles(self, dirarg, extlist):
    list = []
    dirlist = []
    if type(dirarg) == type(''):
      dirlist.append(dirarg)
    elif type(dirarg) == type([]):
      dirlist = dirarg
    for dir in dirlist:
      if not os.path.isdir(dir):
        continue
      for file in os.listdir(dir):
        root, ext = os.path.splitext(file)
        if ext in extlist:
          list.append(file)
    return list


  def printout(self, msg, isError=False):
    msg = msg.strip()
    if self.options.htmloutput:
      if msg[0:4] == ' ***':
        msg = "<span style='color:#aa0000;text-weight:bold;'>" + msg + "</span>"
      elif msg[0:3] == ' **':
        msg = "<span style='color:#aaaa00'>" + msg + "</span>"
      elif msg[0:2] == ' *':
        msg = "<span style='color:#00aa00;'>" + msg + "</span>"
      msg+="<br/>";
    if not self.outfilestream is None:
      msg = msg.replace('\n', self.newLineChar())
      self.outfilestream.write(msg.__str__()+self.newLineChar())
      self.outfilestream.flush()
    if not self.outstream is None:
      self.outstream.write(msg.__str__()+self.newLineChar())
      self.outstream.flush()
    if self.options.printErrors and isError:
      print msg


  def materialInRoRMaterials(self, matname):
    if self.ddef is None:
      return False
    for mat in self.ddef['materials']:
      if mat == matname:
        if self.options.verbosity > 0:
          self.printout(" * Material '%s' found in RoR, not copying" % matname)
        return True
    return False

  def filenameInRoRFiles(self, filename):
    if self.ddef is None:
      return False
    for fn in self.ddef['files']:
      base, filename2 = os.path.split(fn)
      if filename2 == filename:
        if self.options.verbosity > 0:
          self.printout(" * File '%s' found in RoR, not copying" % filename)
        return True
    return False

  def getAlternateSource(self, filename):
    return ''
    #deprecated for now
    for fn in self.ddef['files']:
      base, filename2 = os.path.split(fn)
      if filename2 == filename:
        if self.options.verbosity > 0:
          self.printout("reusing file from RoR destribution: %s" % filename)
        return fn
    return ''

  def newLineChar(self):
    return self.NEWLINE[self.options.lineend]

  #@staticmethod
  def getUniqueID(self):
    if not self.offlinemode:
        try:
            socket.setdefaulttimeout(10)
            res = urllib.urlopen('http://repository.rigsofrods.com/uniqueid/?auto').read()
            self.offlinemode = False
            return res
        except:
            print "\nunable to get uniqueid from the internet, using offline temporary one. DO NOT distribute the generated files!\n"
            self.offlinemode = True
    
    if self.offlinemode:
        uid = ""
        for i in range(0,10):
            uid += "%x" % random.randint(0, 16)
        return "%sXXOFFLINEXXUID" % uid

  @staticmethod
  def stripUniqueID(name):
    ouid = name.find('UID-') # fix for long material names
    if ouid >= 0:
      return name[ouid+4:]
    else:
      return name

  @staticmethod
  def addUniqueID(uid, name):
    return uid+'-'+name

  def uniqueGenericName(self, name):
    if name.__str__().startswith('-'):
      name = name[1:]
    if self.options.autouid == 1:
      slash = name.find('/') # fix for long material names
      ouid = name.find('UID-') # fix for long material names
      if ouid >= 0:
        return self.uid + '-' + name[ouid+4:]

      if len(name) > 41 and name[40] == '-' and (slash == -1 or slash > 41):
        #remove old id
        return self.uid + '-' + name[41:]
      else:
        return self.uid + '-' + name
    elif self.options.autouid == 2:
      # only remove ID
      slash = name.find('/') # fix for long material names
      ouid = name.find('UID-') # fix for long material names
      if ouid >= 0:
        #remove old id
        return name[ouid+4:]

        # remove old, long UID
        p = re.compile('[01234456789abcdef]{40}-')
        name = p.sub('', name)

      if len(name) > 41 and name[40] == '-' and (slash == -1 or slash > 41):
        #remove old id
        return name[41:]
      else:
        return name
    else:
      return name

  def uniqueAFLName(self, name):
    return self.uniqueGenericName(name)

  def uniqueMeshName(self, name):
    return self.uniqueGenericName(name)

  def uniqueMaterialName(self, name):
    # get those away: /f2aeb1a70817f94bca50789bee73bc869aa456ca/
    if self.options.autouid in [1, 2]:
      #print "before: " + name
      p = re.compile('[01234456789abcdef]{40}')
      name = p.sub('', name)
      name = name.replace('//', '/')
      name = name.replace('--', '-')
      #print "after: " + name
    return self.uniqueGenericName(name)

  def uniqueMaterialFileName(self, name):
    return self.uniqueGenericName(name)

  def uniqueTruckFileName(self, name):
    return self.uniqueGenericName(name)

  #def uniqueIDFileName(self):
    #return self.uniqueGenericName('alias.txt')

  def uniqueMinipictureFileName(self, name):
    return self.uniqueGenericName(name)

  def uniqueTextureName(self, name):
    return self.uniqueGenericName(name)

  def parseRE(self, content, r):
    vals = []
    for line in content:
      m = re.match(r, line)
      if not m is None and len(m.groups()) > 0:
        valname = m.groups()[0]
        if not valname in vals:
          vals.append(valname)
    return vals

  """
  class myExecuteBuffer(file):
    def __init__(self, wfile):
      self.wfile = wfile
    def msg(self, ms):
      self.wfile.write(ms.__str__()+'\n')
      self.wfile.flush()
    def close(self):
      print "close()"
      pass
    def flush(self):
      print "flush()"
      pass
    #def fileno(self):
    #	return 1142435423
    def isatty(self):
      return False
    def next(self):
      print "next()"
      pass
    def read(self, size=0):
      print "read()"
      pass
    def readline(self, size=0):
      print "readline()"
      pass
    def readlines(self, size=0):
      print "readlines()"
      pass
    def xreadlines(self):
      print "xreadlines()"
      pass
    def seek(self, offset, whence=0):
      print "seek()"
      pass
    def tell(self):
      print "tell()"
      return 1
    def truncate(self, size=0):
      print "truncate()"
      pass
    def write(self, str):
      print "write()"
      self.msg(str)
    def writelines(self, seq):
      print "writelines()"
      for s in seq:
        self.msg(s)
  """

  def executeProgram(self, cmdline, nwd=None):
    #buf=None
    #if not self.outstream is None:
      #buf = self.myExecuteBuffer(self.outstream)
    try:
      #p = subprocess.Popen(cmdline, shell=True, stdout=buf, stderr=buf)
      p = subprocess.Popen(cmdline, shell=True, stderr=subprocess.PIPE, stdout=subprocess.PIPE, cwd=nwd)
      p.wait()
      lines = []
      if self.options.htmloutput:
        txt = p.stdout.read() + p.stderr.read()
        if len(txt.strip()) > 0 and self.options.verbosity >= 2:
          self.printout("<pre style='border:1px solid blue;padding:0px;margin:0px;'>");
          self.printout(txt)
          self.printout("</pre>");
      else:
        #lines.append('STDOUT:')
        for line in p.stdout.readlines():
            lines.append(' >>> '+line.strip())
        #lines.append('STDERR:')
        for line in p.stderr.readlines():
            lines.append(' >>> '+line.strip())
        self.printout('\n'.join(lines))
      return p.returncode
    except Exception, e:
      self.printout(" ** error while executing '%s': %s" % (cmdline, e.__str__()))

  def getToolExec(self, tool):
    pl = str(platform.system())
    #print pl
    
    if not pl in self.toolset.keys():
      self.printout(" ** error while executing program %s. No path definition for your platform '%s' found." % (tool, pl))
      return ''
    else:
      return self.toolset[pl][tool]

  def processMeshFile(self, filename):
    filename = filename.strip()
    if self.filenameInRoRFiles(filename):
      return
    self.printout(" *** processing mesh: %s" % filename)
    fn = os.path.join(self.src, filename)
    fn2 = os.path.join(self.dst, self.uniqueMeshName(filename))
    if not os.path.isfile(fn):
      #fn = getAlternateSource(filename)
      #if fn == '':
      self.printout(" *** File %s not found ***" % filename)
      self.missingfiles.append(filename)
      return

    self.printout(" * converting mesh %s ..." % filename)

    #cmd = "%s -q %s %s" % (MESHCONVERTER, fn, fn+'.xml')
    tmpfile = fn+'.xml' #'tmp.mesh.xml'
    if str(platform.system()) == "Linux":
      cmd = "%s -q %s %s" % (self.getToolExec('OgreXMLConverter'), fn, tmpfile)
    else:
      cmd = "\"\"%s\" -q \"%s\" \"%s\"\"" % (self.getToolExec('OgreXMLConverter'), fn, tmpfile)
    #cmd = "%s \"%s\" \"%s\"" % (self.getToolExec('OgreXMLConverter'), fn, tmpfile)
    self.printout(cmd)
    if self.executeProgram(cmd, os.path.dirname(fn)) != 0:
      self.printout(" ** error during converting, ignoring that mesh: %s" % filename)

    try:
      f = open(tmpfile, "r")
      lines = f.readlines()
      f.close()
    except:
      self.printout(" ** error converting file: %s" % filename)
      return

    if not os.path.isfile(tmpfile):
      self.printout(" ** error converting file: %s" % filename)
      return

    materialFound = False
    for i in range(0, len(lines)):
      res = lines[i].strip().lower().find('material="')
      if res >= 0:
        #find end
        res2 = lines[i].strip().lower().find('"', res+11)
        if res2 >= 0:
          matname = lines[i].strip()[res+10:res2]
          before = lines[i].strip()[:res+10]
          after = lines[i].strip()[res2:]
          #print "found material in mesh: " + matname
          #print 'before:', before
          #print 'after:', after


          materialFound = True
          if matname in self.materialmap.keys():
            newmat, fromfile = self.materialmap[matname]
            lines[i] = "%s%s%s" % (before, newmat, after)
            #print "%s ==> %s" % (matname, newmat)
          else:
            if self.materialInRoRMaterials(matname):
              continue

            #if matname in ['mirror', 'renderdash', 'seat', 'beacon']:
            #	continue
            if self.options.verbosity > 1:
              self.printout(" ** material found in mesh: %s" % matname)
            newmat = self.uniqueMaterialName(matname)
            self.materialmap[matname.strip()] = (newmat.strip(), filename)
            lines[i] = "%s%s%s" % (before, newmat, after)

    if not materialFound:
        self.printout(" ** error adding unique ID to mesh material, no material found in file: %s" % filename)
    f = open(tmpfile, "w")
    f.writelines(lines)
    f.close()
    if str(platform.system()) == "Linux":
      cmd2 = "%s -q %s %s" % (self.getToolExec('OgreXMLConverter'), tmpfile, fn2)
    else:
      cmd2 = "\"\"%s\" -q \"%s\" \"%s\"\"" % (self.getToolExec('OgreXMLConverter'), tmpfile, fn2)
    #printout(cmd2)
    if self.executeProgram(cmd2, os.path.dirname(fn2)) != 0:
      self.printout(" ** error during converting, ignoring that mesh: %s" % filename)

    try:
      os.remove(tmpfile)
    except:
      pass


    if self.options.upmesh:
      if self.options.verbosity > 0:
        self.printout(" * upgrading mesh: %s" % filename)
      if self.options.opmesh:
        if str(platform.system()) == "Linux":
          cmd = "%s %s %s" % (self.getToolExec('OgreMeshUpgrade'), fn2, fn2+"up")
        else:
          cmd = "\"\"%s\" \"%s\" \"%s\"\"" % (self.getToolExec('OgreMeshUpgrade'), fn2, fn2+"up")
      else:
        if str(platform.system()) == "Linux":
          cmd = "%s -l 3 -d 200 -p 30 -t -b %s %s" % (self.getToolExec('OgreMeshUpgrade'), fn2, fn2+"up")
        else:
          cmd = "\"\"%s\" -l 3 -d 200 -p 30 -t -b \"%s\" \"%s\"\"" % (self.getToolExec('OgreMeshUpgrade'), fn2, fn2+"up")
      if self.executeProgram(cmd, os.path.dirname(fn2)) != 0:
        self.printout(" ** error during mesh upgrading (not upgraded now): %s" % filename)
      else:
        shutil.move(fn2+"up", fn2)

    if self.options.opmesh:
      if self.options.verbosity > 0:
        self.printout(" * optimizing mesh: %s" % filename)
      if str(platform.system()) == "Linux":
        cmd = "%s optimise %s -- %s" % (self.getToolExec('MeshMagick'), fn2, fn2+"op")
      else:
        cmd = "\"\"%s\" optimise \"%s\" -- \"%s\"\"" % (self.getToolExec('MeshMagick'), fn2, fn2+"op")
      self.printout(cmd)
      if self.options.verbosity >= 2:
        self.printout(" * executing: %s" % cmd)
      if self.executeProgram(cmd, os.path.dirname(fn2)) != 0:
        self.printout(" ** error during mesh optimization (not optimized now): %s" % filename)
      else:
        shutil.move(fn2+"op", fn2)

    if self.options.meshinfo:
      if self.options.verbosity > 0:
        self.printout(" * getting mesh info: %s" % filename)
      cmd = "\"\"%s\" info \"%s\"\"" % (self.getToolExec('MeshMagick'), fn2)
      self.executeProgram(cmd, os.path.dirname(fn2))

  def getDrawPositionNode(self, tester, nodeid, width, height, tx, ty, vx, vy, drawmode=0):
    try:
      node = tester.sections['nodes'][int(nodeid)]
      if drawmode == 0:
        # left
        return ( ((node.x+tx) / vx)*width, height-((node.y+ty) / vy)*height )
      elif drawmode == 1:
        # top-down
        return ( ((node.x+tx) / vx)*width, height-((node.z+ty) / vy)*height )
      elif drawmode == 2:
        # 3d-isometric
        # http://www.math.utah.edu/~treiberg/Perspect/Perspect.htm
        x = node.x
        y = node.z
        z = node.y
        x_new = (math.sqrt(3)/2.0)*x + (-math.sqrt(3)/2.0)*y + 0*z
        y_new = (1.0/2.0)*x          + (1.0/2.0)*y           + 1*z
        return ( ((x_new+tx) / vx)*width, height-((y_new+ty) / vy)*height )
    except Exception, e:
      print str(e)
      traceback.print_exc()

  """
  def getDrawPositionRaw(self, pos, width, height, tx, ty, vx, vy, drawmode=0):
    try:
      if drawmode == 0:
        # left
        return ( ((pos[0]+tx) / vx)*width, height-((pos[1]+ty) / vy)*height )
      elif drawmode == 1:
        # top-down
        return ( ((pos[0]+tx) / vx)*width, height-((pos[2]+ty) / vy)*height )
    except Exception, e:
      print str(e)
      traceback.print_exc()
  """

  def drawTruckPreview(self, tester, filename, drawmode=0, detailoption=1):
    tester.getDimensions()

    self.printout(" * drawing scene in mode %d with detail %d" % (drawmode, detailoption))

    if not 'nodes' in tester.sections.keys():
      return
    vx=0
    tx=0 #translation
    vy=0 #virtual sizes
    ty=0
    padding = 0.2 #meters

    if detailoption < 2:
      opacity_invisible = 0.3
    else:
      opacity_invisible = 0.8

    if drawmode == 0:
      #left
      if (tester.minx < 0 and tester.maxx > 0) or (tester.minx > 0 and tester.maxx < 0):
        vx = abs(tester.minx) + abs(tester.maxx)
        if tester.minx < 0:
          tx = abs(tester.minx)
        elif tester.maxx < 0:
          tx = abs(tester.maxx)
      else:
        vx = abs(tester.minx-tester.maxx)

      if (tester.miny < 0 and tester.maxy > 0) or (tester.miny > 0 and tester.maxy < 0):
        vy = abs(tester.miny) + abs(tester.maxy)
        if tester.miny < 0:
          ty = abs(tester.miny)
        elif tester.maxy < 0:
          ty = abs(tester.maxy)
      else:
        vy = abs(tester.miny-tester.maxy)
    elif drawmode == 1:
      #top-down
      if (tester.minx < 0 and tester.maxx > 0) or (tester.minx > 0 and tester.maxx < 0):
        vx = abs(tester.minx) + abs(tester.maxx)
        if tester.minx < 0:
          tx = abs(tester.minx)
        elif tester.maxx < 0:
          tx = abs(tester.maxx)
      else:
        vx = abs(tester.minx-tester.maxx)

      if (tester.minz < 0 and tester.maxz > 0) or (tester.minz > 0 and tester.maxz < 0):
        vy = abs(tester.minz) + abs(tester.maxz)
        if tester.minz < 0:
          ty = abs(tester.minz)
        elif tester.maxz < 0:
          ty = abs(tester.maxz)
      else:
        vy = abs(tester.minz-tester.maxz)
    elif drawmode == 2:
      tx = 0
      ty = 0
      vx = 20
      vy = 20

    vx += padding * 2
    vy += padding * 2
    tx += padding
    ty += padding

    # fit image into area
    width = vx*30
    height = vy*30
    maxWidth = 800
    maxHeight = 800
    scaleX = 1
    scaleY = 1

    scaleX = maxWidth / width
    scaleY = maxHeight / height

    scaleFinale = 0
    if scaleX < scaleY:
      width = width * scaleX
      height = height * scaleX
      scaleFinale = scaleX
    else:
      width = width * scaleY
      height = width * scaleY
      scaleFinale = scaleY

    self.printout(" * image size: %f x %f"%(width, height))

    scene = draw.Scene('test', height, width)

    nodes = tester.sections['nodes']
    beamWidth = 1 * scaleFinale

    if 'nodes' in tester.sections.keys():
      beams = tester.sections['beams']
      for nid in nodes.keys():
        try:
          #print width, height, tx, ty, vx, vy
          pos = self.getDrawPositionNode(tester, nid, width, height, tx, ty, vx, vy, drawmode)
          #print pos
          scene.add(draw.Circle(pos, 2*scaleFinale,(0,0,255)))
        except Exception, e:
          print str(e)
          traceback.print_exc()

    if 'beams' in tester.sections.keys():
      beams = tester.sections['beams']
      for b in beams.values():
        try:
          pos1 = self.getDrawPositionNode(tester, b[0], width, height, tx, ty, vx, vy, drawmode)
          pos2 = self.getDrawPositionNode(tester, b[1], width, height, tx, ty, vx, vy, drawmode)
          color = (0,0,0)
          opacity = 1
          if len(b) > 2:
            if 'i' in b[2]:
              opacity = opacity_invisible
              color = (150, 150, 150)
          scene.add(draw.Line(pos1, pos2, color, beamWidth, opacity))
        except Exception, e:
          print str(e)
          traceback.print_exc()



    if 'shocks' in tester.sections.keys():
      shocks = tester.sections['shocks']
      for s in shocks.values():
        try:
          pos1 = self.getDrawPositionNode(tester, s[0], width, height, tx, ty, vx, vy, drawmode)
          pos2 = self.getDrawPositionNode(tester, s[1], width, height, tx, ty, vx, vy, drawmode)
          color = (50,50,100)
          opacity = 1
          if len(s) > 7:
            if 'i' in s[7]:
              opacity = opacity_invisible
              color = (150, 150, 200)
          scene.add(draw.Line(pos1, pos2, color, beamWidth, opacity))
        except Exception, e:
          print str(e)
          traceback.print_exc()

    if 'hydros' in tester.sections.keys():
      hydros = tester.sections['hydros']
      for h in hydros.values():
        try:
          pos1 = self.getDrawPositionNode(tester, h[0], width, height, tx, ty, vx, vy, drawmode)
          pos2 = self.getDrawPositionNode(tester, h[1], width, height, tx, ty, vx, vy, drawmode)
          color = (50,100,100)
          opacity = 1
          if len(h) > 3:
            if 'i' in h[3]:
              opacity = opacity_invisible
              color = (150, 200, 200)
          scene.add(draw.Line(pos1, pos2, color, beamWidth, opacity))
        except Exception, e:
          print str(e)
          traceback.print_exc()

    if 'commands' in tester.sections.keys():
      commands = tester.sections['commands']
      for c in commands.values():
        try:
          pos1 = self.getDrawPositionNode(tester, c[0], width, height, tx, ty, vx, vy, drawmode)
          pos2 = self.getDrawPositionNode(tester, c[1], width, height, tx, ty, vx, vy, drawmode)
          color = (100,100,0)
          opacity = 1
          if len(c) > 7:
            if 'i' in c[7]:
              opacity = opacity_invisible
              color = (150, 150, 0)
          scene.add(draw.Line(pos1, pos2, color, beamWidth, opacity))
        except Exception, e:
          print str(e)
          traceback.print_exc()

    if 'commands2' in tester.sections.keys():
      commands2 = tester.sections['commands2']
      for c in commands2.values():
        try:
          pos1 = self.getDrawPositionNode(tester, c[0], width, height, tx, ty, vx, vy, drawmode)
          pos2 = self.getDrawPositionNode(tester, c[1], width, height, tx, ty, vx, vy, drawmode)
          color = (100,100,0)
          opacity = 1
          if len(c) > 7:
            if 'i' in c[7]:
              opacity = opacity_invisible
              color = (150, 150, 0)
          scene.add(draw.Line(pos1, pos2, color, beamWidth, opacity))
        except Exception, e:
          print str(e)
          traceback.print_exc()

    if 'wheels' in tester.sections.keys():
      wheels = tester.sections['wheels']
      for w in wheels.values():
        try:
          radius = float(w[0])
          radius = radius / vx * width
          pos1 = self.getDrawPositionNode(tester, w[3], width, height, tx, ty, vx, vy, drawmode)
          pos2 = self.getDrawPositionNode(tester, w[4], width, height, tx, ty, vx, vy, drawmode)
          color = (100,100,100)
          #print pos1b, pos2b
          if drawmode == 0:
            # left
            scene.add(draw.Circle(pos1, radius, color, 0.2))
            scene.add(draw.Circle(pos2, radius, color, 0.2))
          elif drawmode == 1:
            # top-down
            scene.add(draw.Line(pos1, pos2, color, radius*2, 0.2))
        except Exception, e:
          print str(e)
          traceback.print_exc()

    if 'wheels2' in tester.sections.keys():
      wheels2 = tester.sections['wheels2']
      for w in wheels2.values():
        try:
          radius0 = float(w[0])
          radius0 = radius0 / vx * width
          radius1 = float(w[1])
          radius1 = radius1 / vx * width
          pos1 = self.getDrawPositionNode(tester, w[4], width, height, tx, ty, vx, vy, drawmode)
          pos2 = self.getDrawPositionNode(tester, w[5], width, height, tx, ty, vx, vy, drawmode)
          color = (100,100,100)
          #print pos1b, pos2b
          if drawmode == 0:
            # left
            scene.add(draw.Circle(pos1, radius0, color, 0.2))
            scene.add(draw.Circle(pos1, radius1, color, 0.3))
            scene.add(draw.Circle(pos2, radius0, color, 0.2))
            scene.add(draw.Circle(pos2, radius1, color, 0.3))
          elif drawmode == 1:
            # top-down
            scene.add(draw.Line(pos1, pos2, color, radius0*2, 0.2))
            scene.add(draw.Line(pos1, pos2, color, radius1*2, 0.3))
            scene.add(draw.Line(pos1, pos2, color, radius0*2, 0.2))
            scene.add(draw.Line(pos1, pos2, color, radius1*2, 0.3))
        except Exception, e:
          print str(e)
          traceback.print_exc()

    if 'meshwheels' in tester.sections.keys():
      meshwheels = tester.sections['meshwheels']
      for w in meshwheels.values():
        try:
          radius0 = float(w[0])
          radius0 = radius0 / vx * width
          radius1 = float(w[1])
          radius1 = radius1 / vx * width
          pos1 = self.getDrawPositionNode(tester, w[4], width, height, tx, ty, vx, vy, drawmode)
          pos2 = self.getDrawPositionNode(tester, w[5], width, height, tx, ty, vx, vy, drawmode)
          color = (100,100,100)
          color2 = (200,200,200)
          #print pos1b, pos2b
          if drawmode == 0:
            # left
            scene.add(draw.Circle(pos1, radius0, color, 0.2))
            scene.add(draw.Circle(pos1, radius1, color, 0.3))
            scene.add(draw.Circle(pos2, radius0, color, 0.2))
            scene.add(draw.Circle(pos2, radius1, color, 0.3))
          elif drawmode == 1:
            # top-down
            scene.add(draw.Line(pos1, pos2, color, radius0*2, 0.2))
            scene.add(draw.Line(pos1, pos2, color2, radius1*2, 0.3))
            scene.add(draw.Line(pos1, pos2, color, radius0*2, 0.2))
            scene.add(draw.Line(pos1, pos2, color2, radius1*2, 0.3))
        except Exception, e:
          print str(e)
          traceback.print_exc()

    if 'cab' in tester.sections.keys():
      cab = tester.sections['cab']
      try:
        for submeshno in cab.keys():
          clines = cab[submeshno]
          for c in clines:
            cpoints = []
            for i in [0,1,2]:
              pos = self.getDrawPositionNode(tester, c[i], width, height, tx, ty, vx, vy, drawmode)
              cpoints.append(','.join(["%s" % el for el in pos]))

            cpoints.append(cpoints[0])
            fcolor = (100,100,0)
            points = ' , '.join(cpoints)
            scene.add(draw.Polygon(fcolor, points, 0, 0.1))
      except Exception, e:
        print str(e)
        traceback.print_exc()

    if 'props' in tester.sections.keys():
      props = tester.sections['props']
      try:
        for prop in props.values():
          meshname = prop[9].strip()
          print meshname

          srcfile = os.path.join(self.src, meshname)
          tmpfile = os.path.join(self.dst, meshname+'.xml')
          cmd = "\"\"%s\" -q \"%s\" \"%s\"\"" % (self.getToolExec('OgreXMLConverter'), srcfile, tmpfile)
          self.printout(cmd)
          if self.executeProgram(cmd) != 0:
            self.printout(" ** error during mesh conversion, ignoring that mesh: %s" % filename)
            continue

        """
          from xml.dom import minidom

          try:
            f = open(tmpfile, "r")
            xml_content = f.read()
            f.close()
          except:
            self.printout(" ** error converting file: %s" % filename)
            return

          self.printout(" ** loading xmml document: %s" % tmpfile)
          xmldoc = minidom.parseString(xml_content).documentElement


          # find vertexes
          #for ref in xmldoc.getElementsByTagName("sharedgeometry"):
          #	for vertexbuffer in ref.childNodes:
          #		for vertex in vertexbuffer.childNodes:
          #			print vertex
          #continue

          # find faces
          vertexbuffer = {}
          vertexcounter = 0

          facebuffer = {}
          facecounter = 0
          for ref in xmldoc.getElementsByTagName("submeshes"):
            for submesh in ref.childNodes:
              #submesh
              if submesh.nodeType == submesh.ELEMENT_NODE:
                print "+"+submesh.tagName
              for facegroup in submesh.childNodes:
                if facegroup.nodeType != facegroup.ELEMENT_NODE:
                  continue
                print "++"+facegroup.tagName



                if facegroup.tagName == 'faces':
                  for face in facegroup.childNodes:
                    if face.nodeType != face.ELEMENT_NODE:
                      continue
                    if face.tagName == 'face':
                      v1 = int(face.getAttribute("v1"))
                      v2 = int(face.getAttribute("v3"))
                      v3 = int(face.getAttribute("v3"))
                      facebuffer[facecounter] = [v1, v2, v3]
                      #print 'face: ', facecounter, facebuffer[facecounter]
                      facecounter += 1

                elif facegroup.tagName == 'geometry':
                  for geom1 in facegroup.childNodes:
                    for geom2 in geom1.childNodes:
                      if geom2.nodeType != geom2.ELEMENT_NODE:
                        continue

                      if geom2.tagName == 'vertex':
                        for geom3 in geom2.childNodes:
                          if geom3.nodeType != geom3.ELEMENT_NODE:
                            continue

                          if geom3.tagName == 'position':
                            x = float(geom3.getAttribute("x"))
                            y = float(geom3.getAttribute("y"))
                            z = float(geom3.getAttribute("z"))
                            vertexbuffer[vertexcounter] = [x, y, z]
                            #print 'vertex: ', vertexcounter, vertexbuffer[vertexcounter]
                            vertexcounter += 1
          if len(facebuffer) > 0 and len(vertexbuffer) > 0:
            scene.add(draw.Comment("Prop"))
            for face in facebuffer.values():
              coords = []
              for vnum in face:
                pos = vertexbuffer[vnum]
                pos = self.getDrawPositionRaw(pos, width, height, tx, ty, vx, vy, drawmode)
                coords.append("%f,%f" % (pos[0], pos[1]))

              coords = ' , '.join(coords)
              fcolor = (200,100,100)
              scene.add(draw.Polygon(fcolor, coords, 0.1, 0.5))
            #print 'facebuffer: ', facebuffer
            #print 'vertexbuffer: ', vertexbuffer

        pass
        """
      except Exception, e:
        print str(e)
        traceback.print_exc()

    #scene.write_svg(filename)
    #scene.display()


  def processMaterialFile(self, filename):
    #RE_textures = r"^\s*texture.?([a-zA-Z0-9_\-]*\.[a-zA-Z0-9]*)\s?.*"
    #RE_materials = r"\s?material\s?([a-zA-Z0-9_/\-\\]*).?"
    f = open(os.path.join(self.src, filename), "r")
    lines = f.readlines()
    f.close()

    for i in range(0, len(lines)):
      if lines[i].strip().lower().startswith('material '):
        args = lines[i].strip().split(' ')
        if len(args) == 2:
          matname = args[1].strip()
          newmatname = self.uniqueMaterialName(matname)
          self.materialmap[matname] = (newmatname.strip(), filename)
          args[1] = newmatname
          lines[i] = ' '.join(args) + self.newLineChar()

      elif lines[i].strip().lower().startswith('texture '):
        args = lines[i].strip().split(' ')
        if len(args) >= 2:
          imgname = args[1].strip()
          if self.filenameInRoRFiles(imgname):
            continue
          newimgname = self.uniqueTextureName(imgname)
          self.imagemap[imgname] = (newimgname, filename)

          #re-add spaces
          spaces = ''
          for j in range(0, len(lines[i])):
            if lines[i][j] in [' ', '\t']:
              spaces += lines[i][j]
            else:
              break

          args[1] = newimgname
          lines[i] = spaces + ' '.join(args) + self.newLineChar()

    fn = os.path.join(self.dst, self.uniqueMaterialFileName(filename))
    f = open(fn, "w")
    f.writelines(lines)
    f.close()

    """
    if self.options.upmat:
      self.printout(" * upgrading material: %s" % filename)
      cmd = "\"\"%s\" \"%s\" \"%s\"\"" % (self.getToolExec('OgreMaterialUpgrade'), fn, fn+"up")
      if self.optionsverbosity >= 2:
        self.printout(" === executing: %s" % cmd)
      if self.executeProgram(cmd) != 0:
        self.printout(" ** error during material upgrade (not upgraded now): %s" % filename)
      else:
        shutil.move(fn+"up", fn)
    """

  def processTerrainConfig(self, filename, lines):
    for line in lines:
      if line.startswith('#') or line.strip() == '':
        continue
      args = line.strip().split('=')
      key = args[0].lower()
      value = args[1]
      if key == 'worldtexture':
        newimgname = self.uniqueTextureName(value)
        self.imagemap[value] = (newimgname, filename)
      elif key == 'detailtexture':
        newimgname = self.uniqueTextureName(value)
        self.imagemap[value] = (newimgname, filename)
      elif key == 'heightmap.image':
        self.requestedHeightMaps.append((value, filename))


  def processTerrainFile(self, filename):
    self.printout(" processTerrainFile %s"%filename)
    f = open(os.path.join(self.src, filename), "r")
    lines = f.readlines()
    f.close()

  def processODEFFile(self, filename):
    if not os.path.isfile(filename):
      filename = os.path.join(self.src, 'objects')
      if not os.path.isfile(filename):
        return
    self.printout(" processODEFFile %s"%filename)
    f = open(os.path.join(self.src, filename), "r")
    lines = f.readlines()
    f.close()

  def processConfigFile(self, filename):
    f = open(os.path.join(self.src, filename), "r")
    lines = f.readlines()
    f.close()

    # detect config type
    fileType='terrain'
    if os.path.basename(filename) == 'editor.cfg':
      fileType='editor'
    elif os.path.basename(filename) == 'categories.cfg':
      fileType='categories'
    elif os.path.basename(filename) == 'ground_models.cfg':
      fileType='ground_models'
    elif os.path.basename(filename) == 'ground_models.cfg':
      fileType='inputevents'
    elif os.path.basename(filename) == 'wavefield.cfg':
      fileType='wavefield'
    elif os.path.basename(filename) == 'ogre.cfg':
      fileType='ogre'
    elif os.path.basename(filename) == 'resources.cfg':
      fileType='resources'
    if os.path.basename(filename).startswith('plugins_'):
      fileType='plugins'

    if fileType == 'terrain':
      # normal TSM config file
      self.processTerrainConfig(filename, lines)
    elif fileType == 'editor':
      # terrain editor config file
      for line in lines:
        if line.startswith(';') or line.strip() == '':
          continue
        self.requestedODEFs.append( (line.strip(), 'editor.cfg'))
    elif fileType == 'categories':
      pass
    elif fileType == 'ground_models':
      pass
    elif fileType == 'inputevents':
      pass
    elif fileType == 'wavefield':
      pass
    elif fileType == 'ogre':
      pass
    elif fileType == 'resources':
      pass
    elif fileType == 'plugins':
      pass

  def processTruckFile(self, filename):
    f = open(os.path.join(self.src, filename), "r")
    lines_org = f.readlines()
    f.close()

    if self.options.checktr:
      # test it, write report
      self.tester = HTMLRoRTester()
      lines, res = self.tester.run(lines_org, filename)
      
      # add -mini images if existing	
      fnwoext, fnext = os.path.splitext(filename)
      imgname = fnwoext + "-mini.dds"
      minifile = os.path.join(self.src, 'textures', 'temp', imgname)
      if self.options.checktr and os.path.isfile(minifile):
        self.tester.minitype = 'dds'
      newimgname = self.uniqueTextureName(imgname)
      self.imagemap[imgname] = (newimgname, filename)

      imgname = fnwoext + "-mini.png"
      minifile = os.path.join(self.src, 'textures', 'temp', imgname)
      if self.options.checktr and os.path.isfile(minifile):
        self.tester.minitype = 'png'
      newimgname = self.uniqueTextureName(imgname)
      self.imagemap[imgname] = (newimgname, filename)
    
      self.testers.append(self.tester)
      #sys.stdout.write(". uniquifying ...")

      if self.options.checktr_writeCorrected and self.tester.correctederrors > 0:
        self.printout(" * updating original truck with corrected version (%d/%d errors corrected)..." % (self.tester.correctederrors, len(self.tester.errors)))
        f = open(os.path.join(self.src, filename), "w")
        f.writelines(lines)
        f.close()


      if self.options.checktr_intoResult:
        resultfn = os.path.join(self.dst, self.uniqueTruckFileName(filename)+'.testresult.html')
      else:
        resultfn = os.path.join(self.dst, filename+'.testresult.html')

      fw = open(resultfn, "w")
      fw.write(res)
      fw.close()

      if self.options.generatetruckimage > 0:
        self.printout(" * generating images...")
        # draw image ;)
        fn1 = os.path.join(self.dst, self.uniqueTruckFileName(filename)+'.left.svg')
        fn2 = os.path.join(self.dst, self.uniqueTruckFileName(filename)+'.left.png')
        self.drawTruckPreview(self.tester, fn1, 0, self.options.generatetruckimage)
        cmdl = "\"\"%s\" \"%s\" \"%s\"\"" % (self.getToolExec('convert'), fn1, fn2)
        self.executeProgram(cmdl, os.path.dirname(fn2))

        fn1 = os.path.join(self.dst, self.uniqueTruckFileName(filename)+'.topdown.svg')
        fn2 = os.path.join(self.dst, self.uniqueTruckFileName(filename)+'.topdown.png')
        self.drawTruckPreview(self.tester, fn1, 1, self.options.generatetruckimage)
        cmdl = "\"\"%s\" \"%s\" \"%s\"\"" % (self.getToolExec('convert'), fn1, fn2)
        self.executeProgram(cmdl, os.path.dirname(fn2))

        fn1 = os.path.join(self.dst, self.uniqueTruckFileName(filename)+'.3d.svg')
        fn2 = os.path.join(self.dst, self.uniqueTruckFileName(filename)+'.3d.png')
        self.drawTruckPreview(self.tester, fn1, 2, self.options.generatetruckimage)
        #cmdl = "\"\"%s\" \"%s\" \"%s\"\"" % (self.getToolExec('convert'), fn1, fn2)
        #self.executeProgram(cmdl)
    
    cursection = ''
    for i in range(0, len(lines)):
      if len(lines[i].strip()) == 0 or lines[i][0] == ';':
        continue

      # ignore commands for now
      for cmd in self.COMMANDS:
        if lines[i].startswith(cmd):
          continue

      #set current section
      if lines[i].strip() in self.SECTIONS:
        cursection = lines[i].strip()
        continue

      if lines[i].startswith('fileinfo'):
        args = lines[i].strip().split(',')
        args[0] = 'fileinfo ' + self.uid
        lines[i] = ', '.join(args) + self.newLineChar()
        continue

      if lines[i].startswith('set_beam_defaults'):
        args = lines[i].strip().split(',')
        if len(args) == 6:
          matname = args[5].strip()
          if self.options.verbosity > 1:
            self.printout(" ** material found in truck / set_beam_defaults: %s" % matname)
          if not self.materialInRoRMaterials(matname):
            newmatname = self.uniqueMaterialName(matname)
            self.materialmap[matname] = (newmatname.strip(), filename)
            args[5] = newmatname
            lines[i] = ', '.join(args) + self.newLineChar()
        continue

      if cursection == 'globals':
        args = lines[i].strip().split(',')
        if len(args) == 3:
          matname = args[2].strip()
          if self.options.verbosity > 1:
            self.printout(" ** material found in truck / globals: %s" % matname)
          if not self.materialInRoRMaterials(matname):
            newmatname = self.uniqueMaterialName(matname)
            self.materialmap[matname] = (newmatname.strip(), filename)
            args[2] = newmatname
            lines[i] = ', '.join(args)+self.newLineChar()
          continue

      elif cursection == 'help':
        matname = lines[i].strip()
        if self.options.verbosity > 1:
          self.printout(" ** material found in truck / help: %s" % matname)
        if self.materialInRoRMaterials(matname):
          newmatname = self.uniqueMaterialName(matname)
          self.materialmap[matname] = (newmatname.strip(), filename)
          lines[i] = newmatname +self.newLineChar()
        continue

      elif cursection == 'wheels':
        args = lines[i].strip().split(',')
        #print args
        if len(args) == 13:
          args2 = args[12].strip().split(' ')
          if len(args2) == 2:
            matname = args2[0].strip()
            newmats = matname
            if self.options.verbosity > 1:
              self.printout(" ** material found in truck / wheels: %s" % matname)
            if not self.materialInRoRMaterials(matname):
              newmatname = self.uniqueMaterialName(matname)
              self.materialmap[matname] = (newmatname, filename)
              newmats = newmatname

            matname = args2[1]
            if self.options.verbosity > 1:
              self.printout(" ** material found in truck / wheels: %s" % matname)
            if not self.materialInRoRMaterials(matname):
              newmatname = self.uniqueMaterialName(matname)
              self.materialmap[matname] = (newmatname, filename)
              newmats += " " + newmatname
            else:
              newmats += " " + matname


            args[12] = newmats
            lines[i] = ', '.join(args) + self.newLineChar()


      elif cursection == 'wheels2':
        args = lines[i].strip().split(',')
        #print args
        if len(args) == 16:
          args2 = args[15].strip().split(' ')
          if len(args2) == 2:
            matname = args2[0].strip()
            newmats = matname
            if self.options.verbosity > 1:
              self.printout(" ** material found in truck / wheels2: %s" % matname)
            if not self.materialInRoRMaterials(matname):
              newmatname = self.uniqueMaterialName(matname)
              self.materialmap[matname] = (newmatname, filename)
              newmats = newmatname

            matname = args2[1]
            if self.options.verbosity > 1:
              self.printout(" ** material found in truck / wheels2: %s" % matname)
            if not self.materialInRoRMaterials(matname):
              newmatname = self.uniqueMaterialName(matname)
              self.materialmap[matname] = (newmatname, filename)
              newmats += " " + newmatname
            else:
              newmats += " " + matname

            args[15] = newmats
            lines[i] = ', '.join(args) + self.newLineChar()

      elif cursection == 'meshwheels':
        args = lines[i].strip().split(',')
        #print args
        if len(args) == 15:
          args2 = args[14].strip().split(' ')
          if len(args2) == 2:
            meshname = args2[0].strip()
            newmats = meshname
            if self.options.verbosity > 1:
              self.printout(" ** mesh found in truck / meshwheels: %s" % meshname)
            if not self.filenameInRoRFiles(meshname):
              newmeshname = self.uniqueMeshName(meshname)
              self.meshmap[meshname] = newmeshname
              newmats = newmeshname

            matname = args2[1].strip()
            if self.options.verbosity > 1:
              self.printout(" ** material found in truck / meshwheels: %s" % matname)
            if not self.materialInRoRMaterials(matname):
              newmatname = self.uniqueMaterialName(matname)
              self.materialmap[matname] = (newmatname, filename)
              newmats += " " + newmatname
            else:
              newmats += " " + matname

            args[14] = newmats
            #print args
            lines[i] = ', '.join(args) + self.newLineChar()

      elif cursection == 'flares':
        args = lines[i].strip().split(',')
        #print args
        if len(args) == 9:
          args2 = args[8].strip().split(' ')
          if len(args2) == 2:
            matname = args2[1].strip()
            if matname == 'default':
              continue
            if self.options.verbosity > 1:
              self.printout(" ** material found in truck / flares: %s" % matname)
            if not self.materialInRoRMaterials(matname):
              newmatname = self.uniqueMaterialName(matname)
              self.materialmap[matname] = (newmatname, filename)
              newmats = args2[0] + " " + newmatname

            args[8] = newmats
            lines[i] = ', '.join(args) + self.newLineChar()

      elif cursection == 'props':
        args = lines[i].strip().split(',')
        #print args
        if len(args) == 10:
          meshname = args[9].strip()
          if self.options.verbosity > 1:
            self.printout(" ** mesh found in truck / props: %s" % meshname)
          if self.filenameInRoRFiles(meshname):
            continue
          newmeshname = self.uniqueMeshName(meshname)
          self.meshmap[meshname] = newmeshname
          args[9] = newmeshname
          lines[i] = ', '.join(args) + self.newLineChar()

      elif cursection == 'flexbodies':
        args = lines[i].strip().split(',')
        #print args
        if len(args) == 10:
          meshname = args[9].strip()
          if self.options.verbosity > 1:
            self.printout(" ** mesh found in truck / flexbodies: %s" % meshname)
          if self.filenameInRoRFiles(meshname):
            continue
          newmeshname = self.uniqueMeshName(meshname)
          self.meshmap[meshname] = newmeshname
          args[9] = newmeshname
          lines[i] = ', '.join(args) + self.newLineChar()

      elif cursection == 'exhausts':
        args = lines[i].strip().split(',')
        #print args
        if len(args) == 3:
          args2 = args[2].strip().split(' ')
          if len(args2) == 2:
            matname = args2[1].strip()
            if matname == 'default':
              continue
            if self.options.verbosity > 1:
              self.printout(" ** material found in truck / exhausts: %s" % matname)
            if not self.materialInRoRMaterials(matname):
              newmatname = self.uniqueMaterialName(matname)
              self.materialmap[matname] = (newmatname, filename)
              newmats = args[0] + " " + newmatname

            args[2] = newmats
            lines[i] = ', '.join(args) + self.newLineChar()

      elif cursection == 'wings':
        args = lines[i].strip().split(',')
        #print args
        if len(args) == 21:
          aflname = args[20].strip()
          if self.filenameInRoRFiles(aflname):
            continue
          newaflname = self.uniqueAFLName(aflname)
          self.aflmap[aflname] = newaflname
          args[20] = newaflname
          lines[i] = ', '.join(args) + self.newLineChar()

      elif cursection == 'envmap':
        matname = lines[i].strip()
        self.printout(" ** material found in truck / envmap: %s" % matname)
        if not self.materialInRoRMaterials(matname):
          newmatname = self.uniqueMaterialName(matname)
          self.materialmap[matname] = (newmatname.strip(), filename)
          lines[i] = newmatname

      elif cursection == 'managedmaterials':
        args = lines[i].strip().split(' ')
        if len(args) >= 3:
          target_mat = args[0]
          type = args[1]
          
          if type in ['flexmesh_standard', 'flexmesh_transparent']:
            # add uid to target material
            self.printout(" ** material found in truck / managedmaterials: %s" % target_mat)
            newmtarget_mat = self.uniqueMaterialName(target_mat)
            self.materialmap[target_mat] = (newmtarget_mat.strip(), filename)
            self.materialmap_prov[target_mat] = (newmtarget_mat.strip(), filename)

            # add uid to base texture
            base_texture = args[2]
            newbase_texture = self.uniqueTextureName(base_texture)
            self.imagemap[base_texture] = (newbase_texture, filename)
            base_texture = newbase_texture
            
            # parse rest
            damage_texture = '-'
            spec_texture = '-'
            if len(args) > 3:
              damage_texture = args[3].strip()
            if len(args) > 4:
              spec_texture = args[4].strip()
          
            if damage_texture != '-':
              newdamage_texture = self.uniqueTextureName(damage_texture)
              self.imagemap[damage_texture] = (newdamage_texture, filename)
              damage_texture = newdamage_texture
            
            if spec_texture != '-':
              newspec_texture = self.uniqueTextureName(spec_texture)
              self.imagemap[spec_texture] = (newspec_texture, filename)
              spec_texture = newspec_texture
                        
            lines[i] = "%s %s %s %s %s" % (newmtarget_mat, type, base_texture, damage_texture, spec_texture)
          elif type in ['mesh_standard', 'mesh_transparent']:
            # add uid to target material
            self.printout(" ** material found in truck / managedmaterials: %s" % target_mat)
            newmtarget_mat = self.uniqueMaterialName(target_mat)
            self.materialmap[target_mat] = (newmtarget_mat.strip(), filename)
            self.materialmap_prov[target_mat] = (newmtarget_mat.strip(), filename)

            # add uid to base texture
            base_texture = args[2]
            newbase_texture = self.uniqueTextureName(base_texture)
            self.imagemap[base_texture] = (newbase_texture, filename)
            base_texture = newbase_texture
            
            # parse rest
            spec_texture = '-'
            if len(args) > 3:
              spec_texture = args[3].strip()
          
            if spec_texture != '-':
              newspec_texture = self.uniqueTextureName(spec_texture)
              self.imagemap[spec_texture] = (newspec_texture, filename)
              spec_texture = newspec_texture
                        
            lines[i] = "%s %s %s %s" % (newmtarget_mat, type, base_texture, spec_texture)

      elif cursection == 'turboprops':
        args = lines[i].strip().split(',')
        #print args
        if len(args) == 8:
          aflname = args[7].strip()
          if self.filenameInRoRFiles(aflname):
            continue
          newaflname = self.uniqueAFLName(aflname)
          self.aflmap[aflname] = newaflname
          args[7] = newaflname
          lines[i] = ', '.join(args) + self.newLineChar()

      elif cursection == 'fusedrag':
        args = lines[i].strip().split(',')
        #print args
        if len(args) == 4:
          aflname = args[3].strip()
          if self.filenameInRoRFiles(aflname):
            continue
          newaflname = self.uniqueAFLName(aflname)
          self.aflmap[aflname] = newaflname
          args[3] = newaflname
          lines[i] = ', '.join(args) + self.newLineChar()

      elif cursection == 'turboprops2':
        args = lines[i].strip().split(',')
        #print args
        if len(args) == 9:
          aflname = args[8].strip()
          if self.filenameInRoRFiles(aflname):
            continue
          newaflname = self.uniqueAFLName(aflname)
          self.aflmap[aflname] = newaflname
          args[8] = newaflname
          lines[i] = ', '.join(args) + self.newLineChar()

      elif cursection == 'pistonprops':
        args = lines[i].strip().split(',')
        #print args
        if len(args) == 10:
          aflname = args[9].strip()
          if self.filenameInRoRFiles(aflname):
            continue
          newaflname = self.uniqueAFLName(aflname)
          self.aflmap[aflname] = newaflname
          args[9] = newaflname
          lines[i] = ', '.join(args) + self.newLineChar()

    # enforce line return
    nl = []
    for line in lines:
      nl.append(line.strip()+self.newLineChar())
    lines = nl

    truckfn = self.uniqueTruckFileName(filename)
    f = open(os.path.join(self.dst, truckfn), "w")
    f.writelines(lines)
    f.close()

    #if self.options.generateidfile:
      #idfn = os.path.join(self.dst, self.uniqueIDFileName())
      #f = open(idfn, "a")
      #line = "%s %s\n" % (truckfn, filename)
      #f.write(line)
      #f.close()
    #  pass

    # copy over mini picture if existing
    for format in ['.png', '.dds']:
      basename, filename2 = os.path.split(filename)
      fnwoext, fnext = os.path.splitext(filename2)

      fnsrc = os.path.join(self.src, fnwoext+'-mini'+format)
      fndst = os.path.join(self.dst, self.uniqueMinipictureFileName(fnwoext+'-mini'+format))
      if os.path.isfile(fnsrc):
        shutil.copyfile(fnsrc, fndst)
        self.printout(" * copied mini image %s to %s" % (fnsrc, fndst))



