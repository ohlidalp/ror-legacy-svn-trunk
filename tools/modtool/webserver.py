import sys, string,cgi,time,traceback, threading, SocketServer, BaseHTTPServer, zipfile
from rortester import *
from uniquifier import *

VERSION = '0.1.5, 27th May 2008'
CHANGELOG = """
 0.1.0, 25th May 2008 : initial release
 0.1.1, 25th May 2008 : added options, added mesh upgrade and optimization
 0.1.2, 25th May 2008 : added ability to remove add or ignore UID's, added verbosity, improved HTML output, fixed output filename and zip compression
 0.1.3, 26th May 2008 : added unused and missing materials, fixed some bugs with materials.
 0.1.4, 26th May 2008 : found and fixed major problems, working with all original trucks perfectly
 0.1.5, 27th May 2008 : fixed some bugs, added preview pictures
 """

def uniqueTask(autouid):
	if autouid == 1:
		# real unique ID
		uid = Uniquifier.getUniqueID()
	else:
		# faked one to prevent existing directories
		uid = int(time.time()).__str__()

	ipath = os.path.join("input", uid)
	opath = os.path.join("output", uid)
	os.mkdir(ipath)
	os.mkdir(opath)
	return (uid, ipath, opath)

class worker(threading.Thread):
	def __init__(self, evdone, evdestroy, ipath, opath, wfile, options):
		self.opath = opath
		self.ipath = ipath
		self.wfile = wfile
		self.evdone=evdone
		self.evdestroy=evdestroy
		
		self.options = options
		threading.Thread.__init__(self)
		
	def run(self):
		u = Uniquifier(self.ipath, self.opath, self.wfile, self.options)
		self.uid = u.uid

		#print "worker done"
		self.evdone.set()
		#print "worker waiting"
		self.evdestroy.wait()
		#print "worker destroyed"


