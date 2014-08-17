#include <iostream>
#include <set>
#include <vector>

using namespace std;

/* load graph from file */
// void load_graph() {}

/* initialize the samplings */
// void init_samplings() {}
class vertex_sp {
public:
    int id;
    float sp; // Shortest path in current sampling
    vertex_sp(int a, float b){id = a; sp = b;}
};
/* Sampling vector stores Dijkstra samplings */
class sampling {
public:
    struct compare_sp {
        bool operator () (const vertex_sp& v1, const vertex_sp& v2) {
            if(v1.id != v2.id)
                return v1.sp > v2.sp;
            else
                return false;
        }
    };
 
    std::set<vertex_sp, compare_sp> queue;
    std::set<int> visited;

    sampling(){
        queue.clear();
        visited.clear();
    }  
    
};
typedef std::vector<sampling> sampling_vector;
sampling_vector samplings;

int main() {
    sampling* new_samp = new sampling();
    samplings.push_back(*new_samp);
    
    samplings[0].queue.insert(vertex_sp(2, 0.9));
    samplings[0].queue.insert(vertex_sp(3, 0.7));
    samplings[0].queue.insert(vertex_sp(6, 2.8));
    samplings[0].queue.insert(vertex_sp(7, 1.6));
    samplings[0].queue.insert(vertex_sp(8, 1.6));
    std::set<vertex_sp, sampling::compare_sp>::iterator it = samplings[0].queue.find(vertex_sp(2, 0.6));
    cout << it->id << " " << it->sp << endl;
    
    return 0;
}
