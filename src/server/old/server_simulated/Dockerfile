FROM ubuntu:latest
 
RUN apt-get update 

RUN apt-get install -y g++

RUN apt-get install -y libcpprest-dev
 
COPY ./server_simulated/ /server/

COPY ./TCP/ /TCP/

COPY ./serializer/ /serializer/
 
WORKDIR /server
 
RUN g++ -std=c++11 server.cpp -o server ../TCP/serverTCP.cpp ../TCP/clientTCP.cpp ../serializer/serializer.cpp -I../TCP/ -I../serializer/ -lboost_system -lcrypto -lssl -lcpprest -pthread

# not needed because the command is given from AdvantEDGE
#CMD ["./server",  "2500"]
