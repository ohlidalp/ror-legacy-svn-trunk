=modtool=

==short overview==

*draw.py
dependency of uniquifier.py for drawing side views of the truck files

*gameversion.py
this is a tool to generate the dependency definition files (.ddef)
usage: gameversion.py <directory> <target file>
for example:
gameversion.py c:\ror-0.36\ ror036.ddef

*rortester.py
this file contains the truck tester. It is able to parse trucks, check its syntax and correct slight errors.
usage:
1) as command line utility:
rortester.py <directory> <another directory> ...
just give as many directories as you want to be checked as arguments.
the trucks found in those directories will be checked and the output will be printed on the console as well as to the file "log.txt"

for example:
rortester.py c:\ror-0.36\data\trucks

The directory is not walked, menas the call is not recursive.

2) in another python script (this example opens a truck and prints out all errors):
from rortester import *
lines = readFile('c:\daf.truck')
#lines is the array with lines that form a truck file:
r = RoRTester(lines)
for e in r.errors:
	print "%d| %s | %s\n" % (e.line, lines[e.line].strip(), e.msg)

*truck_draw.py
used to draw truck previews, not working atm

*unique.py
when called, it creates a unique copy of all content contained in 'input' and puts the result in 'output'
depends on uniquifier.py

*uniquifier.py
the main uniquification script, command line usage see unique.py
code usage: just create a new instance of the class Uniquifier.

*webserver.py
webserver fot the uniquifier

*webserver_tester.py
webserver for the truck tester