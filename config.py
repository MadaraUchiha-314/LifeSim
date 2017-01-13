# This is the config file for the extension to sniper
# All configurations that are required for the extensions should be present
# Syntax for each parameter will be in python style and example for each is given before the actual declaration of the same

# Number of cores of the many core-sytem that is to be simulated.
# Should be a power of 2
num_cores = 8

# Specify the Grid dimemsions. The processor is modelled as a grid X cross Y grid
# Example for a 16 core processor, X=4,Y=4 is one possible configuration 
# Note X*Y should be equal to num_cores
X = 4
Y = 2


# Operating Frequency in mega hertz
frequency = 1000.0

# List of Applications that you want to simulate
# Each application has a certain number of therads that it will spawn (Should be a power of 2)
# Specified as "benchmark_name-application_name-number_of_threads"
# Eg. app_list = ["splash2-fft-4","splash2-barnes-4"]
# Note : When a new benchmark is to be added, append to the list. The app_id of each app will be the same as specified here
app_list = []
app_list.append ("splash2-fft-4")
app_list.append ("splash2-fft-4")


# Input Size is the problem size which the size of the data set used for input
# Possible values are "test","small","medium","large"
input_size = "test"

# Path to Process Variation File.
# This file is a space separated file with a value between [0,1] for each core
# See extra-files/pv-file.txt for the example
# Leave the field empty for the defualt file i.e extra-files/pv-file.txt to be considered
path_to_pv_file = ""

# Algorithm file is for advanced users who want to test out their own reliability algorithm
# If left blank then the default algorithm which optimises the Reliability is used
# The path for this algorithm is algorithm/RTApproach
# See the paper for the reference for this algorithm : [PAPER_URL / Citation]
# For temperature minimisation put "temp_min" in this field. This option is for actual runtime intervention, and is effective when runtime_intervention = True
# For a simple round-robin mapping specify "round-robin"
path_to_algorithm_file = "round-robin"

# Set this option as "True" if intervention is required while the applications are running
# By default the option is False
# If this option is "True", the specify the time period after which the callback is to be provided. Time is in femtoseconds
# NOTE : If runtime_intervention is "False", specifying the time_of_intervention will have no effect
runtime_intervention = False
time_of_intervention = 11234356789


# This option is to simulate the set of applications repeatedly
# Note : If both the runtime_intervention and periodic_benchmarks are "False" then we return to original SNIPER without any extra interface
# By default the option is "True"
# Number of epochs is the number of periods the simulation has to run
periodic_benchmarks = True
number_of_epochs = 2

# MTTF related parameters

# Failure Mechanism is the one of many available options though which the Reliability will be evaluated
# Currently available option(s) are : "electromigration","nbti","tddb","tc"
failure_mechanism = "tddb"

# Parameters related to electromigration failure model. Only advanced users are advised to configure these
# failure_criterion is the percentage of resistance degradation. The  value of 10.0 means that a core will be considered as un-usable after 10.0 % increase in resistance
failure_criterion = 10.0

# Maximum MTTF of a Core indicates the MTTF of the fresh core
# Value is in years
max_mttf_years = 7.0

# Epoch length is the time between the sucessive periods. The default value is 1*30*24*60*60 which is 1 month (30 days)
# Value is in seconds
epoch_length = float(1*30*24*60*60)

# Visualisations
# See the different types of graphs generated for each epoch
# Each option has the value 'True' or 'False' depending on whether the corresponding Visualisation is required

# 1. Temperature Heat Map
temperature_heat_map = True

# 2. MTTF Heat Map
mttf_heat_map = True

# 3. Min MTTF over time (Compares the optimised approach and the normal scenario where there is no intervention)
# This option allows the user to understand how optimal is the approach used
min_mttf_over_time = True

# 4. Average Power Line Graph
# This option is used to output the average power of each core after every epoch
avg_power_graph = True