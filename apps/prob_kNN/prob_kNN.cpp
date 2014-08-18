#include <graphlab.hpp>
#include <string>
#include <iostream>
#include <stdlib.h>
#include <time.h>
#include <set>
#include <vector>

typedef graphlab::vertex_id_type vid_t; // Vertex id

/* Vertex set records the expected reliability of each vertex */
struct vertex_value {
    vid_t id;
    float sum; // sum of all shortest path length
    int count; // counter of shortest path reached
};

struct compare_vertex_value {
    bool operator () (const vertex_value& v1, const vertex_value& v2) {
        if(v1.id != v2.id)
            return v1.sum / v1.count > v2.sum / v2.count;
        else
            return false;
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
            if(v1.id != v2.id)
                return v1.sp > v2.sp;
            else
                return false;
        }
    };
    
    std::set<vertex_sp, compare_sp> queue;
    std::set<vid_t> visited;
};
typedef std::vector<sampling> sampling_vector;
typedef std::set<vertex_sp, sampling::compare_sp> sampling_priq; // priority queue

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

typedef graphlab::empty gather_type;

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
    strm >> vid >> other_vid >> edata.w >> edata.p;
    
    graph.add_vertex(vid);
    graph.add_vertex(other_vid);
    graph.add_edge(vid, other_vid, edata);

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

    edge_dir_type gather_edges(icontext_type & context, const vertex_type & vertex) const {
        return graphlab::ALL_EDGES;
    }

    gather_type gather(icontext_type& context, const vertex_type& vertex, edge_type& edge) const { 
        srand(time(NULL));
        float rand_val = rand() / (float)RAND_MAX;       
        if(rand_val < edge.data().p && samplings[iter].visited.find(edge.source().id()) == samplings[iter].visited.end()) {
        /* random value satisfies and is not already visited */
            vertex_sp v = vertex_sp(edge.source().id(), sp + edge.data().w);
            sampling_priq::iterator it = samplings[iter].queue.find(v);
            if(it == samplings[iter].queue.end() || v.sp < it->sp)
                samplings[iter].queue.insert(v);
        }

        return gather_type();
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
    }

    void scatter(icontext_type & context, const vertex_type & vertex, edge_type & edge)const { }

};

void add_samp() {
    sampling s;
    s.queue.insert(vertex_sp(src, 0));
    s.visited.clear();
    samplings.push_back(s);
}

int main(int argc, char** argv) {
    graphlab::mpi_tools::init(argc, argv);
    graphlab::distributed_control dc;

    std::string filename;
    int k = 1;
    uint nsamp = 200;
    std::string exec_type = "synchronous";
    int window_size= 10;
    /* Parse input parameters */
    graphlab::command_line_options clopts("Welcome to probabilistic kNN");
    clopts.attach_option("file", filename, "The input filename (required)");
    clopts.attach_option("src", src, "source vertex id");
    clopts.attach_option("k", k, "number of nearest neighbors");
    clopts.attach_option("nsamp", nsamp, "number of samples");
    clopts.attach_option("window", window_size, "window size");
    clopts.attach_option("engine", exec_type, "The engine type synchronous or asynchronous");
    clopts.parse(argc, argv);

    graph_type graph(dc);
    graph.load(filename, line_parser);
    graph.finalize();
    graphlab::omni_engine<kNN_program> engine(dc, graph, exec_type, clopts);

    bool finished = false;
    add_samp();
    while(!finished) {
        int window_count = 0;
        /* Run one iteration of vertex program for all samplings */
        for(sampling_vector::size_type i = 0; i < samplings.size(); i++) {
            if(!samplings[i].queue.empty()) {
                sampling_priq::iterator it = samplings[i].queue.begin();
                vid_t next_vid = it->id;
                engine.signal(next_vid, message_type(i, it->sp));
                samplings[i].visited.insert(next_vid);
                samplings[i].queue.erase(it);
                window_count++;
            }
        }
        engine.start();

        /* TO-DO 
        * 1) Convergence estimation
        * 2) Initialize new samples
        * 3) Aggregating results
        */
        if(window_count < window_size && samplings.size() < nsamp) {
            add_samp();
        }
        else if(samplings.size() >= nsamp && window_count == 0) {
            finished = true;
        }
        
        for(vertex_set::iterator it = vertex_log.begin(); it != vertex_log.end(); it++) {
            dc.cout() << "(" << it->id << ", " << it->sum / it->count << ") ";
        }
        dc.cout() << "\n";
    }

    graphlab::mpi_tools::finalize();
    return 0;
}
