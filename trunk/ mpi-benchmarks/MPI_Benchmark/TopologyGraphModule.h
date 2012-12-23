#pragma once

enum _TOPOLOGY {
	TOPOLOGY_UNKNOWN,
	TOPOLOGY_CIRCLE,
	TOPOLOGY_GRID,
	TOPOLOGY_HYPERCUBE
};

typedef vector < vector <int> > _GRAPH_EDGES;
typedef vector < int > _GRAPH_PATH;
typedef vector < vector < _GRAPH_PATH > > _GRAPH_PATHES;

void AdjacencyDataConversion(const _GRAPH_EDGES __in &adj, vector<int> __out &index, vector<int> __out &edges);

/*
* Generates all links between graph nodes
*
* n			- number of nodes
* topology	- topology to use
* edges		- edges structure
*
* returns:
* -1	- wgorng nodes number for such topology
*  0	- OK
*/
int GenerateTopologyGraph(int __in n, _TOPOLOGY __in topology, _GRAPH_EDGES __out &edges);

/*
* Traces pathes between every node pair
* (including both nodes)
*
* edges  - edges structure
* pathes - resulting pathes
*/
void TraceAllGraphPathes(_GRAPH_EDGES __in edges, _GRAPH_PATHES __out &pathes);