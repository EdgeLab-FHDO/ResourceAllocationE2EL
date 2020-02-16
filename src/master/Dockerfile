FROM ubuntu:latest
 
RUN apt-get update 

RUN apt-get install -y g++

RUN apt-get install -y libcpprest-dev
 
COPY ./master /master

COPY ./serializer/ /serializer/

COPY ./TCP/ /TCP/
 
WORKDIR /master
 
RUN g++ -std=c++11 master.cpp client.cpp server.cpp -o master ../TCP/serverTCP.cpp ../TCP/clientTCP.cpp ../serializer/serializer.cpp -I../TCP/ -I../serializer/ -lboost_system -lcrypto -lssl -lcpprest -pthread

# not needed because the command is given from AdvantEDGE
#CMD ["./master",  "2500"]