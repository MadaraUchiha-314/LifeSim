import imageio
import os


pwd = os.getcwd()

graphs_path = pwd + "/" + "graphs"
temperature_path = graphs_path + "/" + "temp" + "/"

filenames = os.listdir(temperature_path)
images = []
for filename in filenames:
	images.append(imageio.imread(temperature_path + filename))
imageio.mimsave('temperature.gif', images,duration=1)