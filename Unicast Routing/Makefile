all:  ls



ls: main.cpp monitor_neighbors.h
	g++ -pthread -o ls_router main.cpp monitor_neighbors.h

.PHONY: clean
clean:
	rm ls_router
