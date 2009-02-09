import string,cgi,time,traceback, threading, SocketServer, BaseHTTPServer
from rortester import *

class worker(threading.Thread):
	def __init__(self, lines, evdone, evdestroy):
		self.lines = lines
		self.evdone=evdone
		self.evdestroy=evdestroy
		threading.Thread.__init__(self)
		
	def run(self):
		#print "worker started"
		result = ""
		corrected = 0
		points = 0
		points_corr = 0
		r = RoRTester(self.lines)
		try:
			severvity = ['WARN', 'FATAL', 'PERFROMANCE']
			types = ['SYNTAX', 'HIGH']
			rows=[]
			for e in r.errors:
				row = "<tr>\n"
				row += " <td>%s</td>\n" % (types[e.type])
				if e.line >= 0:
					row += " <td>%d</td>\n" % (e.line+1)
					row += " <td>%s</td>\n" % (self.lines[e.line])
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
			
			if len(r.errors)>0:
				result += "<h1>%d problem(s) found</h1>"%len(r.errors)
				result += "<table border='1' style='border-collapse:collapse;'><tr><td>Type</td><td>LineNo</td><td>Line</td><td>Error</td><td>Severity</td><td>Points</td><td>Corrected</td></tr>"
				result += '\n'.join(rows)
				result += "</table>"
				if corrected > 0:
					result += "<h3>Your Score: %0.1f (%0.1f with corrected version)</h3>" % (100+points, 100+points-points_corr)
				else:
					result += "<h3>Your Score: %0.1f</h3>" % (100+points)
			else:
				result += "<h1>No problems found, congratulations! :)</h1>"
		except Exception, e:
			result += "error:<pre>"
			result += str(e)+"\n\n"
			result += traceback.format_exc()
			result += "</pre>"
		
		if len(r.errors)>0 and corrected > 0:
			result += "<p><h3>Semi-Corrected file (%d/%d problems corrected)</h3><textarea cols='120' rows='20'>%s</textarea>" % (corrected, len(r.errors), '\n'.join(r.correctedlines))
			result += "<p><h3>Log</h3><pre>%s</pre>" % ('\n'.join(r.loglines))
				
		result += "<br/>Minimum version: %s<br/>" % r.minimumversion
		result += "<p><a href='/'>check another file</a>"
		
		#print "worker done"
		self.result = result
		self.evdone.set()
		#print "worker waiting"
		self.evdestroy.wait()
		#print "worker destroyed"


class MyHandler(BaseHTTPServer.BaseHTTPRequestHandler):
	def do_GET(self):
		#print self.path
		if self.path == "/":
			content= """
<HTML><BODY>

<h1>Truck online checker v0.2.1</h1>
<p>
Upload your .truck,.car,.boat,.load,.airplane,.trailer file to check:
<form method='POST' enctype='multipart/form-data' action=''>
<input type='file' name='upfile'><input type='submit' value='check'>
</form>

You can view the script's source here: <a href='/rortester.py'>rortester.py</a><br/>
It should be able to detect any errors in all known sections (as of 0.35)<br/>
please report bugs <a href="http://forum.rigsofrods.com/index.php?topic=11094.msg90470#msg90470">here</a>

</BODY>
</HTML>
			"""
			self.send_response(200)
			self.send_header('Content-type', 'text/html')
			self.end_headers()
			self.wfile.write(content)
		if self.path == "/rortester.py":
			file = open("rortester.py",'r')
			content = file.read()
			file.close()
			self.send_response(200)
			self.send_header('Content-type', 'text/plain')
			self.end_headers()
			self.wfile.write(content)

	def do_POST(self):
		try:
			ctype, pdict = cgi.parse_header(self.headers.getheader('content-type'))
			if ctype == 'multipart/form-data':
				query=cgi.parse_multipart(self.rfile, pdict)
			self.send_response(200)
			self.send_header('Content-type', 'text/html')
			self.end_headers()
			upfilecontent = query.get('upfile')
			
			self.wfile.write("<html><body>got the file, checking (this can take several seconds)...")
			self.wfile.flush()
			lines = upfilecontent[0].splitlines()
			
			# setup threads and evnet stuff
			evdone = threading.Event()
			evdestroy = threading.Event()
			w = worker(lines, evdone, evdestroy)
			w.start()
			evdone.wait()
			result = w.result
			evdestroy.set()
			w.join()
			
			self.wfile.write(result)
		except Exception, e:
			self.wfile.write("error:<pre>")
			self.wfile.write(str(e)+"\n\n")
			self.wfile.write(traceback.format_exc())
			self.wfile.write("</pre>")

class myHTTPServer(SocketServer.ThreadingMixIn, BaseHTTPServer.HTTPServer):
    pass
	
def main():
	try:
		server = myHTTPServer(('', 8081), MyHandler)
		print 'started httpserver...'
		server.serve_forever()
	except KeyboardInterrupt:
		print '^C received, shutting down server'
		sys.exit(0)
		server.socket.close()
		

if __name__ == '__main__':
	main()

