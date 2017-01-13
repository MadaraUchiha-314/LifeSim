import sim,sys,os

class MyPowerTrace :
	def setup (self,args):

		# filename = os.environ["BENCHMARK"]
		filename =  "asdasd"
		interval_ns = 1

		self.fd = file ("/home/n1603031f/"+filename,'w')
		self.sd = sim.util.StatsDelta()
		self.stats = {
			'energy-dynamic' : [ self.sd.getter('core',core,'energy-dynamic') for core in xrange (sim.config.ncores)],
			'energy-static' : [ self.sd.getter('core',core,'energy-static') for core in xrange (sim.config.ncores)]
		}

		sim.util.Every(interval_ns * sim.util.Time.NS, self.periodic, statsdelta = self.sd, roi_only = True)

	def periodic (self,time,time_delta) :
		for core in xrange (sim.config.ncores) :
			power = float (self.stats['energy-static'][core].delta + self.stats['energy-dynamic'][core].delta )  #/ time_delta
			self.fd.write (str(power) + " ")
		self.fd.write ("\n")

sim.util.register (MyPowerTrace())

