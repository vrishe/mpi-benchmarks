#include "TopologyGraphModule.h"

void AdjacencyDataConversion(const _GRAPH_EDGES __in &adj, vector<size_t> __out &index, vector<size_t> __out &edges) {
	for (std::size_t i = 0, max_i = adj.size(); i < max_i; ++i) {
		index.push_back(i == 0 ? adj[i].size() : index.back() + adj[i].size());
		edges.insert(edges.end(), adj[i].begin(), adj[i].end());
	}
}

int GenerateTopologyGraph(size_t __in n, _TOPOLOGY __in topology, _GRAPH_EDGES __out &edges) {
	edges.resize(n);
	size_t i;

	switch (topology) {
	case TOPOLOGY_CIRCLE: {
		edges[0].push_back(1);
		edges[0].push_back(n - 1);
		for (i = 1; i < n - 1; ++i) {
			edges[i].push_back(i - 1);
			edges[i].push_back(i + 1);
		}
		edges[n - 1].push_back(n - 2);
		edges[n - 1].push_back(0);
		break;
	}

	case TOPOLOGY_GRID: {
		if (n % 2 != 0)
			return -1;

		size_t height	= 2,
			width	= n / 2;

		// Finging optimal grid sides
		for (i = height + 1; i < width; ++i) {
			if (n % i == 0) {
				height	= i;
				width	= n / height;
			}
		}

		for (i = 0; i < height * width; ++i) {
			if ((i - 1 >= width * (i / width)) && (i - 1 < i))		edges[i].push_back(i - 1);
			if (i - width < i)										edges[i].push_back(i - width);
			if ((i + 1 < width * (i / width + 1)) && (i + 1 > i))	edges[i].push_back(i + 1);	
			if ((i + width < n) && (i + width > i))					edges[i].push_back(i + width);
		}
		break;
	}

	case TOPOLOGY_HYPERCUBE: {
		if (n != 16)
			return -1;

		edges[0].push_back(1);
		edges[0].push_back(3);
		edges[0].push_back(4);
		edges[0].push_back(8);
		
		edges[1].push_back(0);
		edges[1].push_back(2);
		edges[1].push_back(5);
		edges[1].push_back(9);
		
		edges[2].push_back(1);
		edges[2].push_back(3);
		edges[2].push_back(6);
		edges[2].push_back(10);

		edges[3].push_back(0);
		edges[3].push_back(2);
		edges[3].push_back(4);
		edges[3].push_back(11);
		
		edges[4].push_back(5);
		edges[4].push_back(7);
		edges[4].push_back(0);
		edges[4].push_back(12);
		
		edges[5].push_back(4);
		edges[5].push_back(6);
		edges[5].push_back(1);
		edges[5].push_back(13);

		edges[6].push_back(5);
		edges[6].push_back(7);
		edges[6].push_back(2);
		edges[6].push_back(14);

		edges[7].push_back(4);
		edges[7].push_back(6);
		edges[7].push_back(3);
		edges[7].push_back(15);

		edges[8].push_back(9);
		edges[8].push_back(11);
		edges[8].push_back(12);
		edges[8].push_back(0);

		edges[9].push_back(8);
		edges[9].push_back(10);
		edges[9].push_back(13);
		edges[9].push_back(1);

		edges[10].push_back(9);
		edges[10].push_back(11);
		edges[10].push_back(14);
		edges[10].push_back(2);

		edges[11].push_back(8);
		edges[11].push_back(10);
		edges[11].push_back(15);
		edges[11].push_back(3);

		edges[12].push_back(13);
		edges[12].push_back(15);
		edges[12].push_back(8);
		edges[12].push_back(4);

		edges[13].push_back(12);
		edges[13].push_back(14);
		edges[13].push_back(9);
		edges[13].push_back(5);

		edges[14].push_back(13);
		edges[14].push_back(15);
		edges[14].push_back(10);
		edges[14].push_back(6);

		edges[15].push_back(12);
		edges[15].push_back(14);
		edges[15].push_back(11);
		edges[15].push_back(7);

		break;
	}
	}

	return 0;
}

void TracePath(_GRAPH_EDGES __in edges, size_t __in from, size_t __in to, _GRAPH_PATH __out &path) {
	if (from == to) {
		path.push_back(0);
		path.push_back(0);
		return;
	}

	size_t n = edges.size();
	queue < size_t > s;
	bool found = false;
	vector < int > visited(n);
	for (size_t i = 0; i < n; ++i)
		visited[i] = -1;

	s.push(from);
	visited[from] = from;
	while (!s.empty() && !found) {
		size_t cur = s.front();
		
		for each (size_t node in edges.at(cur)) {
			if (visited.at(node) == -1) {
				visited[node] = cur;
				s.push(node);
				if (node == to) {
					found = true;
					break;
				}
			}
		}

		s.pop();
	}

	if (found) {
		path.push_back(to);
		while (to != from) {
			to = visited[to];
			path.insert(path.begin(), to);
		}
	}
}

void TraceAllGraphPathes(_GRAPH_EDGES __in edges, _GRAPH_PATHES __out &pathes) {
	size_t n = edges.size();
	pathes.resize(n);

	for (size_t i = 0; i < n; ++i) {
		pathes[i].resize(n);
		for (size_t j = 0; j < n; ++j)
			TracePath(edges, i, j, pathes[i][j]);
	}
}