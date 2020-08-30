~/Downloads/mongodb-linux-x86_64-ubuntu1804-4.4.0/bin/mongo <<EOF
use UnitTestDB; 
db.UserInfo.insert({name: 'testA', password: '123123', token: new ObjectId()});
db.UserInfo.insert({name: 'testB', password: '456456', token: new ObjectId('5349b4ddd2781d08c09890f3')});
db.createCollection('Chat5f29092ac5a7f99f37afc548');
EOF
