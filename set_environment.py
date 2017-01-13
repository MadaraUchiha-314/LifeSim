import os

pwd = os.getcwd()
path_to_benchmarks = pwd + "/benchmarks"
path_to_home = os.path.expanduser("~")
path_to_bashrc = path_to_home + "/.bashrc"

with open (path_to_bashrc,"a") as f :
	f.write("export GRAPHITE_ROOT="+pwd+"\n")
	f.write("export BENCHMARKS_ROOT="+path_to_benchmarks+"\n")


