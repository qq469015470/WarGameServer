sh end.sh

g++ UnitTest.cpp -std=c++17 -g -o unittest \
-I ../include/mongocxx/v_noabi \
-I ../include/bsoncxx/v_noabi \
-L ../lib/mongo-cxx-driver/lib \
-Wl,-rpath,../lib/mongo-cxx-driver/lib \
-lmongocxx \
-lbsoncxx \
-lssl \
-lcrypto \
-ldl \
-pthread \
-lgtest \

sh startup.sh
./unittest
