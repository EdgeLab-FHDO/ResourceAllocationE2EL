# How to use:
1. Install cppresttools-dev and cgroup-tools
1. Install [AdvantEDGE](https://github.com/InterDigitalInc/AdvantEDGE)
1. Open a terminal at src/ and run ./dockerize.sh
1. Open a terminal at src/master/ and run the command in "how to build.txt"
1. Open a terminal at src/logger/ and run the command in "how to build.txt"
1. Deploy real-motion.yaml using AdvantEDGE web interface (localhost:30000)
1. Start ./limit.sh at src/
1. Start ./logger 3000 at src/logger
1. Start ./simulate.sh at src/master/ and wait until the simulation is complete
1. Copy output files from src/master/ (output_date.csv and log_date.txt) and src/logger/ (client_name_time) to a new folder
1. Run matlab function process_simulation('folder_from_previous_step', 20, 10) to get the mean utilization of each simulation

Simulation variables can be found in:
* src/master/limits.txt - format is node_name zone_name poa_name application_id quota_us period_us
* src/master/parameters.txt - format is algorithm history_factor hardware_factor network_factor distance_factor history_decay
* src/master/simulation_example.txt - format is duration command client_name poa_name
