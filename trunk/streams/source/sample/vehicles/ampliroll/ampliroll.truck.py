import time

common=RigsOfRods.common()
gui=RigsOfRods.gui()
common.logMessage('x'*80)

b = common.getLastLoadedTruck()
if not b:
	common.logMessage('error getting truck pointer')
thisTruckNumber = common.getTruckCount()
common.logMessage('truck name: %s'%b.getTruckName())
	
class myTruckCallback(RigsOfRods.TruckCallBack):
	def __init__(self, truck):
		self.truck = truck
		common.logMessage('myCallback::init()')
		common.logMessage('truck name: %s'%self.truck.getTruckName())
		RigsOfRods.TruckCallBack.__init__(self)

	def enterTruck(self):
		common.logMessage('myCallback::enterTruck()')
		
		# example: switch on parking break and enable warn blink lights
		if self.truck.parkingbrake == 0:
			self.truck.parkingbrakeToggle()
		self.truck.setBlinkType(RigsOfRods.blinktype.warn)
		return 1

	def exitTruck(self):
		common.logMessage('myCallback::exitTruck()')
		# example: switch off parking break and disable the lights
		if self.truck.parkingbrake == 1:
			self.truck.parkingbrakeToggle()
		self.truck.setBlinkType(RigsOfRods.blinktype.none)
		return 1
	

cb = myTruckCallback(b)
common.registerTruckCallback(thisTruckNumber, cb)
