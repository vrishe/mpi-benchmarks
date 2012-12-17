
#ifndef _ADJDATA_H
#define _ADJDATA_H

#include <vector>

typedef std::vector<std::vector<std::size_t>> _GRAPH_EDGES;

void AdjacencyDataConversion(const _GRAPH_EDGES __in &adj, std::vector<std::size_t> __out &index, std::vector<std::size_t> __out &edges);

#endif