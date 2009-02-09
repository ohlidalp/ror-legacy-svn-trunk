#!/bin/env python
# rewritten for RoR 0.34 05/02/2008 thomas (at) thomasfischer.biz

import sys, os.path, re, math, copy, time, traceback

DBAV=0
try:
    from pysqlite2 import dbapi2 as sqlite
    DBAV=1
except ImportError, e:
    pass

#database disabled	
DBAV=0
    
if DBAV:
    print "database available"
    DB = sqlite.connect('data.db')

ERR_SYNTAX=0
ERR_HIGH=1

SEC_WARN=0
SEC_FATAL=1
SEC_PERF=2


VERSIONINFO = {
    '0.33' : ['description', 'end_description', 'end', 'globals', 'nodes', 'beams', 'cameras', 'cinecam', 'help', 'comment', 'end_comment', 'engine', 'engoption', 'brakes', 'wheels', 'wheels2', 'shocks', 'hydros', 'commands', 'rotators', 'contacters', 'ropes', 'fixes', 'minimass', 'ties', 'ropables', 'particles', 'ropables2', 'flares', 'props', 'submesh', 'texcoords', 'cab', 'backmesh', 'guisettings', 'wings', 'turboprops', 'airbrakes', 'fusedrag', 'screwprops', 'globeams', 'fileformatversion', 'author', 'fileinfo', 'forwardcommands', 'importcommands', 'set_beam_defaults', 'rollon', 'hookgroup','rescuer'],
    '0.34' : ['set_skeleton_settings', 'commands2', 'exhausts', 'turboprops2', 'pistonprops'],
    '0.35' : ['rigidifiers', 'turbojets', 'meshwheels', 'flexbodies'],
    '0.36' : ['meshgroups', 'envmap', 'managedmaterials', 'section'],
}


class Error:
    line=-1
    type=0
    msg=''
    points=0
    severity=0
    corrected=False
    def __init__(self, line, type, msg, points, sev=SEC_WARN, corrected=False):
        self.line = line
        self.type=type
        self.msg=msg
        self.points=points
        self.severity=sev
        self.corrected=corrected
        #print line, msg

class Node:
    id=0
    x=0
    y=0
    z=0
    options=''
    def __init__(self, args):
        try:
            self.id=int(args[0])
            self.x=float(args[1])
            self.y=float(args[2])
            self.z=float(args[3])
            if len(args)>4:
                self.options = args[4]
        except Exception, e:
            print str(e)
            traceback.print_exc()
    def __str__(self):
        return "(%0.2f,%0.2f,%0.2f,%s)" % (self.x, self.y, self.z, self.options)

