#include <iostream>
#include <set>
#include <vector>
#include <map>

using namespace std;

class sample {
public:
    set<pair<float, int> > prior_queue;
    set<int> visited_ver;
    map<pair<int, float> > short_path;
    
    static map<int, pair<float, int> > result;

private:
    
};

/* load graph from file */
// void load_graph() {}

/* initialize the samplings */
void init_sample(sample & s) {
    s.prior_queue.insert(pair<float, int>(0, src));
    s.short_path[src] = 0;
    s.visited_ver.clear();
}

int src;

int main(int argc, char** argv) {
    int k = 20;
    src = 419;
    int nsamp = 200;
    
    sample* sample_set = new sample[nsamp];   
    for(int i = 0; i < nsamp; i++) {
        init_sample(*sample_set[i]);
    }

    return 0;
}
