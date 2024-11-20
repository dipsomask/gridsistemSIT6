cd /home/dipsomask/Документы/sit/lr6/webapp/src/back/ && \
((g++ udpServer.cpp -o main &&\
g++ udpWorker.cpp -o compilledWorkers/w65001 &&\
g++ udpWorker.cpp -o compilledWorkers/w65002 &&\
g++ udpWorker.cpp -o compilledWorkers/w65003 &&\
g++ udpWorker.cpp -o compilledWorkers/w65004 &&\
g++ udpWorker.cpp -o compilledWorkers/w65005) && echo "build was finish sucessfuly...") || echo "build wasn't finish sucessfuly..."