class MyHandler(BaseHTTPServer.BaseHTTPRequestHandler):
	def do_GET(self):
		#print self.path
		#print self.path
		if self.path == "/":
			content= """
<HTML><BODY>

<h1>RoR Mod Tool </h1>
version (%s)
<p>
Select and upload a .zip file which contains all required files for your mod:
<form method='POST' enctype='multipart/form-data' action=''>
<input type='file' name='upfile'><input type='submit' value='upload'>

<p/>
<table border="0">
<tr><td colspan="2"><h3>Options</h3></td></tr>

<tr><td>Unique ID Mode:</td><td>
	<select name="autouid">
	 <option value="0">Do not change Resources</option>
	 <option value="1" selected>Add unique ID</option>
	 <option value="2">Remove all unique ID</option>
	</select></td>
</tr>

<tr><td>Target Version:</td><td>
	<select name="ddeffile">
	 <option value="ror033.ddef">0.33</option>
	 <option value="ror034.ddef">0.34</option>
	 <option value="ror035.ddef" selected>0.35</option>
	 <option value="nodep">No Dependencies</option>
	</select></td>
</tr>

<tr><td>&nbsp;</td><td><label><input type="checkbox" name="checktr" checked>Check and Correct Trucks if possible</label></td></tr>
<tr><td>&nbsp;</td><td><label><input type="checkbox" name="upgrademesh" checked>Upgrade Meshes</label></td></tr>
<tr><td>&nbsp;</td><td><label><input type="checkbox" name="optimizemesh" checked>Optimize Meshes</label></td></tr>
<tr><td>&nbsp;</td><td><label><input disabled type="checkbox" name="checkrating">Create rating Overview</label></td></tr>
<tr><td>&nbsp;</td><td><label><input type="checkbox" name="meshinfo">Print out Mesh Info (use at least verbosity=1!)</label></td></tr>

<tr><td>Generate Truck Preview Images:</td><td>
	<select name="truckimage">
	 <option value="0">None</option>
	 <option value="1" selected>Simple</option>
	 <option value="2">Full</option>
	</select></td>
</tr>

<tr><td>Verbosity:</td><td>
	<select name="verbosity">
	 <option value="0" selected>0: Least output, mostly quiet</option>
	 <option value="1">1: Less output, some more outputs</option>
	 <option value="2">2: Full debug output</option>
	</select></td>
</tr>

<tr><td>Line break style:</td><td>
	<select name="lineend">
	 <option value="win" selected>Windows (CR+LF)</option>
	 <option value="linux">Linux (LF)</option>
	 <option value="mac">Mac (CR)</option>
	</select></td>
</tr>

</table>
</form>

<h3>Notes / Comments</h3>
- Tip: you can also use the Truck Checker separately <a href="http://rigsofrods.com:8081/">here</a>.
<p/>
- This is alpha status software, expect errors and <b>please report Bugs <a href='http://forum.rigsofrods.com/index.php?topic=11094.msg90470#msg90470'>here</a></b>

<h3>changelog:</h3>
<pre>%s</pre>
</BODY>
</HTML>
			""" % (VERSION, CHANGELOG)
			self.send_response(200)
			self.send_header('Content-type', 'text/html')
			self.end_headers()
			self.wfile.write(content)
		if self.path.startswith('/output/'):
			base, filename = os.path.split(self.path)
			
			fpath = self.path
			if sys.platform.startswith('linux'):
				fpath = fpath.lstrip('/')
			else:
				fpath = fpath.replace('/', '\\')
				fpath = fpath.lstrip('\\')
			
			if not os.path.isfile(fpath):
				self.send_response(404)
				self.end_headers()
				self.wfile.write("404 - file not found!")
				return
			file = open(fpath, 'rb')
			content = file.read()
			file.close()
			
			self.send_response(200)
			self.send_header('Content-Length', len(content))
			self.send_header('Content-type', 'application/octet-stream')
			self.send_header('Content-Disposition', filename)
			self.end_headers()
			self.wfile.write(content)

	def do_POST(self):
		try:
			# Parse the form data posted
			form = cgi.FieldStorage(
				fp=self.rfile,
				headers=self.headers,
				environ={'REQUEST_METHOD':'POST',
						 'CONTENT_TYPE':self.headers['Content-Type'],
						 })	
			
			#print form
			
			# Echo back information about what was posted in the form
			#for field in form.keys():
			#	print field, form[field]
			
			#ctype, pdict = cgi.parse_header(self.headers.getheader('content-type'))
			#if ctype == 'multipart/form-data':
			#	query=cgi.parse_multipart(self.rfile, pdict)
			
			self.send_response(200)
			self.send_header('Content-type', 'text/html')
			self.end_headers()
			
			upfilecontent = form['upfile'].file.read()
					
			options = UTaskOptions()
			checktr = ('checktr' in form.keys())
			checkrating = ('checkrating' in form.keys())
			upmesh = ('upgrademesh' in form.keys())
			opmesh = ('optimizemesh' in form.keys())
			meshinfo = ('meshinfo' in form.keys())
			upmat = False #('upgrademat' in form.keys())

			generatetruckimage = 1
			if 'truckimage' in form.keys():
				generatetruckimage = int(form['truckimage'].value)
			
			ddeffile = 'nodep'
			if 'ddeffile' in form.keys():
				ddeffile = form['ddeffile'].value
			autouid = int(form['autouid'].value)
			verbosity = int(form['verbosity'].value)
			
			lineend='win'
			if 'lineend' in form.keys():
				lineend = (form['lineend'].value)
			
			self.wfile.write("<html><body>got the file, processing (this can take several seconds)...<p>")
			self.wfile.flush()
			
			# create unique directories
			uid, ipath, opath = uniqueTask(autouid)
			
			ufilename = "upload.zip"
			if 'upfile' in form.keys():
				ufilename = form['upfile'].filename
			#print ufilename
			
			ufilename = Uniquifier.stripUniqueID(ufilename)
			ufilename = Uniquifier.addUniqueID(uid, ufilename)
			
			root, ext = os.path.splitext(ufilename)
			if ext.lower() != '.zip':
				self.wfile.write("only .zip Archives allowed. Please go back and try again.")
				self.wfile.flush()
				return
				

			# save uploaded file
			dstfile = os.path.join(ipath, ufilename)
			#print dstfile
			fo = open(dstfile, "wb")
			fo.write(upfilecontent)
			fo.close()
			
			#try to unpack uploaded file
			#print dstfile
			if not zipfile.is_zipfile(dstfile):
				self.wfile.write("error: uploaded file is no valid zip file!<p>")
				return
			zfile = zipfile.ZipFile(dstfile, "r")
			#zfile.printdir()
			self.wfile.write("- unzipping ...<br/>")
			self.wfile.flush()
			for info in zfile.infolist():
				fname = info.filename
				# decompress each file's data
				data = zfile.read(fname)
				filename = os.path.join(ipath, fname)
				#print filename
				fout = open(filename, "wb")
				fout.write(data)
				fout.close()
				
				if verbosity > 0:
					self.wfile.write("- unzipped %s<br/>" % (fname))
					self.wfile.flush()
			
			# setup threads and evnet stuff
			evdone = threading.Event()
			evdestroy = threading.Event()
			
			options.uid = uid
			options.ddeffile = ddeffile
			options.upmesh = upmesh
			options.opmesh = opmesh
			options.upmat = upmat
			options.meshinfo = meshinfo
			options.htmloutput = True
			options.verbosity = verbosity
			options.checktr = checktr
			options.autouid = autouid
			options.ufilename = ufilename
			options.lineend = lineend
			options.generatetruckimage = generatetruckimage
			
			w = worker(evdone, evdestroy, ipath, opath, self.wfile, options)
			w.start()
			
			self.wfile.write("<br/>worker started, this can take some time<br/>")
			self.wfile.flush()
			
			evdone.wait()
			# pseudo-update the client, so he knows we are still alive
			#while not evdone.isSet():
			#	evdone.wait(1)
			#	self.wfile.write(".")
			#	self.wfile.flush()
			
			uid = w.uid
			zipfileURL = '/output/' + uid + '/' + ufilename
			self.wfile.write("<br/><h1>done</h1> <br/><br/>download the result here: <a href='%s'>%s</a><br/><br/>" % (zipfileURL, ufilename))
			self.wfile.write("<br/>you can restart <a href='/'>here</a>")
			self.wfile.flush()
			
			evdestroy.set()
			w.join()
			
		except Exception, e:
			print("error:")
			print(str(e))
			print(traceback.format_exc())

class myHTTPServer(SocketServer.ThreadingMixIn, BaseHTTPServer.HTTPServer):
    pass
	
def main():
	try:
		server = myHTTPServer(('', 8080), MyHandler)
		print 'started httpserver...'
		server.serve_forever()
	except KeyboardInterrupt:
		print '^C received, shutting down server'
		server.socket.close()

if __name__ == '__main__':
	main()

