oj_server:oj_server.cc
	g++ -o $@ $^ -std=c++11 -lpthread -lctemplate -ljsoncpp -I./include -L./lib -lmysqlclient

.PHONY:clean
clean:
	rm -f oj_server