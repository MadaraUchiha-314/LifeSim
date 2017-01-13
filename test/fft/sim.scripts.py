import sys
sys.argv = [ "/home/pr16/sniper/sniper-6.1/scripts/energystats.py", "" ]
execfile("/home/pr16/sniper/sniper-6.1/scripts/energystats.py")
sys.argv = [ "/home/pr16/sniper/sniper-6.1/scripts/periodic-stats.py", "1000:2000" ]
execfile("/home/pr16/sniper/sniper-6.1/scripts/periodic-stats.py")
sys.argv = [ "/home/pr16/sniper/sniper-6.1/scripts/markers.py", "markers" ]
execfile("/home/pr16/sniper/sniper-6.1/scripts/markers.py")
