#include <graphlab.hpp>
#include <string>
#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <set>
#include <map>
#include <vector>
using namespace std;

typedef graphlab::vertex_id_type vid_t; // Vertex id
typedef graphlab::empty vertex_type;

/* Edge type and vertex type definition */
class edge_type{
public:
    float w; // weight 
    float p; // probability of existence 
    void save(graphlab::oarchive& oarc) const { oarc << w << p; }
    void load(graphlab::iarchive& iarc) { iarc >> w >> p; }
};
typedef graphlab::distributed_graph<vertex_type, edge_type> graph_type;

/* Message tyep */
class message_type : public set<int> {
public:
    message_type():set<int>(){}
    message_type(int iter) { this->insert(iter); }
    message_type & operator+=(const message_type & rhs) { 
        this->insert(rhs.begin(), rhs.end());
        return *this;
    }
    void save(graphlab::oarchive& oarc) const { 
//        for(message_type::iterator it = this->begin(); it != this->end(); it++)
//            oarc << (int)*it;
    }
    void load(graphlab::iarchive& iarc) { 
//        for(message_type::iterator it = this->begin(); it != this->end(); it++)
//            iarc >> (int)*it;
    }
};
typedef graphlab::empty gather_type;


class sampling {
public:
    std::set<pair<float, vid_t> > prior_queue; // set are ordered in non-decreasing order
    std::set<vid_t> visited_ver;
    std::map<vid_t, float> short_path;
    
    static std::map<vid_t, pair<float, int> > result;
};
typedef pair<float, vid_t> vsp;
std::map<vid_t, pair<float, int> > sampling::result;

/* Input parameter: source vertex id */
vid_t src;
bool directed;
std::vector<sampling> samplings;

void add_samp() {
    sampling s;
    s.prior_queue.insert(vsp(0, src));
    s.short_path[src] = 0;
    s.visited_ver.clear();
    samplings.push_back(s);
}

bool line_parser(graph_type& graph, const std::string& filename, 
                const std::string& textline) {
    const char* str= textline.data();
    vid_t vid, other_vid;
    edge_type edata;
    sscanf(str, "%u   %u  %f  %f", (uint*)&vid, (uint*)&other_vid, &edata.w, &edata.p);
    
    graph.add_vertex(vid); // Don't forget to add vertex
    graph.add_vertex(other_vid);
    graph.add_edge(vid, other_vid, edata);

    return true;
}


class kNN_program : 
    public graphlab::ivertex_program<graph_type, gather_type, message_type>,
    public graphlab::IS_POD_TYPE {
private:
    //sampling* p_cur_samp;
    set<int> idx;
    vid_t id;
    //float sp;

    inline vid_t get_other_vid(const edge_type& edge, const vertex_type& vertex) const {
        if(directed)
            return edge.target().id();
        else
            return vertex.id() == edge.source().id()? edge.target().id() : edge.source().id();
    }
public:
    void init(icontext_type & context, const vertex_type & vertex, const message_type & msg) {
        id = vertex.id();
        idx = msg;
      //  p_cur_samp = &samplings[msg.i];
      //  sp = p_cur_samp->short_path[id];
        for(set<int>::iterator it = idx.begin(); it != idx.end(); it++) {
            sampling* p_cur_samp = &samplings[*it];
            p_cur_samp->visited_ver.insert(id);
            p_cur_samp->prior_queue.erase(p_cur_samp->prior_queue.begin());
        }
    }

    edge_dir_type gather_edges(icontext_type & context, const vertex_type & vertex) const {
        if(directed)
            return graphlab::OUT_EDGES;
        else
            return graphlab::ALL_EDGES;
    }

    gather_type gather(icontext_type& context, const vertex_type& vertex, edge_type& edge) const { 
        for(set<int>::iterator it = idx.begin(); it != idx.end(); it++) {
            sampling* p_cur_samp = &samplings[*it];
            float sp = p_cur_samp->short_path[id];

            float rand_val = rand() / (float)RAND_MAX; // Generate a randome value between 0 and 1

            if(rand_val < edge.data().p) {
                vid_t other_vid = get_other_vid(edge, vertex);
                float other_sp = sp + edge.data().w;
                if(p_cur_samp->visited_ver.find(other_vid) == p_cur_samp->visited_ver.end()) {
                    // vertex other_vid not visited
                    if(p_cur_samp->short_path.find(other_vid) == p_cur_samp->short_path.end()) { // not in priority queue and short_path
                        p_cur_samp->prior_queue.insert(vsp(other_sp, other_vid));
                        p_cur_samp->short_path[other_vid] = other_sp;
                    }
                    else if(other_sp < p_cur_samp->short_path[other_vid]) { // update with minimum shortest path value
                        p_cur_samp->prior_queue.erase(p_cur_samp->prior_queue.find(vsp(p_cur_samp->short_path[other_vid], other_vid)));
                        p_cur_samp->prior_queue.insert(vsp(other_sp, other_vid));
                        p_cur_samp->short_path[other_vid] = other_sp;
                    }
                }
            }
        }

        return gather_type();
    }

    void apply(icontext_type& context, vertex_type& vertex, const gather_type & total) {
        float sum = 0;
        for(set<int>::iterator it = idx.begin(); it != idx.end(); it++) {
            sum += samplings[*it].short_path[id];
        }

        if(sampling::result.find(id) != sampling::result.end()) {
            pair<float, int> tmp = sampling::result[id];
            tmp.first += sum;   
            tmp.second += idx.size();
            sampling::result[id] = tmp;
        }
        else {
            sampling::result[id] = pair<float, int>(sum, idx.size());
        }

    }

    void scatter(icontext_type & context, const vertex_type & vertex, edge_type & edge)const { }

};


