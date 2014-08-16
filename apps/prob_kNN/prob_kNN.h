#include <set>
#include <vector>

typedef graphlab::vertex_id_type vid_t; // Vertex id

/* Vertex set records the expected reliability of each vertex */
struct vertex_value {
    float sum; // sum of all shortest path length
    int count; // counter of shortest path reached
};

struct compare_vertex_value {
    bool operator () (const vertex_value& v1, const vertex_value& v2) {
        return v1.sum / v1.count > v2.sum / v2.count;
    }
};
typedef std::set<vertex_value, compare_vertex_value> vertex_set;

struct vertex_sp {
    vid_t id;
    float sp; // Shortest path in current sampling
    vertex_sp(vid_t a, float b){id = a; sp = b;}
};

/* Sampling vector stores Dijkstra samplings */
struct sampling {
    struct compare_sp {
        bool operator () (const vertex_sp& v1, const vertex_sp& v2) {
            return v1.sp > v2.sp;
        }
    };
    
    std::set<vertex_sp, compare_sp> queue;
    std::set<vid_t> visited;
};
typedef std::vector<sampling> sampling_vector;

/* Edge type and vertex type definition */
struct edge_type{
    float p; // probability of existence 
    float w; // weight 

    void save(graphlab::oarchive& oarc) const {
        oarc << p << w;
    }

    void load(graphlab::iarchive& iarc) {
        iarc >> p >> w;
    }
};
typedef graphlab::empty vertex_type;
typedef graphlab::distributed_graph<vertex_type, edge_type> graph_type;

/* Message tyep */
class message_type {
public:
    int i; // index of iteration
    float s; // shortest path from source to endpoint, including the weight of current edge

    message_type & operator+=(const message_type & rhs) {
        return *this;
    }

    void save(graphlab::oarchive& oarc) const {
        oarc << i << s;
    }

    void load(graphlab::iarchive& iarc) {
        iarc >> i >> s;
    }

};

class gather_type : public std::vector<vertex_sp> {
public:
    gather_type & operator+=(const gather_type & rhs) {
        this->insert(this->end(), rhs.begin(), rhs.end());
        return *this;
    }

    void save(graphlab::oarchive& oarc) const {
        for(uint i = 0; i < this->size(); i++)
            oarc << this->at(i).id << this->at(i).sp;
    }

    void load(graphlab::iarchive& iarc) {
         for(uint i = 0; i < this->size(); i++)
            iarc >> this->at(i).id >> this->at(i).sp;
    }

};
