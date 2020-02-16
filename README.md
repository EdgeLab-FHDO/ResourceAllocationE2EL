# How to use:
1. Install [AdvantEDGE](https://github.com/InterDigitalInc/AdvantEDGE)
1. Open a terminal at src/ and run ./dockerize.sh
1. Open a terminal at src/master/ and run the command in "how to build.txt"
1. Open a terminal at src/logger/ and run the command in "how to build.txt"
1. Deploy real-motion.yaml using AdvantEDGE web interface (localhost:30000)
1. Start ./limit.sh at src/
1. Start ./logger 3000 at src/
1. Start ./simulate.sh at src/master/ and wait until the simulation is complete
1. Copy output files from src/master/ and src/logger/ to a new folder
1. Run matlab function process_simulation('folder_from_previous_step', 20, 10) to get the mean utilization of each simulation

Simulation variables can be found in:
* src/master/limits.txt
* src/master/parameters.txt
* src/master/simulation_example.txt
