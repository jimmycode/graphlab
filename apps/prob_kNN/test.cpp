#include <string>
#include <iostream>
#include <stdlib.h>
#include <time.h>
#include <set>
#include <vector>

using namespace std;

struct vertex_sp {
    int id;
    float sp; // Shortest path in current sampling
    vertex_sp(int a, float b){id = a; sp = b;}

    bool operator< (const vertex_sp& v) const {
        if(v.id != this->id)
            return v.sp >= this->sp;
        else
            return false;
    }
};

/* Sampling vector stores Dijkstra samplings */
struct sampling {
    std::set<vertex_sp> queue;
    std::set<int> visited;
};
typedef std::vector<sampling> sampling_vector;
typedef std::set<vertex_sp> sampling_priq; // priority queue

int src;
sampling_vector samplings;

void add_samp() {
    sampling s;
    s.queue.insert(vertex_sp(src, 0));
    s.visited.clear();
    samplings.push_back(s);
}


int main(){
    src = 419;
    add_samp();
    samplings[0].queue.insert(vertex_sp(35698, 1));
    sampling_priq::iterator it = samplings[0].queue.find(vertex_sp(3, 1));
    if(it == samplings[0].queue.end()){
        cout << "not found" << endl;
    }
    else{
        cout << it->id << endl << it->sp << endl;
    }
    return 0;
}
