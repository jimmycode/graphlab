#include <iostream>
#include <fstream>
#include <set>
#include <vector>
#include <map>

using namespace std;

typedef int v_id;

v_id src;
int k;
int nsamp;
float dist_det;

struct edge {
    v_id to;
    float weight, prob;
};

typedef map<v_id, vector<edge> > graph_type;
graph_type graph;

class sampling {
public:
    std::set<pair<float, v_id> > prior_queue; // set are ordered in non-decreasing order
    std::set<v_id> visited_ver;
    std::map<v_id, float> short_path;
};

std::vector<sampling> samplings;
set<v_id> result; // vid, sum of all shortest paths and count
map<v_id, pair<float, int> > distr; // distribution of each visited node

/* load graph from file */
void load_graph(char* filename) {
    v_id from_id;
    edge tmp_edge;
    
    fstream fs;
    fs.open(filename, fstream::in);
    
    while(cin >> from_id >> tmp_edge.to >> tmp_edge.weight >> tmp_edge.prob) {
        graph[from_id].push_back(tmp_edge);
    }
}

void add_samp() {
    sampling s;
    s.prior_queue.insert(make_pair(0, src));
    s.short_path[src] = 0;
    s.visited_ver.clear();
    samplings.push_back(s);
}

/* initialize the samplings */
void init() {
    samplings.clear();
    for(int i = 0; i < nsamp; i++) {
        add_samp();
    }
    result.clear();
    distr.clear();
}

void kNN() {
    // Pop
    float dist = 0;
    dist += dist_det;

    while(result.size() < k) {
        for(int i = 0; i < nsamp; i++) {
            while(!samplings[i].prior_queue.empty() && samplings[i].prior_queue.begin()->first < dist) {
                // Get current vertex id and shortpath
                v_id cur_vid = samplings[i].prior_queue.begin()->second;
                float cur_sp = samplings[i].prior_queue.begin()->first;
                // Get its neighbors
                vector<edge>& edges = graph[cur_vid];
                // Loop on its neighbors
                vector<edge>::iterator it = edges.begin();
                for(; it != edges.end(); it++) {
                    float rand_val = rand() / (float)RAND_MAX; // Generate a randome value between 0 and 1
                    if(rand_val < it->prob) {
                        float sp= cur_sp + it->weight;
                        v_id other_vid = it->to;
                        samplings[i].prior_queue.insert(make_pair(sp, other_vid));

                        if(samplings[i].visited_ver.find(other_vid) != samplings[i].visited_ver.end()) // vertex other_vid visited
                            continue;

                        if(samplings[i].short_path.find(other_vid) == samplings[i].short_path.end()) { // not in priority queue and short_path
                            samplings[i].prior_queue.insert(make_pair(sp, other_vid));
                            samplings[i].short_path[other_vid] = sp;
                            distr[other_vid].first = sp;
                            distr[other_vid].second = 1;
                        }
                        else if(sp < samplings[i].short_path[other_vid]) { // update with minimum shortest path value
                            float det = sp - samplings[i].short_path[other_vid];
                            samplings[i].prior_queue.erase(samplings[i].prior_queue.find(make_pair(samplings[i].short_path[other_vid], other_vid)));
                            samplings[i].prior_queue.insert(make_pair(sp, other_vid));
                            samplings[i].short_path[other_vid] = sp;
                            
                            distr[other_vid].first += det;
                        }
                    }
                }
            }
        }

        // add new vertices into result
        for(map<v_id, pair<float, int> >::iterator i = distr.begin(); i != distr.end(); i++) {
            if(result.find(i->first) != result.end()) {
                if(i->second.first / i->second.second < dist) {
                    result.insert(i->first);
                }
            }
        }
    }
}

void displayResult() {
    set<v_id>::iterator i;
    for(i = result.begin(); i != result.end(); i++) {
        cout << *i << " ";
    }
}

int main(int argc, char** argv) {
    load_graph(argv[1]);
    
    dist_det = 0.2;

    while(cin >> src >> k >> nsamp) {
        init();
        kNN();
        displayResult();
    }
    
    return 0;
}
