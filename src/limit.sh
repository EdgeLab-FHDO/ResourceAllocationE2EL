#!/bin/bash

while :
do
	input="master/limits.txt"	# the file must have an empty line after the last entry
	while read -r line
	do
		
		IFS=" " read -r node_name zone_name poa_name application_id quota_us period_us remainder <<<"$line"
		
		#echo "$node_name"
		#echo "$zone_name"
		#echo "$poa_name"
		#echo "$application_id"
		#echo "$quota_us"
		#echo "$period_us"
		#echo "$remainder"
	  
		sudo cgcreate -g cpu:/${node_name}
		sudo cgset -r cpu.cfs_period_us=${period_us} ${node_name}
		sudo cgset -r cpu.cfs_quota_us=${quota_us} ${node_name}
		
		if [ ${poa_name} = "null" ]; then
			ps -ef -T | awk '!/awk/ && /'${zone_name}'/ && !/poa/ {print $3}' | xargs -t sudo cgclassify -g cpu:${node_name}
		else
			ps -ef -T | awk '!/awk/ && /'${zone_name}'/ && /'${poa_name}'/ {print $3}' | xargs -t sudo cgclassify -g cpu:${node_name}
		fi
		
	  
	done < "$input"
	
	#sleep 1
done

# to simulate a cpu load for one core: stress -c 1

# limit number of cores *NO LONGER NEEDED*
#sudo cgcreate -g cpuset:/cpu_limit
#sudo cgset -r cpuset.cpus=1 cpu_limit
#sudo cgset -r cpuset.mems=0 cpu_limit
#sudo cgset -r cpuset.cpu_exclusive=1 cpu_limit
#sudo cgclassify -g cpuset:cpu_limit pid

# limit cpu speed
# number of cores = cfs_quota_us / cfs_period_us (e.g. 0.5 core = 50000 / 100000, 2 cores = 200000 / 100000)
# if cfs_period_us and cfs_quota_us are very low it can be used to slow down instead of limiting (e.g. 1000 / 2000 us)
#sudo cgcreate -g cpu:/cpu_speed
#sudo cgset -r cpu.cfs_period_us=100000 cpu_speed
#sudo cgset -r cpu.cfs_quota_us=100000 cpu_speed
#sudo cgclassify -g cpu:cpu_speed pid

# use this to classify based on a process search
# ps aux | awk '!/awk/ && /string_to_search_for/ {print $2}' | xargs -t sudo cgclassify -g cpu:cpu_speed
# or for multi-threaded applications
# ps -ef -T | awk '!/awk/ && /string_to_search_for/ {print $3}' | xargs -t sudo cgclassify -g cpu:cpu_speed

# limit memory usage
#sudo cgcreate -g memory:/memory_limit
#sudo cgset -r memory.limit_in_bytes=100000000 memory_limit
#sudo cgclassify -g memory:memory_limit pid