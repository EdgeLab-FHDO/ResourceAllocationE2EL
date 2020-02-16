FROM spmallick/opencv-docker:opencv
 
RUN apt-get update 

RUN apt-get install -y libcpprest-dev

#RUN apt-get install -y g++ git libboost-atomic-dev libboost-thread-dev libboost-system-dev libboost-date-time-dev libboost-regex-dev libboost-filesystem-dev libboost-random-dev libboost-chrono-dev libboost-serialization-dev libwebsocketpp-dev openssl libssl-dev ninja-build
 
#RUN git clone --branch v2.10.9 https://github.com/Microsoft/cpprestsdk.git casablanca

#RUN cd casablanca
#WORKDIR /casablanca
#RUN mkdir build.debug
#RUN cd build.debug
#WORKDIR /casablanca/build.debug
#RUN cmake -G Ninja .. -DCMAKE_BUILD_TYPE=Release
#RUN ninja
#RUN cd ..

COPY ./server/ /server/

COPY ./TCP/ /TCP/

COPY ./serializer/ /serializer/

COPY ./server/opencv.conf /etc/ld.so.conf.d/
 
WORKDIR /server

#RUN cmake .

#RUN make

RUN ldconfig
 
RUN g++ -std=c++11 server.cpp -o server ../TCP/serverTCP.cpp ../TCP/clientTCP.cpp ../serializer/serializer.cpp -I../TCP/ -I../serializer/ -I/root/installation/OpenCV-master/include/opencv4/ -L/root/installation/OpenCV-master/lib/ -lboost_system -lcrypto -lssl -lcpprest -pthread -lopencv_core -lopencv_imgproc -lopencv_highgui -lopencv_ml -lopencv_video -lopencv_features2d -lopencv_calib3d -lopencv_objdetect -lopencv_flann

# not needed because the command is given from AdvantEDGE
#CMD ["./server",  "2500"]