class RoRTester:
    loglines=[]
    minimumversion = '0.0.0'
    sections={}
    errors = []
    
    def __init__(self, lines, filename):
        self.starttime=time.clock()
        self.fname = filename
        self.fname_without_uid = filename
        fnwoext, fnext = os.path.splitext(filename)
        self.fext = fnext.lstrip('.')
        self.dname = ''
        self.version = ''
        self.categoryid = 0
        self.uid = ''
        self.minitype = ''
        self.loglines=[]
        self.parseFile(lines)
        
    def log(self, msg):
        tstr = "%02.4f" % (time.clock()-self.starttime)
        self.loglines.append(tstr+"| "+msg.__str__())

    def isFloat(self, value):
        try:
            m = re.search("([-+]?(\d+(\.\d*)?|\.\d+)([eE][-+]?\d+)?)", value)
            if len(m.groups()) == 0:
                return False
            f = float(value)
            return True
        except:
            return False

    def isFilledSection(self, name):
        return (name in self.sections.keys() and self.sections[name] > 0)

    def isInt(self, value):
        try:
            m = re.search("([-+]?\d+)", value)
            if len(m.groups()) == 0:
                return False
            return (float(value) % 1.0 == 0.0)
        except:
            return False

    def isNode(self, value):
        if not 'nodes' in self.sections.keys():
            # no nodes (yet?)
            return False
        if not self.isInt(value):
            return False
        else:
            return (int(value) in self.sections['nodes'].keys())

    def isKey(self, value):
        if not self.isInt(value):
            return False
        else:
            value = int(value)
            return (value>0 and value<42)
            
    def getMinVersion(self, name):
        for version in VERSIONINFO.keys():
            if name in VERSIONINFO[version]:
                return version
        return '0';

    def getDimensions(self):
        if not 'nodes' in self.sections.keys():
            return
        # find maxima and minima
        self.minx =  99999
        self.maxx = -99999
        self.miny =  99999
        self.maxy = -99999
        self.minz =  99999
        self.maxz = -99999
        for n in self.sections['nodes'].values():
            if n.x < self.minx:
                self.minx = n.x
            if n.x > self.maxx:
                self.maxx = n.x
            if n.y < self.miny:
                self.miny = n.y
            if n.y > self.maxy:
                self.maxy = n.y
            if n.z < self.minz:
                self.minz = n.z
            if n.z > self.maxz:
                self.maxz = n.z
        # semi-ball approx.
        if 'wheels' in self.sections.keys():
            for w in self.sections['wheels'].values():
                radius = float(w[0])
                node1 = self.sections['nodes'][int(w[3])]
                node2 = self.sections['nodes'][int(w[4])]
                wheelnodes = (node1, node2)
                for node in wheelnodes:
                    if node.x + radius > self.maxx:
                        self.maxx = node.x + radius
                    if node.x - radius < self.minx:
                        self.minx = node.x - radius
                    if node.y + radius > self.maxy:
                        self.maxy = node.y + radius
                    if node.y - radius < self.miny:
                        self.miny = node.y - radius
                    #if node.z + radius > self.maxz:
                    #	self.maxz = node.z + radius
                    #if node.z - radius < self.minz:
                    #	self.minz = node.z - radius
        if 'wheels2' in self.sections.keys():
            for w in self.sections['wheels2'].values():
                radiuses = (float(w[0]), float(w[1]))
                for radius in radiuses:
                    node1 = self.sections['nodes'][int(w[4])]
                    node2 = self.sections['nodes'][int(w[5])]
                    wheelnodes = (node1, node2)
                    for node in wheelnodes:
                        if node.x + radius > self.maxx:
                            self.maxx = node.x + radius
                        if node.x - radius < self.minx:
                            self.minx = node.x - radius
                        if node.y + radius > self.maxy:
                            self.maxy = node.y + radius
                        if node.y - radius < self.miny:
                            self.miny = node.y - radius
        if 'meshwheels' in self.sections.keys():
            for w in self.sections['meshwheels'].values():
                radiuses = (float(w[0]), float(w[1]))
                for radius in radiuses:
                    node1 = self.sections['nodes'][int(w[4])]
                    node2 = self.sections['nodes'][int(w[5])]
                    wheelnodes = (node1, node2)
                    for node in wheelnodes:
                        if node.x + radius > self.maxx:
                            self.maxx = node.x + radius
                        if node.x - radius < self.minx:
                            self.minx = node.x - radius
                        if node.y + radius > self.maxy:
                            self.maxy = node.y + radius
                        if node.y - radius < self.miny:
                            self.miny = node.y - radius
        self.log("dimensions: %f,%f,  %f,%f,  %f,%f" % (self.minx, self.maxx, self.miny, self.maxy, self.minz, self.maxz))
    
    def checkForWrongFormat(self, errors, lineno, insertlines, source, target):
        # check for wrong formatted stuff, like 'Nodes'
        source = source.rstrip('\n\r')
        if source.strip().lower() == target and source != target:
            #print "source: '%s', target: '%s'" % (source, target)
            errors.append(Error(lineno, ERR_SYNTAX, "invalid format", -1, SEC_FATAL, True))
            insertlines[lineno] = ";corrected format below: it was '%s' instead of the correct '%s'" % (source.replace('\n', '<LB>').replace('\r', '<LB>'), target)
            # ew must replace  now, since we need the corrected header later
            #self.correctedlines[lineno] = "%s" % (target)
            #source = target
            return True, target
        return False, ""
            
    
    def parseFile(self, lines):
        errors = []
        beamlines = {}
        insertlines = {}
        materialdependencies = []
        afldependencies = []
        meshdependencies = []
        sectionorder = []
        cameralines={}
        self.sections = {}
        commentMode=False
        section_keywords = ['beams', 'cinecam', 'nodes', 'cameras', 'end', 'description', 'end_description', 'help', 'comment' 'end_comment', 'engine', 'engoption', 'brakes', 'wheels', 'wheels2', 'shocks', 'hydros', 'commands', 'commands2', 'commandlist_begin', 'commandlist_end', 'rotators', 'contacters', 'ropes', 'fixes', 'minimass', 'ties', 'ropables', 'particles', 'flares', 'props', 'submesh', 'exhausts', 'guisettings', 'wings', 'turboprops', 'fusedrag', 'turboprops2', 'pistonprops', 'turbojets', 'screwprops', 'globeams', 'rigidifiers', 'texcoords', 'cab', 'airbrakes', 'globals', 'flexbodies', 'meshwheels', 'envmap', 'managedmaterials', 'end_section']
        command_keywords = ['set_beam_defaults', 'rollon', 'fileformatversion', 'author', 'fileinfo', 'forwardcommands', 'importcommands','set_skeleton_settings', 'backmesh', 'tachoMaterial', 'speedoMaterial', 'speedoMax', 'useMaxRPM','rescuer','sectionconfig','section']
        section = ""
        self.dname = ''
        submeshcounter = 0
        self.correctedlines = copy.copy(lines)
        self.log("checking syntax...")
        for lineno in range(0, len(lines)):
        
            msg = "   processing truck (%03d%%)" % (int(lineno)/float(len(lines))*100)
            sys.stdout.write("%s%s" % (msg, '\b'*len(msg)))
            line = lines[lineno].rstrip('\n\r')
            #print "'%s' -> '%s'" % (lines[lineno], line)
            #print line
            
            if lineno == 0:
                self.dname = line.strip()
            # comment mode
            if line == "comment":
                commentMode=True
                continue
            if commentMode and line == "end_comment":
                commentMode=False
                continue
            elif commentMode:
                continue

            # check for wrong formatted sections
            for cmd in section_keywords:
                res, cline = self.checkForWrongFormat(errors, lineno, insertlines, line, cmd)
                if res:
                    line = cline
                    #print "\n'%s' -> '%s'" % (line, cline)
                    self.correctedlines[lineno] = line
                    # not more then one section per line
                    break;
            
            # check for wrong formatted commands
            for cmd in command_keywords:
                oline = copy.copy(line[:len(cmd)])
                res, cline = self.checkForWrongFormat(errors, lineno, insertlines, oline, cmd)
                if res:
                    #print "\n"+oline, " -> ", cline
                    line = cline + line[len(cmd):]
                    #print "\n"+line
                    self.correctedlines[lineno] = line
                    # not more then one command per line
                    break;
            
            #print '%s: %s' %(section, line)
            
            #check for command
            iscommand=False
            for c in command_keywords:
                # check minimum version
                if line[:len(c)] == c:
                    minver = self.getMinVersion(c)
                    #print c, minver
                    if minver > self.minimumversion:
                        #print "mew minversion: ", c,  minver
                        self.minimumversion = minver
                        
                    iscommand=True
                    if c == 'set_beam_defaults':
                        try:
                            args = line[len(c):].split(',')
                            alen = len(args)
                            # restrict for material
                            if alen > 5:
                                alen = 5
                            for i in range(0, alen):
                                if not self.isFloat(args[i]):
                                    errors.append(Error(lineno, ERR_SYNTAX, "set_beam_defaults: argument must be a float", -1))							
                            if len(args) > 5:
                                materialdependencies.append(args[5].strip())
                        except Exception, e:
                            print str(e)
                            errors.append(Error(lineno, ERR_SYNTAX, "generic set_beam_defaults line error", -1, ERR_SYNTAX)) 
                    elif c == 'speedoMax':
                        try:
                            if not self.isFloat(line[len(c):]):
                                errors.append(Error(lineno, ERR_SYNTAX, "speedoMax argument must be a float", -1))
                        except Exception, e:
                            print str(e)
                            errors.append(Error(lineno, ERR_SYNTAX, "generic speedoMax line error", -1, ERR_SYNTAX)) 
                    elif c == 'useMaxRPM':						
                        try:
                            if not self.isInt(line[len(c):]) or not int(line[len(c):]) in [0,1]:
                                errors.append(Error(lineno, ERR_SYNTAX, "useMaxRPM argument must be 0 or 1", -1))
                        except Exception, e:
                            print str(e)
                            errors.append(Error(lineno, ERR_SYNTAX, "generic useMaxRPM line error", -1, ERR_SYNTAX)) 
                                
                    elif c == 'set_skeleton_settings':
                        try:
                            args = line[len(c):].split(',')
                            if len(args) < 2:
                                errors.append(Error(lineno, ERR_SYNTAX, "too less arguments for valid set_skeleton_settings!", -1, ERR_SYNTAX))

                            for i in range(0, 1):
                                if not self.isFloat(args[i]):
                                    errors.append(Error(lineno, ERR_SYNTAX, "%s for set_skeleton_settings is not a valid float!" % (args[i]), -1, ERR_SYNTAX))
                        except Exception, e:
                            print str(e)
                            errors.append(Error(lineno, ERR_SYNTAX, "generic set_skeleton_settings line error", -1, ERR_SYNTAX)) 
                                                
                    elif c == 'tachoMaterial':
                        materialdependencies.append(line[len(c):].strip())
                    elif c == 'speedoMaterial':
                        materialdependencies.append(line[len(c):].strip())
                    elif c == 'fileformatversion':
                        try:
                            if not int(line[len(c):].strip()) in [1,2]:
                                errors.append(Error(lineno, ERR_SYNTAX, "fileformat version invalid. Must be 1 or 2", -1))
                        except Exception, e:
                            print str(e)
                            errors.append(Error(lineno, ERR_SYNTAX, "generic fileformatversion line error", -1, ERR_SYNTAX)) 
                    elif c == 'author':
                        try:
                            args = line[len(c):].strip().split(' ')
                            #print args
                            if len(args) != 4:
                                errors.append(Error(lineno, ERR_SYNTAX, "author line has too less arguments to be valid", -1, SEC_WARN, True))
                            
                            if len(args) > 1:
                                if not self.isInt(args[1]):
                                    errors.append(Error(lineno, ERR_SYNTAX, "authorid must be a number", -1))
                            
                            authortype = 'author'
                            if len(args) > 0:
                                authortype = args[0].strip()
                            
                            authorid = -1
                            if len(args) > 1:
                                try:
                                    authorid = int(args[1].strip())
                                except:
                                    pass

                            authorname = 'unknown'
                            if len(args) > 2:
                                authorname = args[2].strip()

                            authoremail = 'unknown'
                            if len(args) > 3:
                                authoremail = args[3].strip()
                                
                            #insertlines[lineno] = ';XXX fixed author line below')
                            self.correctedlines[lineno] = "author %s %d %s %s" % (authortype, authorid, authorname, authoremail)
                        except Exception, e:
                            print str(e)
                            errors.append(Error(lineno, ERR_SYNTAX, "generic author line error", -1, ERR_SYNTAX)) 
                        
                    elif c == 'fileinfo':
                        try:
                            args = line[len(c):].split(',')
                            if len(args) != 3:
                                errors.append(Error(lineno, ERR_SYNTAX, "fileinfo line has too less arguments to be valid", -1))
                                break
                            
                            # todo: fix uid detection!
                            #if str(args[0]).strip() != '-1' and len(args[0].strip()) != 40:
                            #	errors.append(Error(lineno, ERR_SYNTAX, "fileinfo: uniqueid must be -1 or the unique hash", -1))
                            if not self.isInt(args[1]):
                                errors.append(Error(lineno, ERR_SYNTAX, "fileinfo: categoryid must be an integer", -1))
                                self.categoryid = -1
                            else:
                                self.categoryid = int(args[1])

                            if not int(args[1]) in [108,146,147,148,149,150,155,152,153,154,107,151,156,159,160,161,162,110,113,114,117,118]:
                                errors.append(Error(lineno, ERR_SYNTAX, "fileinfo: categoryid %d is no valid category number" % int(args[1]), -1))
                            if not self.isInt(args[2]):
                                errors.append(Error(lineno, ERR_SYNTAX, "fileinfo: fileversion must be an integer", -1))
                                self.version = -1
                            else:
                                self.version = int(args[2])
                            
                        except Exception, e:
                            print str(e)
                            errors.append(Error(lineno, ERR_SYNTAX, "generic fileinfo line error", -1, SEC_FATAL)) 
                    break
            
            if iscommand:
                continue
            # check for new section
            if line in section_keywords:
                section=line
                
                if not section in self.sections:
                    self.sections[section] = {}
                
                # check for version
                minver = self.getMinVersion(section)
                if minver > self.minimumversion:
                    #print "mew minversion: ", section, minver
                    self.minimumversion = minver
                    
                #check for submesh
                if section == 'submesh':
                    submeshcounter += 1

                sectionorder.append(section)
                continue
            
            # ignore comments and empty lines
            if len(line.strip()) == 0 or line[0] == ";":
                continue
                
            args = line.split(' ')
            nargs = []
            for i in range(0,len(args)):
                args2 = args[i].strip().split(',')
                for a2 in args2:
                    if a2 != "":
                        nargs.append(a2)
            args=nargs
            #print args
            
            #////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
            if section == 'nodes':
                try:
                    isloadnode=False
                    if len(args) < 4:
                        errors.append(Error(lineno, ERR_SYNTAX, "to less arguments for valid node", -1))
                    if not self.isInt(args[0]):
                        errors.append(Error(lineno, ERR_SYNTAX, "first node argument must be a plain integer", -1))
                    if not self.isFloat(args[1]):
                        errors.append(Error(lineno, ERR_SYNTAX, "second node argument must be a float", -1))
                    if not self.isFloat(args[2]):
                        errors.append(Error(lineno, ERR_SYNTAX, "third node argument must be a float", -1))
                    if not self.isFloat(args[3]):
                        errors.append(Error(lineno, ERR_SYNTAX, "fourth node argument must be a float", -1))
                    if len(args) >=5:
                        for c in args[4].strip():
                            if c == 'l':
                                isloadnode=True
                            if not c in ['l','f','x','y','c','h','e','b','n']:
                                errors.append(Error(lineno, ERR_SYNTAX, "invalid node option: %s" % c, -1))
                    if len(args) >=6:
                        if not self.isFloat(args[5]):
                            errors.append(Error(lineno, ERR_SYNTAX, "seventh node argument must be a float", -1))
                        else:
                            if not isloadnode:
                                errors.append(Error(lineno, ERR_SYNTAX, "specified load weight without the node being a load node", -1))
                    args[0] = int(args[0])
                    args[1] = float(args[1])
                    args[2] = float(args[2])
                    args[3] = float(args[3])
                    nodeid = args[0]
                    if len(self.sections[section]) != args[0]:
                        errors.append(Error(lineno, ERR_SYNTAX, "node counter desync! %d should follow"%nodecounter, -1, SEC_FATAL))
                    
                    node = Node(args)
                    self.sections[section][nodeid] = node
                except:
                    errors.append(Error(lineno, ERR_SYNTAX, "generic node line error", -1, SEC_FATAL))
            #////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
            elif section == 'beams':
                try:
                    if len(args) < 2:
                        errors.append(Error(lineno, ERR_SYNTAX, "to less arguments for valid beam", -1))
                    if not self.isInt(args[0]):
                        errors.append(Error(lineno, ERR_SYNTAX, "first beam argument must be a plain integer", -1))
                    if not self.isInt(args[1]):
                        errors.append(Error(lineno, ERR_SYNTAX, "second beam argument must be a plain integer", -1))
                    if len(args) >=3:
                        for c in args[2].strip():
                            if c == 'l':
                                isloadnode=True
                            if not c in ['i','r','v','n']:
                                errors.append(Error(lineno, ERR_SYNTAX, "invalid beam option: %s" % c, -1))
                    args[0] = int(args[0])
                    args[1] = int(args[1])
                    
                    beamcounter = len(self.sections[section])
                    
                    if args[0] == args[1]:
                        errors.append(Error(lineno, ERR_SYNTAX, "beam between same nodes is invalid", -1, SEC_FATAL, True))
                        insertlines[lineno] = ';XXX FIX beam below (%d) was using same node numbers(%d,%d)' % (beamcounter, args[0], args[1])						
                        self.correctedlines[lineno] = ";" + self.correctedlines[lineno]

                    
                    if self.isNode(args[0]):
                        errors.append(Error(lineno, ERR_SYNTAX, "node %d for beam %d does not exist!" % (args[0], beamcounter), -1, SEC_FATAL))
                    if self.isNode(args[1]):
                        errors.append(Error(lineno, ERR_SYNTAX, "node %d for beam %d does not exist!" % (args[1], beamcounter), -1, SEC_FATAL))
                    
                    #beams[beamcounter] = args
                    beamlines[beamcounter] = lineno
                    #beamcounter+=1
                    
                    self.sections[section][beamcounter] = args
                except Exception, e:
                    print str(e)
                    traceback.print_exc()
                    errors.append(Error(lineno, ERR_SYNTAX, "generic beam line error", -1, SEC_FATAL))
            #////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
            elif section == 'cameras':
                cameralines[lineno] = args
            #////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
            elif section == 'cinecam':
                try:
                    if len(args) < 11:
                        errors.append(Error(lineno, ERR_SYNTAX, "to less arguments for valid cinecam", -1))
                    for i in range(0,2):
                        if not self.isFloat(args[i]):
                            errors.append(Error(lineno, ERR_SYNTAX, "%d. cinecam argument must be a float"%i, -1))
                    
                    for i in range(3,11):
                        if not self.isNode(args[i]):
                            errors.append(Error(lineno, ERR_SYNTAX, "node %s for cinecam does not exist!" % (args[i]), -1, SEC_FATAL))
                    
                    self.sections[section][len(self.sections[section])] = args

                except Exception, e:
                    print str(e)
                    traceback.print_exc()
                    errors.append(Error(lineno, ERR_SYNTAX, "generic cinecam line error", -1, SEC_FATAL))
            #////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
            elif section == 'help':
                try:
                    if len(args) < 1:
                        errors.append(Error(lineno, ERR_SYNTAX, "to less arguments for valid help", -1))
                    materialdependencies.append(args[0].strip())
                    
                    self.sections[section][len(self.sections[section])] = args					
                except Exception, e:
                    print str(e)
                    traceback.print_exc()
                    errors.append(Error(lineno, ERR_SYNTAX, "generic help line error", -1, SEC_FATAL))
            #////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
            elif section == 'globals':
                try:
                    if len(args) < 3:
                        errors.append(Error(lineno, ERR_SYNTAX, "to less arguments for valid globals", -1))
                    for i in range(0,1):
                        if not self.isFloat(args[i]):
                            errors.append(Error(lineno, ERR_SYNTAX, "%d. globals argument must be a float"%i, -1))
                    args[0] = float(args[0])
                    args[1] = float(args[1])
                    materialdependencies.append(args[2].strip())
                    
                    self.sections[section][len(self.sections[section])] = args
                except Exception, e:
                    print str(e)
                    traceback.print_exc()
                    errors.append(Error(lineno, ERR_SYNTAX, "generic globals line error", -1, SEC_FATAL))
            #////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
            elif section == 'engine':
                try:
                    if len(args) < 7:
                        errors.append(Error(lineno, ERR_SYNTAX, "to less arguments for valid engine", -1))
                    lastgear_found = False
                    lastgear_nonzero = 0
                    for i in range(0, len(args)):
                        if not self.isFloat(args[i]):
                            errors.append(Error(lineno, ERR_SYNTAX, "%d. engine argument must be a float"%i, -1))
                        else:
                            if float(args[i]) == -1:
                                lastgear_found = True
                            elif float(args[i]) != 0 and lastgear_found == False and i > 6:
                                lastgear_nonzero = i
                    
                    if not lastgear_found:
                        errors.append(Error(lineno, ERR_SYNTAX, "last engine gear must be -1", -1, SEC_WARN, True))
                        
                        args = line.split(',')
                        lastgear_nonzero += 1
                        #print lastgear_nonzero
                        if lastgear_nonzero >= len(args):
                            lastgear_nonzero = len(args) -1
                        args[lastgear_nonzero] = -1
                        self.correctedlines[lineno] = ', '.join(["%s" % el for el in args])

                    
                    self.sections[section][len(self.sections[section])] = args
                except Exception, e:
                    print str(e)
                    traceback.print_exc()
                    errors.append(Error(lineno, ERR_SYNTAX, "generic engine line error", -1, SEC_FATAL))
            #////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
            elif section == 'engoption':
                try:
                    if len(args) < 2:
                        errors.append(Error(lineno, ERR_SYNTAX, "to less arguments for valid engoption", -1))
                    if not self.isFloat(args[0]):
                        errors.append(Error(lineno, ERR_SYNTAX, "first engoption argument must be a float"%i, -1))
                    if not args[1].strip() in ['c', 't']:
                        errors.append(Error(lineno, ERR_SYNTAX, "invalid engine type: %s"%args[1].strip(), -1))

                    self.sections[section][len(self.sections[section])] = args
                except Exception, e:
                    print str(e)
                    traceback.print_exc()
                    errors.append(Error(lineno, ERR_SYNTAX, "generic engoption line error", -1, SEC_FATAL))
            #////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
            elif section == 'brakes':
                try:
                    if len(args) < 1:
                        errors.append(Error(lineno, ERR_SYNTAX, "to less arguments for valid brakes", -1))
                    if not self.isFloat(args[0]):
                        errors.append(Error(lineno, ERR_SYNTAX, "first brake argument must be a float"%i, -1))

                    self.sections[section][len(self.sections[section])] = args
                except Exception, e:
                    print str(e)
                    traceback.print_exc()
                    errors.append(Error(lineno, ERR_SYNTAX, "generic brake line error", -1, SEC_FATAL))
            #////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
            elif section == 'wheels':
                try:
                    #;radius, width, numrays, node1, node2, snode, braked, propulsed, arm, mass,  spring,   damp,   facemat          bandmat
                    #0.54,   0.40,  12,      35,    36,    9999,  1,      0,         3,   200.0, 800000.0, 4000.0, tracks/wheelface tracks/wheelband1
                    if len(args) < 14:
                        errors.append(Error(lineno, ERR_SYNTAX, "to less arguments for valid wheel", -1))
                    
                    for i in range(0,1):
                        if not self.isFloat(args[i]):
                            errors.append(Error(lineno, ERR_SYNTAX, "%d. wheel argument must be a float"%i, -1))
                    
                    if not self.isInt(args[2]):
                        errors.append(Error(lineno, ERR_SYNTAX, "numrays wheel argument must be an integer", -1))
                    
                    if not self.isNode(args[3]):
                        errors.append(Error(lineno, ERR_SYNTAX, "node %s for node1 argument in wheels does not exist!" % (args[3]), -1, SEC_FATAL))
                        
                    if not self.isNode(args[4]):
                        errors.append(Error(lineno, ERR_SYNTAX, "node %s for node2 argument in wheels does not exist!" % (args[4]), -1, SEC_FATAL))

                    if not self.isInt(args[5]) and int(args[5]) != 9999 and not self.isNode(abs(int(args[5]))):
                        errors.append(Error(lineno, ERR_SYNTAX, "node %s for snode argument in wheels does not exist (or use 9999 if not used)!" % (args[5]), -1, SEC_FATAL))

                    if not self.isInt(args[6]) or not int(args[6]) in [0,1,2,3]:
                        errors.append(Error(lineno, ERR_SYNTAX, "braked wheel argument must be 0, 1, 2 or 3", -1))

                    if not self.isInt(args[7]) or not int(args[7]) in [0,1,2]:
                        errors.append(Error(lineno, ERR_SYNTAX, "propulsed wheel argument must be 0, 1 or 2", -1))

                    if not self.isNode(args[8]):
                        errors.append(Error(lineno, ERR_SYNTAX, "node %s for arm argument in wheels does not exist!" % (args[8]), -1, SEC_FATAL))

                    for i in range(9,11):
                        if not self.isFloat(args[i]):
                            errors.append(Error(lineno, ERR_SYNTAX, "%d. wheel argument must be a float"%i, -1))
                    
                    if args[12].strip() == "":
                        errors.append(Error(lineno, ERR_SYNTAX, "facematerial wheel argument cannot be empy", -1, SEC_FATAL))
                    else:
                        materialdependencies.append(args[12].strip())

                    if args[13].strip() == "":
                        errors.append(Error(lineno, ERR_SYNTAX, "bandmaterial wheel argument cannot be empy", -1, SEC_FATAL))
                    else:
                        materialdependencies.append(args[13].strip())

                    self.sections[section][len(self.sections[section])] = args
                except Exception, e:
                    print str(e)
                    traceback.print_exc()
                    errors.append(Error(lineno, ERR_SYNTAX, "generic wheel line error", -1, SEC_FATAL))
            #////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
            elif section == 'wheels2':
                try:
                    #;radius,              width,numrays,node1,node2,snode,braked,propulsed, arm, mass, spring,        damp,       facemat          bandmat
                    #;radius,radius2,width,numrays,node1,node2,snode,braked,propulsed, arm, mass, rim spring, rim damp, simple spring, simple damp, texface, texband
                    #0.335, 0.625, 0.40, 12, 44, 45, 9999, 1, 1, 3, 280.0, 900000.0, 200.0, 400000.0, 2000.0, tracks/daffwheelface tracks/dafwheelband
                    if len(args) < 17:
                        errors.append(Error(lineno, ERR_SYNTAX, "to less arguments for valid wheel2", -1))
                    
                    for i in range(0,2):
                        if not self.isFloat(args[i]):
                            errors.append(Error(lineno, ERR_SYNTAX, "%d. wheel2 argument must be a float"%i, -1))
                    
                    if not self.isInt(args[3]):
                        errors.append(Error(lineno, ERR_SYNTAX, "numrays wheel2 argument must be an integer", -1))
                    
                    if not self.isNode(args[4]):
                        errors.append(Error(lineno, ERR_SYNTAX, "node %s for node1 argument in wheel2 does not exist!" % (args[4]), -1, SEC_FATAL))
                        
                    if not self.isNode(args[5]):
                        errors.append(Error(lineno, ERR_SYNTAX, "node %s for node2 argument in wheel2 does not exist!" % (args[5]), -1, SEC_FATAL))

                    if not self.isInt(args[6]) and int(args[5]) != 9999 and not self.isNode(abs(int(args[6]))):
                        errors.append(Error(lineno, ERR_SYNTAX, "node %s for snode argument in wheel2 does not exist (or use 9999 if not used)!" % (args[6]), -1, SEC_FATAL))

                    if not self.isInt(args[7]) or not int(args[7]) in [0,1,2,3]:
                        errors.append(Error(lineno, ERR_SYNTAX, "braked wheel2 argument must be 0, 1, 2 or 3", -1))

                    if not self.isInt(args[8]) or not int(args[8]) in [0,1,2]:
                        errors.append(Error(lineno, ERR_SYNTAX, "propulsed wheel2 argument must be 0, 1 or 2, not %s" %(args[9]), -1))

                    if not self.isNode(args[9]):
                        errors.append(Error(lineno, ERR_SYNTAX, "node %s for arm argument in wheel2 does not exist!" % (args[9]), -1, SEC_FATAL))

                    for i in range(10,14):
                        if not self.isFloat(args[i]):
                            errors.append(Error(lineno, ERR_SYNTAX, "%d. wheel2 argument must be a float"%i, -1))
                    
                    if args[15].strip() == "":
                        errors.append(Error(lineno, ERR_SYNTAX, "facematerial wheel2 argument cannot be empy", -1, SEC_FATAL))
                    else:
                        materialdependencies.append(args[15].strip())

                    if args[16].strip() == "":
                        errors.append(Error(lineno, ERR_SYNTAX, "bandmaterial wheel2 argument cannot be empy", -1, SEC_FATAL))
                    else:
                        materialdependencies.append(args[16].strip())

                    self.sections[section][len(self.sections[section])] = args
                except Exception, e:
                    print str(e)
                    traceback.print_exc()
                    errors.append(Error(lineno, ERR_SYNTAX, "generic wheel2 line error", -1, SEC_FATAL))
            #////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
            elif section == 'meshwheels':
                try:
                    if len(args) < 16:
                        errors.append(Error(lineno, ERR_SYNTAX, "to less arguments for valid meshwheels", -1))
                    
                    for i in range(0,2):
                        if not self.isFloat(args[i]):
                            errors.append(Error(lineno, ERR_SYNTAX, "%d. meshwheels argument must be a float"%i, -1))
                    
                    if not self.isInt(args[3]):
                        errors.append(Error(lineno, ERR_SYNTAX, "numrays meshwheels argument must be an integer", -1))
                    
                    if not self.isNode(args[4]):
                        errors.append(Error(lineno, ERR_SYNTAX, "node %s for node1 argument in meshwheels does not exist!" % (args[4]), -1, SEC_FATAL))
                        
                    if not self.isNode(args[5]):
                        errors.append(Error(lineno, ERR_SYNTAX, "node %s for node2 argument in meshwheels does not exist!" % (args[5]), -1, SEC_FATAL))

                    if not self.isInt(args[6]) and int(args[5]) != 9999 and not self.isNode(abs(int(args[6]))):
                        errors.append(Error(lineno, ERR_SYNTAX, "node %s for snode argument in meshwheels does not exist (or use 9999 if not used)!" % (args[6]), -1, SEC_FATAL))
                    
                    if not self.isInt(args[7]) or not int(args[7]) in [0,1,2,3]:
                        errors.append(Error(lineno, ERR_SYNTAX, "braked meshwheels argument must be 0, 1, 2 or 3", -1))

                    if not self.isInt(args[8]) or not int(args[8]) in [0,1,2]:
                        errors.append(Error(lineno, ERR_SYNTAX, "propulsed meshwheels argument must be 0, 1 or 2, not %s" %(args[9]), -1))

                    if not self.isNode(args[9]):
                        errors.append(Error(lineno, ERR_SYNTAX, "node %s for arm argument in meshwheels does not exist!" % (args[9]), -1, SEC_FATAL))

                    for i in range(10,12):
                        if not self.isFloat(args[i]):
                            errors.append(Error(lineno, ERR_SYNTAX, "%d. meshwheels argument must be a float"%i, -1))
                    
                    if not args[13].strip() in ['l','r']:
                            errors.append(Error(lineno, ERR_SYNTAX, "invalid meshwheels side: %s" % c, -1))
                    
                    if args[14].strip() == "":
                        errors.append(Error(lineno, ERR_SYNTAX, "mesh meshwheels argument cannot be empy", -1, SEC_FATAL))
                    else:
                        meshdependencies.append(args[14].strip())

                    if args[15].strip() == "":
                        errors.append(Error(lineno, ERR_SYNTAX, "material meshwheels argument cannot be empy", -1, SEC_FATAL))
                    else:
                        materialdependencies.append(args[15].strip())

                    self.sections[section][len(self.sections[section])] = args
                except Exception, e:
                    print str(e)
                    traceback.print_exc()
                    errors.append(Error(lineno, ERR_SYNTAX, "generic meshwheels line error", -1, SEC_FATAL))
            #////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
            elif section == 'managedmaterials':
                #new_material type base_texture damage_texture specular_texture
                try:
                    if len(args) < 3:
                        errors.append(Error(lineno, ERR_SYNTAX, "to less arguments for valid managedmaterials", -1))
                    
                    mmtype = args[1]
                    if not mmtype in ['flexmesh_standard', 'flexmesh_transparent', 'mesh_standard', 'mesh_transparent']:
                        errors.append(Error(lineno, ERR_SYNTAX, "managedmaterials type '%s' unkown" % mmtype, -1))
                        
                    if mmtype == 'flexmesh_standard':
                        # todo: add checking? not very much possible here, so leave it out...
                        pass

                except Exception, e:
                    print str(e)
                    traceback.print_exc()
                    errors.append(Error(lineno, ERR_SYNTAX, "generic managedmaterials line error", -1, SEC_FATAL))
            #////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////		
            elif section == 'shocks':
                try:
                    if len(args) < 7:
                        errors.append(Error(lineno, ERR_SYNTAX, "to less arguments for valid shocks", -1))
                    for i in range(0,1):
                        if not self.isNode(args[i]):
                            errors.append(Error(lineno, ERR_SYNTAX, "node %s for shocks does not exist!" % (args[i]), -1, SEC_FATAL))

                    for i in range(2,6):
                        if not self.isFloat(args[i]):
                            errors.append(Error(lineno, ERR_SYNTAX, "%d. shocks argument must be a float"%i, -1))
                    
                    if len(args) >=8:
                        for c in args[7].strip():
                            if not c in ['l','r','i','n']:
                                errors.append(Error(lineno, ERR_SYNTAX, "invalid shocks option: %s" % c, -1))
                    
                    args[0] = int(args[0])
                    args[1] = int(args[1])
                    
                    self.sections[section][len(self.sections[section])] = args
                except Exception, e:
                    print str(e)
                    traceback.print_exc()
                    errors.append(Error(lineno, ERR_SYNTAX, "generic shocks line error", -1, SEC_FATAL))
            #////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////		
            elif section == 'hydros':
                try:
                    if len(args) < 3:
                        errors.append(Error(lineno, ERR_SYNTAX, "to less arguments for valid hydros", -1))
                    
                    for i in range(0,1):
                        if not self.isNode(args[i]):
                            errors.append(Error(lineno, ERR_SYNTAX, "node %s for hydros does not exist!" % (args[i]), -1, SEC_FATAL))

                    if not self.isFloat(args[2]):
                        errors.append(Error(lineno, ERR_SYNTAX, "3. hydros argument must be a float", -1))
                    
                    if len(args) >=4:
                        for c in args[3].strip():
                            if not c in ['s','i','n']:
                                errors.append(Error(lineno, ERR_SYNTAX, "invalid hydros option: %s" % c, -1))

                    self.sections[section][len(self.sections[section])] = args
                except Exception, e:
                    print str(e)
                    traceback.print_exc()
                    errors.append(Error(lineno, ERR_SYNTAX, "generic hydros line error", -1, SEC_FATAL))
            #////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////		
            elif section == 'commands':
                try:
                    if len(args) < 7:
                        errors.append(Error(lineno, ERR_SYNTAX, "to less arguments for valid commands", -1))
                    
                    for i in range(0,1):
                        if not self.isNode(args[i]):
                            errors.append(Error(lineno, ERR_SYNTAX, "node %s for commands does not exist!" % (args[i]), -1, SEC_FATAL))

                    for i in range(2,4):
                        if not self.isFloat(args[i]):
                            errors.append(Error(lineno, ERR_SYNTAX, "%d. commands argument must be a float"%i, -1))

                    for i in range(5,6):
                        if not self.isKey(args[i]):
                            errors.append(Error(lineno, ERR_SYNTAX, "%d. commands argument must be a valid key"%i, -1))
                        
                    if len(args) > 7:
                        for c in args[7].strip():
                            if not c in ['n','i','r']:
                                errors.append(Error(lineno, ERR_SYNTAX, "invalid commands option: %s" % c, -1))
                                
                    self.sections[section][len(self.sections[section])] = args
                except Exception, e:
                    print str(e)
                    traceback.print_exc()
                    errors.append(Error(lineno, ERR_SYNTAX, "generic commands line error", -1, SEC_FATAL))
            #////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////		
            elif section == 'commands2':
                try:
                    if len(args) < 8:
                        errors.append(Error(lineno, ERR_SYNTAX, "to less arguments for valid commands2", -1))
                    
                    for i in range(0,1):
                        if not self.isNode(args[i]):
                            errors.append(Error(lineno, ERR_SYNTAX, "node %s for commands2 does not exist!" % (args[i]), -1, SEC_FATAL))

                    for i in range(2,5):
                        if not self.isFloat(args[i]):
                            errors.append(Error(lineno, ERR_SYNTAX, "%d. commands2 argument must be a float"%i, -1))

                    for i in range(6,7):
                        if not self.isKey(args[i]):
                            errors.append(Error(lineno, ERR_SYNTAX, "%d. commands2 argument must be a valid key"%i, -1))
                        
                    if len(args) > 8:
                        for c in args[8].strip():
                            if not c in ['n','i','r','c','f','p','o']:
                                errors.append(Error(lineno, ERR_SYNTAX, "invalid commands2 option: %s" % c, -1))
                    
                    self.sections[section][len(self.sections[section])] = args
                except Exception, e:
                    print str(e)
                    traceback.print_exc()
                    errors.append(Error(lineno, ERR_SYNTAX, "generic commands2 line error", -1, SEC_FATAL))
            #////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////		
            elif section == 'rotators':
                try:
                    #;axis1, axis2, a1, a2, a3, a4,   b1, b2  b3, b4, rate, keyleft, keyright
                    #29,     30,    31, 32, 34, 33,   37, 38, 36, 35, 0.1,  1,       2
                    if len(args) < 13:
                        errors.append(Error(lineno, ERR_SYNTAX, "to less arguments for valid rotators", -1))
                    
                    for i in range(0,9):
                        if not self.isNode(args[i]):
                            errors.append(Error(lineno, ERR_SYNTAX, "node %s for rotators does not exist!" % (args[i]), -1, SEC_FATAL))

                    
                    if not self.isFloat(args[10]):
                        errors.append(Error(lineno, ERR_SYNTAX, "rate rotators argument must be a float", -1))

                    if not self.isKey(args[11]):
                        errors.append(Error(lineno, ERR_SYNTAX, "keyleft rotators argument must be a valid key (0-42)", -1))

                    if not self.isKey(args[12]):
                        errors.append(Error(lineno, ERR_SYNTAX, "keyright rotators argument must be a valid key (0-42)", -1))

                    self.sections[section][len(self.sections[section])] = args
                except Exception, e:
                    print str(e)
                    traceback.print_exc()
                    errors.append(Error(lineno, ERR_SYNTAX, "generic rotators line error", -1, SEC_FATAL))
            #////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////		
            elif section == 'contacters':
                try:
                    if len(args) < 1:
                        errors.append(Error(lineno, ERR_SYNTAX, "to less arguments for valid contacters", -1))
                    
                    if not self.isNode(args[0]):
                        errors.append(Error(lineno, ERR_SYNTAX, "node %s for contacters does not exist!" % (args[0]), -1, SEC_FATAL))
                    
                    self.sections[section][len(self.sections[section])] = args
                except Exception, e:
                    print str(e)
                    traceback.print_exc()
                    errors.append(Error(lineno, ERR_SYNTAX, "generic contacters line error", -1, SEC_FATAL))
            #////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////		
            elif section == 'ropes':
                try:
                    if len(args) < 2:
                        errors.append(Error(lineno, ERR_SYNTAX, "to less arguments for valid ropes", -1))
                    
                    if not self.isNode(args[0]):
                        errors.append(Error(lineno, ERR_SYNTAX, "argument 1 node %s for ropes does not exist!" % (args[0]), -1, SEC_FATAL))
                    if not self.isNode(args[1]):
                        errors.append(Error(lineno, ERR_SYNTAX, "argument 2 node %s for ropes does not exist!" % (args[1]), -1, SEC_FATAL))
                
                    self.sections[section][len(self.sections[section])] = args
                except Exception, e:
                    print str(e)
                    traceback.print_exc()
                    errors.append(Error(lineno, ERR_SYNTAX, "generic ropes line error", -1, SEC_FATAL))
            #////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////		
            elif section == 'fixes':
                try:
                    if len(args) < 1:
                        errors.append(Error(lineno, ERR_SYNTAX, "to less arguments for valid fixes", -1))
                    
                    if not self.isNode(args[0]):
                        errors.append(Error(lineno, ERR_SYNTAX, "node %s for fixes does not exist!" % (args[0]), -1, SEC_FATAL))
                    self.sections[section][len(self.sections[section])] = args
                except Exception, e:
                    print str(e)
                    traceback.print_exc()
                    errors.append(Error(lineno, ERR_SYNTAX, "generic fixes line error", -1, SEC_FATAL))
            #////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////		
            elif section == 'minimass':
                try:
                    if len(args) < 1:
                        errors.append(Error(lineno, ERR_SYNTAX, "to less arguments for valid minimass", -1))
                    
                    if not self.isFloat(args[0]):
                        errors.append(Error(lineno, ERR_SYNTAX, "minimass %s is not a valid float value!" % (args[0]), -1, SEC_FATAL))
                    self.sections[section][len(self.sections[section])] = args
                except Exception, e:
                    print str(e)
                    traceback.print_exc()
                    errors.append(Error(lineno, ERR_SYNTAX, "generic fixes line error", -1, SEC_FATAL))
            #////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////		
            elif section == 'ties':
                try:
                    if len(args) < 5:
                        errors.append(Error(lineno, ERR_SYNTAX, "to less arguments for valid ties", -1))
                    
                    if not self.isNode(args[0]):
                        errors.append(Error(lineno, ERR_SYNTAX, "node %s for ties does not exist!" % (args[0]), -1, SEC_FATAL))
                    
                    for i in range(1, 4):
                        if not self.isFloat(args[i]):
                            errors.append(Error(lineno, ERR_SYNTAX, "ties argument %d = '%s' is not a valid float value!" % (i, args[i]), -1, SEC_FATAL))
                
                    self.sections[section][len(self.sections[section])] = args
                except Exception, e:
                    print str(e)
                    traceback.print_exc()
                    errors.append(Error(lineno, ERR_SYNTAX, "generic ties line error", -1, SEC_FATAL))
            #////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////		
            elif section == 'ropables':
                try:
                    if len(args) < 1:
                        errors.append(Error(lineno, ERR_SYNTAX, "to less arguments for valid ropables", -1))
                    
                    if not self.isNode(args[0]):
                        errors.append(Error(lineno, ERR_SYNTAX, "node %s for ropables does not exist!" % (args[0]), -1, SEC_FATAL))
                    self.sections[section][len(self.sections[section])] = args
                except Exception, e:
                    print str(e)
                    traceback.print_exc()
                    errors.append(Error(lineno, ERR_SYNTAX, "generic ropables line error", -1, SEC_FATAL))
            #////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////		
            elif section == 'particles':
                try:
                    if len(args) < 3:
                        errors.append(Error(lineno, ERR_SYNTAX, "to less arguments for valid particles", -1))
                    
                    for i in range(0, 1):
                        if not self.isNode(args[i]):
                            errors.append(Error(lineno, ERR_SYNTAX, "node %s for particles does not exist!" % (args[i]), -1, SEC_FATAL))
                    self.sections[section][len(self.sections[section])] = args
                except Exception, e:
                    print str(e)
                    traceback.print_exc()
                    errors.append(Error(lineno, ERR_SYNTAX, "generic particles line error", -1, SEC_FATAL))
            #////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////		
            elif section == 'rigidifiers':
                try:
                    if len(args) < 5:
                        errors.append(Error(lineno, ERR_SYNTAX, "to less arguments for valid rigidifiers", -1))
                    
                    for i in range(0, 2):
                        if not self.isNode(args[i]):
                            errors.append(Error(lineno, ERR_SYNTAX, "node %s for rigidifiers does not exist!" % (args[i]), -1, SEC_FATAL))

                    for i in range(3, 4):
                        if not self.isFloat(args[i]):
                            errors.append(Error(lineno, ERR_SYNTAX, "%s for rigidifiers is not a valid float!" % (args[i]), -1, SEC_FATAL))
                    self.sections[section][len(self.sections[section])] = args
                except Exception, e:
                    print str(e)
                    traceback.print_exc()
                    errors.append(Error(lineno, ERR_SYNTAX, "generic rigidifiers line error", -1, SEC_FATAL))
            #////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////		
            elif section == 'flares':
                try:
                    if len(args) < 5:
                        errors.append(Error(lineno, ERR_SYNTAX, "to less arguments for valid flares", -1))
                    
                    for i in range(0, 2):
                        if not self.isNode(args[i]):
                            errors.append(Error(lineno, ERR_SYNTAX, "node %s for flares does not exist!" % (args[i]), -1, SEC_FATAL))

                    for i in range(3, 4):
                        if not self.isFloat(args[i]):
                            errors.append(Error(lineno, ERR_SYNTAX, "%s for flares is not a valid float!" % (args[i]), -1, SEC_FATAL))
                    
                    if len(args) > 5:
                        if args[5].strip() not in ['f','b','l','r','R','u']:
                            errors.append(Error(lineno, ERR_SYNTAX, "invalid type for flares: %s" % (args[5]), -1, SEC_FATAL))
                        
                    if len(args) > 6:
                        if not self.isInt(args[6]):
                            errors.append(Error(lineno, ERR_SYNTAX, "invalid controlnumber for flares: %s" % (args[6]), -1, SEC_FATAL))
                        
                    if len(args) > 8:
                        for i in range(7, 8):
                            if not self.isFloat(args[i]):
                                errors.append(Error(lineno, ERR_SYNTAX, "%s for flares is not a valid float!" % (args[i]), -1, SEC_FATAL))
                            
                    if len(args) > 9:
                        if args[9].strip() != 'default':
                            materialdependencies.append(args[9].strip())

                    self.sections[section][len(self.sections[section])] = args
                except Exception, e:
                    print str(e)
                    traceback.print_exc()
                    errors.append(Error(lineno, ERR_SYNTAX, "generic flares line error", -1, SEC_FATAL))
            #////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////		
            elif section == 'props':
                try:
                    if len(args) < 10:
                        errors.append(Error(lineno, ERR_SYNTAX, "to less arguments for valid props", -1))
                    
                    for i in range(0, 2):
                        if not self.isNode(args[i]):
                            errors.append(Error(lineno, ERR_SYNTAX, "node %s for props does not exist!" % (args[i]), -1, SEC_FATAL))

                    for i in range(3, 8):
                        if not self.isFloat(args[i]):
                            errors.append(Error(lineno, ERR_SYNTAX, "%s for props is not a valid float!" % (args[i]), -1, SEC_FATAL))
                        
                    materialdependencies.append(args[9].strip())

                    self.sections[section][len(self.sections[section])] = args
                except Exception, e:
                    print str(e)
                    traceback.print_exc()
                    errors.append(Error(lineno, ERR_SYNTAX, "generic props line error", -1, SEC_FATAL))
            #////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////		
            elif section == 'flexbodies':
                try:
                    if line.startswith('forset'):
                        #ignore foresets for now
                        continue
                    if len(args) < 10:
                        errors.append(Error(lineno, ERR_SYNTAX, "to less arguments for valid flexbodies", -1))
                    
                    for i in range(0, 2):
                        if not self.isNode(args[i]):
                            errors.append(Error(lineno, ERR_SYNTAX, "node %s for flexbodies does not exist!" % (args[i]), -1, SEC_FATAL))

                    for i in range(3, 8):
                        if not self.isFloat(args[i]):
                            errors.append(Error(lineno, ERR_SYNTAX, "%s for flexbodies is not a valid float!" % (args[i]), -1, SEC_FATAL))
                        
                    materialdependencies.append(args[9].strip())

                    self.sections[section][len(self.sections[section])] = args
                except Exception, e:
                    print str(e)
                    traceback.print_exc()
                    errors.append(Error(lineno, ERR_SYNTAX, "generic flexbodies line error", -1, SEC_FATAL))
            #////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////		
            elif section == 'texcoords':
                try:
                    if len(args) < 3:
                        errors.append(Error(lineno, ERR_SYNTAX, "to less arguments for valid submesh / texcoords", -1))
                    
                    if not self.isNode(args[0]):
                        errors.append(Error(lineno, ERR_SYNTAX, "node %s for submesh / texcoords does not exist!" % (args[0]), -1, SEC_FATAL))

                    for i in range(1, 2):
                        if not self.isFloat(args[i]) or float(args[i]) < 0 or float(args[i]) > 1:
                            errors.append(Error(lineno, ERR_SYNTAX, "%s for submesh / texcoords is not a valid float UV coordinate between 0 and 1!" % (args[i]), -1, SEC_FATAL))
                        
                    if not submeshcounter in self.sections[section].keys():
                        self.sections[section][submeshcounter] = []
                    self.sections[section][submeshcounter].append(args)

                except Exception, e:
                    print str(e)
                    traceback.print_exc()
                    errors.append(Error(lineno, ERR_SYNTAX, "generic submesh / texcoords line error", -1, SEC_FATAL))
            #////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////		
            elif section == 'cab':
                try:
                    if len(args) < 3:
                        errors.append(Error(lineno, ERR_SYNTAX, "to less arguments for valid submesh / cab", -1))
                    
                    for i in range(0, 2):
                        if not self.isNode(args[i]):
                            errors.append(Error(lineno, ERR_SYNTAX, "node %s for submesh / cab does not exist!" % (args[i]), -1, SEC_FATAL))
                    
                    if len(args) > 3:
                        if not args[3] in ['c','D','b','n']:
                            errors.append(Error(lineno, ERR_SYNTAX, "invalid option '%s' for submesh / cab!" % (args[3]), -1, SEC_FATAL))
                    
                    if not submeshcounter in self.sections[section].keys():
                        self.sections[section][submeshcounter] = []
                    self.sections[section][submeshcounter].append(args)
                    
                except Exception, e:
                    print str(e)
                    traceback.print_exc()
                    errors.append(Error(lineno, ERR_SYNTAX, "generic submesh / cab line error", -1, SEC_FATAL))
            #////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////		
            elif section == 'exhausts':
                try:
                    if len(args) < 4:
                        errors.append(Error(lineno, ERR_SYNTAX, "to less arguments for valid exhausts", -1))
                    
                    for i in range(0, 1):
                        if not self.isNode(args[i]):
                            errors.append(Error(lineno, ERR_SYNTAX, "node %s for exhausts does not exist!" % (args[i]), -1, SEC_FATAL))

                    if not self.isFloat(args[2]):
                        errors.append(Error(lineno, ERR_SYNTAX, "factor argument %s for exhausts is not a Float!" % (args[2]), -1, SEC_FATAL))
                    
                    matname = args[3].strip()
                    if matname != 'default':
                        materialdependencies.append(matname)

                    self.sections[section][len(self.sections[section])] = args
                except Exception, e:
                    print str(e)
                    traceback.print_exc()
                    errors.append(Error(lineno, ERR_SYNTAX, "generic exhausts line error", -1, SEC_FATAL))
            #////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////		
            elif section == 'wings':
                try:
                    if len(args) < 21:
                        errors.append(Error(lineno, ERR_SYNTAX, "to less arguments for valid wings", -1))
                    
                    for i in range(0, 7):
                        if not self.isNode(args[i]):
                            errors.append(Error(lineno, ERR_SYNTAX, "node %s for wings does not exist!" % (args[i]), -1, SEC_FATAL))

                    for i in range(8, 15):
                        if not self.isFloat(args[i]):
                            errors.append(Error(lineno, ERR_SYNTAX, "argument %s for wings is not a Float!" % (args[i]), -1, SEC_FATAL))
                    
                    if not args[16] in ['n','a','b','f','e','r','S','T']:
                        errors.append(Error(lineno, ERR_SYNTAX, "invalid wings option %s for wings!" % (args[16]), -1, SEC_FATAL))

                    for i in range(17, 19):
                        if not self.isFloat(args[i]):
                            errors.append(Error(lineno, ERR_SYNTAX, "argument %s for wings is not a Float!" % (args[i]), -1, SEC_FATAL))
                    
                    afldependencies.append(args[19].strip())

                    self.sections[section][len(self.sections[section])] = args
                except Exception, e:
                    print str(e)
                    traceback.print_exc()
                    errors.append(Error(lineno, ERR_SYNTAX, "generic wings line error", -1, SEC_FATAL))
            #////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////		
            elif section == 'airbrakes':
                try:
                    if len(args) < 14:
                        errors.append(Error(lineno, ERR_SYNTAX, "to less arguments for valid airbrakes", -1))
                    
                    for i in range(0, 3):
                        if not self.isNode(args[i]):
                            errors.append(Error(lineno, ERR_SYNTAX, "node %s for airbrakes does not exist!" % (args[i]), -1, SEC_FATAL))

                    for i in range(4, 13):
                        if not self.isFloat(args[i]):
                            errors.append(Error(lineno, ERR_SYNTAX, "argument %s for airbrakes is not a Float!" % (args[i]), -1, SEC_FATAL))

                    self.sections[section][len(self.sections[section])] = args
                except Exception, e:
                    print str(e)
                    traceback.print_exc()
                    errors.append(Error(lineno, ERR_SYNTAX, "generic airbrakes line error", -1, SEC_FATAL))
            #////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////		
            elif section == 'turboprops':
                try:
                    if len(args) < 8:
                        errors.append(Error(lineno, ERR_SYNTAX, "to less arguments for valid turboprops", -1))
                    
                    for i in range(0, 5):
                        if not self.isNode(args[i]):
                            errors.append(Error(lineno, ERR_SYNTAX, "node %s for turboprops does not exist!" % (args[i]), -1, SEC_FATAL))

                    if not self.isFloat(args[6]):
                            errors.append(Error(lineno, ERR_SYNTAX, "argument %s for turboprops is not a Float!" % (args[6]), -1, SEC_FATAL))

                    afldependencies.append(args[7].strip())
                    
                    self.sections[section][len(self.sections[section])] = args
                except Exception, e:
                    print str(e)
                    traceback.print_exc()
                    errors.append(Error(lineno, ERR_SYNTAX, "generic turboprops line error", -1, SEC_FATAL))
            #////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////		
            elif section == 'fusedrag':
                try:
                    if len(args) < 4:
                        errors.append(Error(lineno, ERR_SYNTAX, "to less arguments for valid fusedrag", -1))
                    
                    for i in range(0, 1):
                        if not self.isNode(args[i]):
                            errors.append(Error(lineno, ERR_SYNTAX, "node %s for fusedrag does not exist!" % (args[i]), -1, SEC_FATAL))

                    if not self.isFloat(args[2]):
                            errors.append(Error(lineno, ERR_SYNTAX, "3rd argument %s for fusedrag is not a Float!" % (args[2]), -1, SEC_FATAL))

                    afldependencies.append(args[3].strip())
                    
                    self.sections[section][len(self.sections[section])] = args
                except Exception, e:
                    print str(e)
                    traceback.print_exc()
                    errors.append(Error(lineno, ERR_SYNTAX, "generic fusedrag line error", -1, SEC_FATAL))
            #////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////		
            elif section == 'turboprops2':
                try:
                    if len(args) < 9:
                        errors.append(Error(lineno, ERR_SYNTAX, "to less arguments for valid turboprops2", -1))
                    
                    for i in range(0, 6):
                        if not self.isNode(args[i]):
                            errors.append(Error(lineno, ERR_SYNTAX, "node %s for turboprops2 does not exist!" % (args[i]), -1, SEC_FATAL))

                    if not self.isFloat(args[7]):
                            errors.append(Error(lineno, ERR_SYNTAX, "argument %s for turboprops2 is not a Float!" % (args[7]), -1, SEC_FATAL))

                    afldependencies.append(args[8].strip())
                    
                    self.sections[section][len(self.sections[section])] = args
                except Exception, e:
                    print str(e)
                    traceback.print_exc()
                    errors.append(Error(lineno, ERR_SYNTAX, "generic turboprops2 line error", -1, SEC_FATAL))
            #////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////		
            elif section == 'pistonprops':
                try:
                    if len(args) < 10:
                        errors.append(Error(lineno, ERR_SYNTAX, "to less arguments for valid pistonprops", -1))
                    
                    for i in range(0, 6):
                        if not self.isNode(args[i]):
                            errors.append(Error(lineno, ERR_SYNTAX, "node %s for pistonprops does not exist!" % (args[i]), -1, SEC_FATAL))

                    for i in range(7, 8):
                        if not self.isFloat(args[i]):
                            errors.append(Error(lineno, ERR_SYNTAX, "argument %s for pistonprops is not a Float!" % (args[i]), -1, SEC_FATAL))

                    afldependencies.append(args[9].strip())
                    
                    self.sections[section][len(self.sections[section])] = args
                except Exception, e:
                    print str(e)
                    traceback.print_exc()
                    errors.append(Error(lineno, ERR_SYNTAX, "generic pistonprops line error", -1, SEC_FATAL))
            #////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////		
            elif section == 'turbojets':
                try:
                    if len(args) < 9:
                        errors.append(Error(lineno, ERR_SYNTAX, "to less arguments for valid turbojets", -1))
                    
                    for i in range(0, 2):
                        if not self.isNode(args[i]):
                            errors.append(Error(lineno, ERR_SYNTAX, "node %s for turbojets does not exist!" % (args[i]), -1, SEC_FATAL))

                    if not self.isInt(args[3]) or not int(args[3]) in [0,1]:
                        errors.append(Error(lineno, ERR_SYNTAX, "reversable argument must be either 0 or 1 in turbojets", -1, SEC_FATAL))
                            
                    for i in range(4, 8):
                        if not self.isFloat(args[i]):
                            errors.append(Error(lineno, ERR_SYNTAX, "argument %s for turbojets is not a Float!" % (args[i]), -1, SEC_FATAL))

                    self.sections[section][len(self.sections[section])] = args
                except Exception, e:
                    print str(e)
                    traceback.print_exc()
                    errors.append(Error(lineno, ERR_SYNTAX, "generic turbojets line error", -1, SEC_FATAL))
            #////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////		
            elif section == 'screwprops':
                try:
                    if len(args) < 4:
                        errors.append(Error(lineno, ERR_SYNTAX, "to less arguments for valid screwprops", -1))
                    
                    for i in range(0, 2):
                        if not self.isNode(args[i]):
                            errors.append(Error(lineno, ERR_SYNTAX, "node %s for screwprops does not exist!" % (args[i]), -1, SEC_FATAL))

                    if not self.isFloat(args[3]):
                        errors.append(Error(lineno, ERR_SYNTAX, "argument %s for screwprops is not a Float!" % (args[3]), -1, SEC_FATAL))

                    self.sections[section][len(self.sections[section])] = args
                except Exception, e:
                    print str(e)
                    traceback.print_exc()
                    errors.append(Error(lineno, ERR_SYNTAX, "generic screwprops line error", -1, SEC_FATAL))
            #////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////		
            elif section == 'end':
                try:
                    if line.strip() != '':
                        errors.append(Error(lineno, ERR_SYNTAX, "there is text behind the end" % (args[i]), -1, SEC_WARN))
                    self.sections[section][len(self.sections[section])] = args
                except Exception, e:
                    print str(e)
                    traceback.print_exc()
                    errors.append(Error(lineno, ERR_SYNTAX, "generic end line error", -1, SEC_FATAL))
            #////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////		
            
        # parse cameras later, like RoR does. This allows to specify the camera section before the nodes section ...
        for lineno in cameralines.keys():
            args = cameralines[lineno]
            #print lineno, args
            try:
                if len(args) < 3:
                    errors.append(Error(lineno, ERR_SYNTAX, "to less arguments for valid camera", -1))
                for i in range(0,3):
                    if not self.isNode(args[i]):
                        errors.append(Error(lineno, ERR_SYNTAX, "node %s for camera does not exist!" % (args[i]), -1, SEC_FATAL))
                
                if int(args[0]) == int(args[1]) or int(args[0]) == int(args[2]) or int(args[1]) == int(args[2]):
                    errors.append(Error(lineno, ERR_SYNTAX, "camera between same nodes is invalid", -1, SEC_WARN))
                    self.sections[section][len(self.sections[section])] = args
            except Exception, e:
                print str(e)
                traceback.print_exc()
                errors.append(Error(lineno, ERR_SYNTAX, "generic camera line error", -1, SEC_FATAL))
        
        
        #for section in self.sections.keys():
        #	print "%20s: %d" % (section, len(self.sections[section]))
        
        self.log("syntax checking done, checking for higher errors...")
                
        if not self.isFilledSection('help'):
            errors.append(Error(-1, ERR_HIGH, "please provide a help section" , 0, SEC_WARN))
        
        if not self.isFilledSection('globals'):
            errors.append(Error(-1, ERR_HIGH, "a globals section is required" , -10, SEC_FATAL))

        if not self.isFilledSection('nodes'):
            errors.append(Error(-1, ERR_HIGH, "a nodes section is required" , -10, SEC_FATAL))
        
        if not self.isFilledSection('beams'):
            errors.append(Error(-1, ERR_HIGH, "a nodes beams is required" , -10, SEC_FATAL))
        
        if not self.isFilledSection('end'):
            errors.append(Error(-1, ERR_HIGH, "an end mark is required. Please add 'end' at the end :)" , -10, SEC_FATAL))
        
        
        self.log("checking for correct section order...")
        end_cond = 0
        for i in range(0, len(sectionorder)):
            section = sectionorder[i]
            if section in ['globals', 'nodes', 'beams']:
                end_cond += 1
                if end_cond > 2:
                    break
            if section in ['cameras', 'cinecam', 'wheels', 'wheels2', 'meshwheels', 'shocks', 'hydros', 'commands', 'commands2', 'rotators', 'contacters', 'ropes', 'fixes', 'ties', 'ropables', 'particles', 'rigidifiers', 'flares', 'props', 'flexbodies', 'submesh', 'cab' ,'texcoords', 'exhausts', 'wings', 'airbrakes', 'turboprops', 'fusedrag', 'turboprops2', 'pistonprops', 'turbojets', 'screwprops']:
                sev = SEC_FATAL
                if section == 'cameras':
                    sev = SEC_WARN
                errors.append(Error(-1, ERR_HIGH, "move the section '%s' behind the globals, nodes and beams section!"%section , -1, sev))
            
        
        self.log("checking for too short beams...")
        if 'beams' in self.sections.keys():
            for beamk in self.sections['beams'].keys():
                beam = self.sections['beams'][beamk]
                nodek1 = beam[0]
                nodek2 = beam[1]
                    
                try:
                    beam = (nodes[nodek1].x-nodes[nodek2].x, nodes[nodek1].y-nodes[nodek2].y, nodes[nodek1].z-nodes[nodek2].z)
                    beamlength = math.sqrt(beam[0] * beam[0] + beam[1] * beam[1] + beam[2] * beam[2])
                    if beamlength < 0.01:
                        errors.append(Error(-1, ERR_HIGH, "beam %d between node %d and node %d is too short (under 0.01 meter)" % (beamk, nodek1, nodek2), -2, SEC_FATAL))
                except:
                    pass

            self.log("checking for duplicate beams...")
            dupes = []
            beams = self.sections['beams']
            for beamk1 in beams.keys():
                for beamk2 in beams.keys():
                    if beamk1 == beamk2:
                        continue
                    try:
                        if beams[beamk1][0] == beams[beamk2][0] and beams[beamk1][1] == beams[beamk2][1]:
                            if not beamk1 in dupes and not beamk1 in dupes:
                                errors.append(Error(-1, ERR_HIGH, "beam %d and beam %d are duplicate between nodes (%d,%d)->(%d,%d)" % (beamk1, beamk2, beams[beamk1][0],beams[beamk1][1], beams[beamk2][0], beams[beamk2][1]), -0.2, SEC_WARN, True))
                                insertlines[beamlines[beamk1]] = ';XXX FIX beam below (%d) was duplicate of %d and nodes (%d,%d)->(%d,%d)' % (beamk1, beamk2, beams[beamk1][0],beams[beamk1][1], beams[beamk2][0], beams[beamk2][1])
                                
                                self.correctedlines[beamlines[beamk1]] = ";" + self.correctedlines[beamlines[beamk1]]
                                dupes.append(beamk1)
                                dupes.append(beamk2)
                        if beams[beamk1][0] == beams[beamk2][1] and beams[beamk1][1] == beams[beamk2][0]:
                            if not beamk1 in dupes and not beamk1 in dupes:
                                errors.append(Error(-1, ERR_HIGH, "beam %d and beam %d are inverse to each other between node (%d,%d)->(%d,%d)" % (beamk1, beamk2, beams[beamk1][0],beams[beamk1][1], beams[beamk2][0], beams[beamk2][1]), -0.2, SEC_WARN, True))
                                insertlines[beamlines[beamk1]] = ';XXX FIX beam below (%d) was inverse duplicate to beam %d and nodes (%d,%d)->(%d,%d)' % (beamk1, beamk2, beams[beamk1][0],beams[beamk1][1], beams[beamk2][0], beams[beamk2][1])
                                
                                self.correctedlines[beamlines[beamk1]] = ";" + self.correctedlines[beamlines[beamk1]]
                                dupes.append(beamk1)
                                dupes.append(beamk2)
                    except:
                        pass

        if 'nodes' in self.sections.keys():
            try:
                self.log("checking for loose nodes...")
                lnodes = copy.copy(self.sections['nodes'])
                
                if 'beams' in self.sections.keys():
                    for beamk in self.sections['beams'].keys():
                        beam = self.sections['beams'][beamk]
                        nodek1 = int(beam[0])
                        nodek2 = int(beam[1])
                        if nodek1 in lnodes.keys():
                            del lnodes[nodek1]
                        if nodek2 in lnodes.keys():
                            del lnodes[nodek2]
                
                if 'shocks' in self.sections.keys():
                    for shockk in self.sections['shocks'].keys():
                        shock = self.sections['shocks'][shockk]
                        nodek1 = int(shock[0])
                        nodek2 = int(shock[1])
                        if nodek1 in lnodes.keys():
                            del lnodes[nodek1]
                        if nodek2 in lnodes.keys():
                            del lnodes[nodek2]
                
                if 'commands' in self.sections.keys():
                    for commandk in self.sections['commands'].keys():
                        command = self.sections['commands'][commandk]
                        nodek1 = int(command[0])
                        nodek2 = int(command[1])
                        if nodek1 in lnodes.keys():
                            del lnodes[nodek1]
                        if nodek2 in lnodes.keys():
                            del lnodes[nodek2]

                if len(lnodes) > 0:
                    for nodek in lnodes:
                        errors.append(Error(-1, ERR_HIGH, "node %d is not attached to any beam" % (nodek), -2))
            except Exception, e:
                print str(e)
                traceback.print_exc()
                errors.append(Error(lineno, ERR_SYNTAX, "loose node check error", -1, SEC_FATAL))
                
        self.log("checking done!")
        
        self.log("adding comments to corrected file...")
        for i in range(len(lines), 0, -1):
            if i in insertlines.keys():
                self.correctedlines.insert(i, insertlines[i])
    
        self.log("all done!")
        self.errors = errors
        self.materialdependencies = materialdependencies

