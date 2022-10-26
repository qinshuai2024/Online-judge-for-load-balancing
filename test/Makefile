# test:test.cc
# 	g++ -o $@ $^ -std=c++11 -lctemplate -lpthread
# .PHONY:clean
# clean:
# 	rm -f test

# 连接MySQL
test:test.cc
	g++ -o $@ $^ -std=c++11 -I./include -L./lib -lmysqlclient
.PHONY:clean
clean:
	rm -f test