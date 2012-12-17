
#include "mpi_extension.h"

#define __foreach(it, i, c) for(it i = c.begin(), _end = c.end(); i != _end; i++)

void AdjacencyDataConversion(const _GRAPH_EDGES &adj, std::vector<std::size_t> &index, std::vector<std::size_t> &edges)
{
	for (std::size_t i = 0, max_i = adj.size(); i < max_i; ++i)
	{
		index.push_back(i == 0 ? adj[i].size() : index.back() + adj[i].size());
		edges.insert(edges.end(), adj[i].begin(), adj[i].end());
	}
}