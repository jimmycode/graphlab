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

/* Sampling vector stores Dijkstra samplings */
struct sampling {
    struct vertex_sp {
        vid_t id;
        float sp; // Shortest path in current sampling
    };

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
struct message_type {
    int i; // index of iteration
    float s; // shortest path from source to endpoint, including the weight of current edge
};

class gather_type : public std::vector<vid_t> {
public:
    gather_type & operator+=(const gather_type & rhs) {
        this->insert(this->end(), rhs.begin(), rhs.end());
        return *this;
    }
};
