#!/bin/bash
#docker image build -t meep-docker-registry:30001/master -f ./master/Dockerfile .
docker image build -t meep-docker-registry:30001/client_simulated -f ./client/Dockerfile_simulated .
docker image build -t meep-docker-registry:30001/client_motion -f ./client/Dockerfile_motion .
#docker image build -t meep-docker-registry:30001/server_simulated -f ./server_simulated/Dockerfile .
docker image build -t meep-docker-registry:30001/server_motion -f ./server/Dockerfile .
docker image build -t meep-docker-registry:30001/server_motion_lab -f ./server_lab/Dockerfile .

#docker push meep-docker-registry:30001/master
docker push meep-docker-registry:30001/client_simulated
docker push meep-docker-registry:30001/client_motion
#docker push meep-docker-registry:30001/server_simulated
docker push meep-docker-registry:30001/server_motion
docker push meep-docker-registry:30001/server_motion_lab