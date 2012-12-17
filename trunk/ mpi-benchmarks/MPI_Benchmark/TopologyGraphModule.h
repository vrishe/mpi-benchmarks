#pragma once

enum _TOPOLOGY {
	TOPOLOGY_CIRCLE,
	TOPOLOGY_GRID,
	TOPOLOGY_HYPERCUBE
};

typedef vector < vector <size_t> > _GRAPH_EDGES;
typedef vector < size_t > _GRAPH_PATH;
typedef vector < vector < _GRAPH_PATH > > _GRAPH_PATHES;

void AdjacencyDataConversion(const _GRAPH_EDGES __in &adj, vector<size_t> __out &index, vector<size_t> __out &edges);

/*
* Generates all links between graph nodes
*
* edges		- out edges structure
* n			- number of nodes
* topology	- topology to use
*
* returns:
* -1	- wgorng nodes number for such topology
*  0	- OK
*/
int GenerateTopologyGraph(size_t __in n, _TOPOLOGY __in topology, _GRAPH_EDGES __out &edges);

void TraceAllGraphPathes(_GRAPH_EDGES __in edges, _GRAPH_PATHES __out &pathes);

void TracePath(size_t __in from, size_t __in to, _GRAPH_PATH __out &path);