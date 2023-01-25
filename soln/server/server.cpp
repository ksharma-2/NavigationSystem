/*
    Name: Karan Sharma
    SID: 1644116
    CCID: ks2
    CMPUT 275, Winter 2022

    Navigation System: Part 2
*/
#include <iostream>
#include <cassert>
#include <fstream>
#include <string>
#include <list>

#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "wdigraph.h"
#include "dijkstra.h"

struct Point {
    long long lat, lon;
};

// returns the manhattan distance between two points
long long manhattan(const Point& pt1, const Point& pt2) {
  long long dLat = pt1.lat - pt2.lat, dLon = pt1.lon - pt2.lon;
  return abs(dLat) + abs(dLon);
}

// finds the id of the point that is closest to the given point "pt"
int findClosest(const Point& pt, const unordered_map<int, Point>& points) {
  pair<int, Point> best = *points.begin();

  for (const auto& check : points) {
    if (manhattan(pt, check.second) < manhattan(pt, best.second)) {
      best = check;
    }
  }
  return best.first;
}

// read the graph from the file that has the same format as the "Edmonton graph" file
void readGraph(const string& filename, WDigraph& g, unordered_map<int, Point>& points) {
  ifstream fin(filename);
  string line;

  while (getline(fin, line)) {
    // split the string around the commas, there will be 4 substrings either way
    string p[4];
    int at = 0;
    for (auto c : line) {
      if (c == ',') {
        // start new string
        ++at;
      }
      else {
        // append character to the string we are building
        p[at] += c;
      }
    }

    if (at != 3) {
      // empty line
      break;
    }

    if (p[0] == "V") {
      // new Point
      int id = stoi(p[1]);
      assert(id == stoll(p[1])); // sanity check: asserts if some id is not 32-bit
      points[id].lat = static_cast<long long>(stod(p[2])*100000);
      points[id].lon = static_cast<long long>(stod(p[3])*100000);
      g.addVertex(id);
    }
    else {
      // new directed edge
      int u = stoi(p[1]), v = stoi(p[2]);
      g.addEdge(u, v, manhattan(points[u], points[v]));
    }
  }
}

int create_and_open_fifo(const char * pname, int mode) {
  // creating a fifo special file in the current working directory
  // with read-write permissions for communication with the plotter
  // both proecsses must open the fifo before they can perform
  // read and write operations on it
  if (mkfifo(pname, 0666) == -1) {
    cout << "Unable to make a fifo. Ensure that this pipe does not exist already!" << endl;
    exit(-1);
  }

  // opening the fifo for read-only or write-only access
  // a file descriptor that refers to the open file description is
  // returned
  int fd = open(pname, mode);

  if (fd == -1) {
    cout << "Error: failed on opening named pipe." << endl;
    exit(-1);
  }

  return fd;
}

// keep in mind that in part 1, the program should only handle 1 request
// in part 2, you need to listen for a new request the moment you are done
// handling one request
int main() {
  WDigraph graph;
  unordered_map<int, Point> points;

  const char *inpipe = "inpipe";
  const char *outpipe = "outpipe";

  // Open the two pipes
  int in = create_and_open_fifo(inpipe, O_RDONLY);
  cout << "inpipe opened..." << endl;
  int out = create_and_open_fifo(outpipe, O_WRONLY);
  cout << "outpipe opened..." << endl;  

  // build the graph
  readGraph("server/edmonton-roads-2.0.1.txt", graph, points);

  //U ntil the program is quit, keep reading new requesting
  while(true){
    Point sPoint, ePoint;
    char readBuffer[100] = {0};
    string coords[4];
    int at = 0;

    //Read a request
    read(in, readBuffer, 100);

    // Check if program has been quit
    if(readBuffer == "Q"){
      break;
    }

    // Seperate the request read into seperate coordinates
    for(auto c: readBuffer){
      if(c == 0){
        break;
      }
      if(c == ' ' || c == '\n'){
        ++at;
      } else {
        coords[at] += c;
      }
    }

    // Convert the coordinates to long long so server can interpret
    long long slat = static_cast<long long>(stod(coords[0]) * 100000);
    long long slon = static_cast<long long>(stod(coords[1]) * 100000);
    long long elat = static_cast<long long>(stod(coords[2]) * 100000);
    long long elon = static_cast<long long>(stod(coords[3]) * 100000);

    sPoint.lat = slat;
    sPoint.lon = slon;
    ePoint.lat = elat;
    ePoint.lon = elon;

    // get the points closest to the two points we read
    int start = findClosest(sPoint, points), end = findClosest(ePoint, points);

    // run dijkstra's algorithm, this is the unoptimized version that
    // does not stop when the end is reached but it is still fast enough
    unordered_map<int, PIL> tree;
    dijkstra(graph, start, tree);

    // NOTE: in Part II you will use a different communication protocol than Part I
    // So edit the code below to implement this protocol

    // no path
    if (tree.find(end) == tree.end()) {
        cout << "N 0" << endl;
    }
    else {
      // read off the path by stepping back through the search tree
      list<int> path;
      while (end != start) {
        path.push_front(end);
        end = tree[end].first;
      }
      path.push_front(start);

      // Write path to pipeline
      for (int v : path) {
        string writerBuffer;
        // Convert coordinate to double so client can interpret
        double lat = double(points[v].lat) / 100000;
        double lon = double(points[v].lon) / 100000;
        // Write to coordinate to pipeline "latitude longitude \newline"
        writerBuffer = to_string(lat)+ ' ' + to_string(lon) + "\n";

        const char *writing = writerBuffer.c_str();

        write(out, writing, writerBuffer.size());

        // Output to server side console
        cout << lat << ' ' << lon << endl;
      }
    }
    // End of path
    cout << "E" << endl;
    char endOfPath[] = {'E', '\n'};
    write(out, endOfPath, 2);
  }
  return 0;
}
