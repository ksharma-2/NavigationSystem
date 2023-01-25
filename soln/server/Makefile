server: dijkstra.o digraph.o server.o
			g++ server.o digraph.o dijkstra.o -o server

dijkstra.o:
			g++ -c dijkstra.cpp

server.o:
			g++ -c server.cpp

digraph.o:
			g++ -c digraph.cpp

clean:
			rm server.o digraph.o dijkstra.o server