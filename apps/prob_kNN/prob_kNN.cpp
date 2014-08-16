#include <graphlab.hpp>
#include <string>
#include <stdlib.h>
#include "prob_kNN.h"

/* Input parameter: source vertex id */
vid_t src;
/* Set of visited vertices */
vertex_set visited;
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
    message_type msg;
public:
    void init(icontext_type & context, const vertex_type & vertex, const message_type & msg) {
        this->msg = msg;
    }

    gather_type gather(icontext_type& context, const vertex_type& vertex, edge_type& edge) const { 
        gather_type* tmp = new gather_type();
        float rand_val = rand() / (float)RAND_MAX;       
        if(rand_val < edge.data().p) {
            tmp->push_back(edge.source().id());
        }

        return *tmp;
    }

       
};

int main(int argc, char** argv) {
    graphlab::mpi_tools::init(argc, argv);
    graphlab::distributed_control dc;

    graph_type graph(dc);
    graph.load("test", line_parser);

    graphlab::mpi_tools::finalize();
}
