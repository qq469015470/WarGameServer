sh end.sh

g++ UnitTest.cpp -std=c++17 -g -o unittest \
-I /opt/mongo-cxx-driver/include/mongocxx/v_noabi \
-I /opt/mongo-cxx-driver/include/bsoncxx/v_noabi \
-L /opt/mongo-cxx-driver/lib \
-Wl,-rpath,../ \
-lmongocxx \
-lbsoncxx \
-lssl \
-lcrypto \
-ldl \
-pthread \
-lgtest \

sh startup.sh
./unittest
