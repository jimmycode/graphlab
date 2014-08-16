#include <graphlab.hpp>
#include <string>
#include <iostream>
#include <stdlib.h>
#include <set>
#include <vector>

typedef graphlab::vertex_id_type vid_t; // Vertex id

/* Vertex set records the expected reliability of each vertex */
struct vertex_value {
    vid_t id;
    float sum; // sum of all shortest path length
    int count; // counter of shortest path reached

    bool operator== (const vertex_value& v) {
        return v.id == this->id;
    }
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

    message_type(){}
    message_type(int iter, float sp) {
        i = iter;
        s = sp;
    }

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

/* Input parameter: source vertex id */
vid_t src;
/* Log of visited vertices */
vertex_set vertex_log;
/* Vector of samplings */
sampling_vector samplings;

bool line_parser(graph_type& graph, const std::string& filename, 
                const std::string& textline) {
    std::stringstream strm(textline);
    vid_t vid, other_vid;
    edge_type edata;
    strm >> vid >> other_vid >> edata.p >> edata.w;

    return true;
}

class kNN_program : 
    public graphlab::ivertex_program<graph_type, gather_type, message_type>,
    public graphlab::IS_POD_TYPE {
private:
    float sp; int iter;
public:
    void init(icontext_type & context, const vertex_type & vertex, const message_type & msg) {
        sp = msg.s;
        iter = msg.i;
    }

    gather_type gather(icontext_type& context, const vertex_type& vertex, edge_type& edge) const { 
        gather_type* tmp = new gather_type();
        float rand_val = rand() / (float)RAND_MAX;       
        if(rand_val < edge.data().p) {
            tmp->push_back(vertex_sp(edge.source().id(), sp + edge.data().w));
        }

        return *tmp;
    }

    void apply(icontext_type& context, vertex_type& vertex, const gather_type & total) {
        vertex_value tmp;
        tmp.id = vertex.id();
        vertex_set::iterator it = vertex_log.find(tmp);

        if(it != vertex_log.end()) {
            tmp.sum = it->sum + sp;   
            tmp.count = it->count + 1;
            vertex_log.erase(it);
            vertex_log.insert(tmp);
        }
        else {
            tmp.sum = sp;
            tmp.count = 1;
            vertex_log.insert(tmp);
        }

        for(gather_type::size_type i = 0; i < total.size(); i++) {
            samplings[iter].queue.insert(total[i]);
        }
    }

};

int main(int argc, char** argv) {
    graphlab::mpi_tools::init(argc, argv);
    graphlab::distributed_control dc;

    std::string filename;
    int k = 1;
    int nsamp = 200;
    std::string exec_type = "synchronous";
    /* Parse input parameters */
    graphlab::command_line_options clopts("Welcome to probabilistic kNN");
    clopts.attach_option("file", filename, "The input filename (required)");
    clopts.attach_option("src", src, "source vertex id");
    clopts.attach_option("k", k, "number of nearest neighbors");
    clopts.attach_option("nsamp", nsamp, "number of samples");
    clopts.attach_option("engine", exec_type, "The engine type synchronous or asynchronous");
    clopts.parse(argc, argv);

    graph_type graph(dc);
    graph.load(filename, line_parser);
    graph.finalize();
    graphlab::omni_engine<kNN_program> engine(dc, graph, exec_type, clopts);

    bool converged = false;
    while(!converged) {
        /* Run one iteration of vertex program for all samplings*/
        for(sampling_vector::size_type i = 0; i < samplings.size(); i++) {
            if(!samplings[i].queue.empty()) {
                vid_t next_vid = samplings[i].queue.begin()->id;
                engine.signal(next_vid, message_type(i, samplings[i].queue.begin()->sp));
            }
        }
    }

    graphlab::mpi_tools::finalize();
}