int main(int argc, char** argv) {
    graphlab::mpi_tools::init(argc, argv);
    graphlab::distributed_control dc;

    std::string filename;
    uint k = 1;
    uint nsamp = 200;
    directed = false;
    std::string exec_type = "synchronous";
   // int window_size= nsamp;
    float dist = 0;
    float dist_det = 0.4;
    /* Parse input parameters */
    graphlab::command_line_options clopts("Welcome to probabilistic kNN");
    clopts.attach_option("file", filename, "The input filename (required)");
    clopts.attach_option("src", src, "source vertex id");
    clopts.attach_option("k", k, "number of nearest neighbors");
    clopts.attach_option("nsamp", nsamp, "number of samples");
    clopts.attach_option("dir", directed, "directed (1) or undirected (0) graph");
    // clopts.attach_option("window", window_size, "window size");
    clopts.attach_option("dist_det", dist_det, "distance increment");
    clopts.attach_option("engine", exec_type, "The engine type synchronous or asynchronous");
    clopts.parse(argc, argv);
   // if(!clopts.is_set("window")) {
   //     window_size = nsamp;
   // }

    graph_type graph(dc);
    graph.load(filename, line_parser);
    graph.finalize();
    graphlab::omni_engine<kNN_program> engine(dc, graph, exec_type, clopts);
    srand(time(NULL));

    bool finished = false;

    for(uint i = 0; i < nsamp; i++) {
        add_samp();
    }

    while(!finished) {
        //int window_count = 0;
        dist += dist_det;
        /* Run one iteration of vertex program for all samplings */
        for(std::vector<sampling>::size_type i = 0; i < samplings.size(); i++) {
            if(!samplings[i].prior_queue.empty() && samplings[i].prior_queue.begin()->first < dist) {
                vid_t next_vid = samplings[i].prior_queue.begin()->second;
                engine.signal(next_vid, message_type(i));
                //window_count++;
            }
        }
        engine.start();

        /* TO-DO 
        * 1) Convergence estimation
        * 2) Initialize new samples
        * 3) Aggregating results
        */
        if(sampling::result.size() >= k + 1) {
            finished = true;
        }
        //else if(window_count < window_size && samplings.size() < nsamp) {
        //    add_samp();
        //}
    }

    for(std::map<vid_t, pair<float, int> >::iterator it = sampling::result.begin(); it != sampling::result.end(); it++) {
        if(it->first != src)
            dc.cout() << "(" << it->first << ", " << it->second.first / (float)it->second.second << ") ";
    }
    dc.cout() << "\n";

    graphlab::mpi_tools::finalize();
    return 0;
}