def readFile(filename):
    file = open(filename,'r')
    content = file.readlines()
    file.close()
    return content

def writeFile(filename, lines):
    file = open(filename,'w')
    file.writelines(lines)
    file.close()


def checkDirectory(dname):
    log = []
    if os.path.isdir(dname):
        files = []
        for file in os.listdir(dname):
            if file.endswith('.truck') or file.endswith('.car') or file.endswith('.trailer') or file.endswith('.load') or file.endswith('.airplane') or file.endswith('.boat') or file.endswith('.fixed'):
                files.append(os.path.join(dname, file))

        for f in files:
            log.append('### '+f+'\n')
            lines = readFile(f)
            r = RoRTester(lines, f)
            for e in r.errors:
                if e.line >= 0:
                    log.append("%d| %s | %s\n" % (e.line, lines[e.line].strip(), e.msg))
                else:
                    log.append("%s\n" % (e.msg))
            log.append('*'*80+'\n')
        
        f = open(os.path.join(dname, "checklog.txt"), "w")
        f.writelines(log)
        f.close()
    return len(files)


def main():
    log = []
    for argno in range(1,len(sys.argv)):
        if os.path.isdir(sys.argv[argno]):
            checkDirectory(sys.argv[argno])
        elif os.path.isfile(sys.argv[argno]):
            log.append('### '+sys.argv[argno]+'\n')
            lines = readFile(sys.argv[argno])
            r = RoRTester(lines, sys.argv[argno])
            for e in r.errors:
                if e.line >= 0:
                    log.append("%d| %s | %s\n" % (e.line, lines[e.line].strip(), e.msg))
                else:
                    log.append("%s\n" % (e.msg))
            log.append('*'*80+'\n')
            
    f = open(os.path.join(dname, "checklog.txt"), "w")
    f.writelines(log)
    f.close()
    
    
