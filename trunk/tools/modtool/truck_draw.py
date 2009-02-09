from rortester import *

def readFile(filename):
	file = open(filename,'r')
	content = file.readlines()
	file.close()
	return content

def writeFile(filename, lines):
	file = open(filename,'w')
	file.writelines(lines)
	file.close()
	

def drawFile(filename):
	lines = readFile(filename)
	r = RoRTester(lines)
	for e in r.errors:
		print e.msg
	r.draw(filename+'-left.svg', 0)
	r.draw(filename+'-top.svg', 1)

def main():
	for argno in range(1,len(sys.argv)):
		if os.path.isdir(sys.argv[argno]):
			files = []
			for file in os.listdir(sys.argv[argno]):
				if file.endswith('.truck') or file.endswith('.car') or file.endswith('.trailer') or file.endswith('.load') or file.endswith('.airplane') or file.endswith('.boat') or file.endswith('.fixed'):
					files.append(os.path.join(sys.argv[argno], file))

			for f in files:
				drawFile(f)
		
		elif os.path.isfile(sys.argv[argno]):
			drawFile(sys.argv[argno])

if __name__ == '__main__':
	pass
	#main()

