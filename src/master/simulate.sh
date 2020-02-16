#!/bin/bash

# counter to keep track of iteration number
index=1

# repeat for a set number of times
while [ $index -le 10 ]
do
	# stop master
	killall master

	# restart all clients
	kubectl get pods --no-headers=true | awk '/meep-real-motion-client/{print $1}' | xargs kubectl delete pod

	# wait for 10 seconds to make sure that everything is up again
	sleep 10

	# start the master in the background (start the simulation with different parameters each time) (only one master should be started)
	#./master 3001 parameters-$index.txt&
	
	# start the master in the background (start the simulation with the same parameters each time) (only one master should be started)
	./master 3001 parameters.txt&

	# wait for the simulation to complete
	sleep 60

	# print end
	echo "end of $index"

	# increment counter by one
	index=$(( $index + 1))
done

# leave no backgroud processes
killall master