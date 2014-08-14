#include <graphlab.hpp>
using namespace graphlab;

struct web_page {
    std::string pagename;
    double pagerank;
    web_page():pagerank(0.0) { }
    explicit web_page(std::string name):pagename(name),pagerank(0.0){ }

    void save(graphlab::oarchive& oarc) const {
        oarc << pagename << pagerank;
    }

    void load(graphlab::iarchive& iarc) {
        iarc >> pagename >> pagerank;
    }
};

typedef graphlab::distributed_graph<web_page, graphlab::empty> graph_type;

bool line_parser(graph_type& graph, const std::string& filename, 
                 const std::string& textline) {
  std::stringstream strm(textline);
  graphlab::vertex_id_type vid;
  std::string pagename;
  // first entry in the line is a vertex ID
  strm >> vid;
  strm >> pagename;
  // insert this web page
  graph.add_vertex(vid, web_page(pagename));
  // while there are elements in the line, continue to read until we fail
  while(1){
    graphlab::vertex_id_type other_vid;
    strm >> other_vid;
    if (strm.fail())
      return true;
    graph.add_edge(vid, other_vid);
  }
  return true;
}

int main(int argc, char** argv) {
    mpi_tools::init(argc, argv);
    distributed_control dc;
      
    graph_type graph(dc);
    graph.load("graph.txt", line_parser);
    graph.finalize();

    dc.cout() << "Hello World!\n";

    mpi_tools::finalize();
}