class HTMLRoRTester(RoRTester):
    def __init__(self):
        pass
    
    def run(self, lines, filename):
        #print "worker started"
        RoRTester.__init__(self, lines, filename)
        result = ""
        corrected = 0
        points = 0
        points_corr = 0
        try:
            severvity = ['WARN', 'FATAL', 'PERFROMANCE']
            types = ['SYNTAX', 'HIGH']
            rows=[]
            for e in self.errors:
                row = "<tr>\n"
                row += " <td>%s</td>\n" % (types[e.type])
                if e.line >= 0:
                    row += " <td>%d</td>\n" % (int(e.line)+1)
                    row += " <td>%s</td>\n" % (lines[e.line])
                    row += " <td>%s</td>\n" % (e.msg)
                else:
                    row += " <td colspan='3'>%s</td>\n" % (e.msg)
                
                row += " <td>%s</td>\n" % (severvity[e.severity])
                row += " <td>%.1f</td>\n" % (e.points);
                points+=e.points
                if e.corrected:
                    row += " <td>yes</td>\n"
                    corrected+=1
                    points_corr+=e.points
                else:
                    row += " <td>no</td>\n"
                row += "</tr>\n";
                rows.append(row)
            
            if len(self.errors)>0:
                result += "<h1>%d problem(s) found</h1>"%len(self.errors)
                result += "<table border='1' style='border-collapse:collapse;'><tr><td>Type</td><td>LineNo</td><td>Line</td><td>Error</td><td>Severity</td><td>Points</td><td>Corrected</td></tr>"
                result += '\n'.join(rows)
                result += "</table>"
                if corrected > 0:
                    result += "<h3>Your Score: %0.1f (%0.1f with corrected version)</h3>" % (100+points, 100+points-points_corr)
                else:
                    result += "<h3>Your Score: %0.1f</h3>" % (100+points)
            else:
                result += "<h1>No problems found, congratulations! :)</h1>"
            
            result += "<h3>compatible with RoR %s</h3>" % self.minimumversion
                
        except Exception, e:
            result += "error:<pre>"
            result += str(e)+"\n\n"
            result += traceback.format_exc()
            result += "</pre>"
            
        fixed_lines = []
        for line in self.correctedlines:
            fixed_lines.append(line.strip()+'\n')
        
        self.correctederrors = corrected
        
        if len(self.errors)>0 and corrected > 0:
            result += "<p><h3>Semi-Corrected file (%d/%d problems corrected)</h3><textarea cols='120' rows='20'>%s</textarea>" % (corrected, len(self.errors), ''.join(fixed_lines))
            result += "<p><h3>Log</h3><pre>%s</pre>" % ('\n'.join(self.loglines))

        return fixed_lines, result
    
if __name__ == '__main__':
    main()
