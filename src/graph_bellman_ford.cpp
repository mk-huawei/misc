#include <limits>
#include <vector>

using distance_t = std::size_t;
using distance_vector = std::vector<distance_t>;

template <typename T>
T infinity() {
    return std::numeric_limits<T>::max();
}

using vid_t = size_t;
static constexpr vid_t null_vid = infinity<vid_t>();
using vid_vector = std::vector<vid_t>;

template <typename Graph, typename WeightFunc>
distance_vector bellman_ford(Graph g, WeightFunc w, vid_t seed) {
    const auto& V = vertices(g);

    // init
    distance_vector d(size(V), infinity<distance_t>());
    vid_vector predecessor(size(V), null_vid);

    d[seed] = 0;
    // dla i od 1 do |V[G]| - 1 wykonaj
    for (const auto& _ : V) {
        //   dla każdej krawędzi (u,v) w E[G] wykonaj
        bool modified = false;
        for (const auto& e : edges(g)) {
            //     jeżeli d[v] > d[u] + w(u,v) to
            auto new_weight = d[source(e)] + w(e);
            if (new_weight < d[target(e)]) {
                //       d[v] = d[u] + w(u,v)
                d[target(e)] = new_weight;
                //       poprzednik[v] = u
                predecessor[target(e)] = source(e);

                modified = true;
            }
        }
        if (!modified) {
            // Optimization: don't repeat if no edge was relaxed in this
            // iteration.
            break;
        }
    }
}

int main() {
    graph g;
    bellman_ford(g, [](edge_reference_t e) { return 1; });
}
