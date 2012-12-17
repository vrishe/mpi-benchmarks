#pragma once

enum _TOPOLOGY {
	CIRCLE,
	GRID,
	HYPERCUBE
};

typedef vector < vector <size_t> > _GRAPH_EDGES;

int generateTopologyGraph(_GRAPH_EDGES &edges, size_t n, _TOPOLOGY topology);