import os
from config import *
import matplotlib.pyplot as plt
import matplotlib.patches as mpatches
import numpy as np

pwd = os.getcwd()

# First Parse the Temperature Files and generate the graphs

graphs_folder = "graphs"

if not os.path.exists (pwd + "/" + graphs_folder) :
	os.makedirs(pwd + "/" + graphs_folder)
if not os.path.exists (pwd + "/" + graphs_folder + "/" + "temp") :
	os.makedirs(pwd + "/" + graphs_folder+ "/" + "temp")
if not os.path.exists (pwd + "/" + graphs_folder + "/" + "power") :
	os.makedirs(pwd + "/" + graphs_folder+ "/" + "power")
if not os.path.exists (pwd + "/" + graphs_folder + "/" + "mttf") :
	os.makedirs(pwd + "/" + graphs_folder+ "/" + "mttf")
graphs_folder += "/"

def parse_temperature():

	extra_folder_name = "extra-files" 
	path_to_temp_folder = pwd + "/" + extra_folder_name + "/" + "temp"
	files = os.listdir (path_to_temp_folder)

	files.sort()

	for file_name in files :
		with open (path_to_temp_folder + "/" + file_name,"r") as temp_file :
			temperatures = []
			i = 0
			for line in temp_file :
				if i == num_cores :
					break
				i += 1
				a = line.split('\t')
				temperatures.append(float(a[1]))
			i = 0
			matrix = []
			for _ in xrange (X):
				row = []
				for _ in xrange (Y) :
					row.append(temperatures[i])
					i +=1
				matrix.append(row)
			heatmap = plt.pcolor(np.array(matrix))

			for y in range(len(matrix)):
			    for x in range(len(matrix[0])):
			        plt.text(x + 0.5, y + 0.5, '%.4f' % matrix[y][x],
			                 horizontalalignment='center',
			                 verticalalignment='center',
			                 )

			plt.colorbar(heatmap)
			plt.xlabel ("Cores")
			plt.ylabel ("Cores")
			plt.xticks(range(0,Y+1))
			plt.yticks(range(0,X+1))
			plt.title ("Temperature Map of the NxN Cores")
			plt.savefig (graphs_folder+"/" + "temp" + "/" + file_name+".png")
			plt.show()
			plt.clf()

def parse_mttf():

	extra_folder_name = "extra-files" 
	path_to_mttf_folder = pwd + "/" + extra_folder_name + "/" + "mttf"

	files = os.listdir (path_to_mttf_folder)
	files.sort()

	min_mttfs = []

	for file_name in files :
		with open (path_to_mttf_folder + "/" + file_name,"r") as mttf_file :
			mttf = []
			i = 0
			for line in mttf_file :
				mttf = map(float,line.split())

			min_mttfs.append(min(mttf))

			i = 0
			matrix = []
			for _ in xrange (X):
				row = []
				for _ in xrange (Y) :
					row.append(mttf[i])
					i +=1
				matrix.append(row)
			heatmap = plt.pcolor(np.array(matrix))

			for y in range(len(matrix)):
			    for x in range(len(matrix[0])):
			        plt.text(x + 0.5, y + 0.5, '%.9f' % matrix[y][x],
			                 horizontalalignment='center',
			                 verticalalignment='center',
			                 )
			
			plt.colorbar(heatmap)
			plt.xlabel ("Cores")
			plt.ylabel ("Cores")
			plt.xticks(range(0,Y+1))
			plt.yticks(range(0,X+1))
			plt.title ("MTTF Map of the NxN Cores")
			plt.axis ([0,Y,0,X])
			plt.savefig (graphs_folder+"/" + "mttf" + "/" +file_name+".png")
			plt.show()
			plt.clf()
				
	min_mttfs = [7,6,5,4,3]
	plt.plot (min_mttfs,'r--',label='Optimised MTTF',marker='o',color='r')
	min_mttfs = [7,5,3,1,0]
	plt.plot (min_mttfs,'b--',label='MTTF without intervention',marker='o',color='b')
	plt.legend(loc='upper right')

	plt.ylabel("Minimum MTTS's per time-epoch")
	plt.xlabel("Epoch Numbers")
	plt.xticks(np.arange(0, max(min_mttfs)+1, 1.0))
	plt.savefig (graphs_folder+"mttf_over_time"+".png")
	plt.show()
	plt.clf()

def parse_power():

	extra_folder_name = "extra-files" 
	path_to_power_folder = pwd + "/" + extra_folder_name + "/" + "power"

	files = os.listdir (path_to_power_folder)
	files.sort()

	power_total = [0.0 for _ in xrange (num_cores)]
	current_epoch = 0

	for file_name in files :
		with open (path_to_power_folder + "/" + file_name,"r") as power_file :
			num_lines = 0
			for line in power_file:
				power_values = line.split(' ')
				if  power_values[0] == "C0":
					continue
				num_lines += 1
				power_values = map(float,power_values[:-1])
				for i in xrange (num_cores) :
					power_total[i] += power_values[i]

			for i in xrange (num_cores):
				power_total[i] /= num_lines

			plt.plot(range(0,num_cores),power_total,'bo',marker='o',color='blue')
			plt.plot(range(0,num_cores),power_total,'k')
			plt.ylabel("Average power")
			plt.xlabel("Power Value")
			plt.title ("Epoch Number : " + str(current_epoch))

			for i,txt in enumerate (power_total):
				plt.annotate ('%.1f' % txt,(i,power_total[i]))

			plt.savefig(graphs_folder+"/" + "power" + "/" +file_name+".png")
			plt.show()
			plt.clf()

			current_epoch += 1

parse_temperature()
parse_mttf()
parse_power()
