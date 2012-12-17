#include "TopologyGraphModule.h"

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
int generateTopologyGraph(_GRAPH_EDGES &edges, size_t n, _TOPOLOGY topology) {
	edges.resize(n);
	int i, j;

	switch (topology) {
	case CIRCLE:
		edges[0].push_back(1);
		edges[0].push_back(n);
		for (i = 1; i < n - 1; ++i) {
			edges[i].push_back(i - 1);
			edges[i].push_back(i + 1);
		}
		edges[n - 1].push_back(n - 2);
		edges[n - 1].push_back(0);
		break;

	case GRID:
		if (n % 2 != 0)
			return -1;

		int height	= 2,
			width	= n / 2;

		// Finging optimal grid sides
		for (i = height + 1; i < width; ++i) {
			if (n % i == 0) {
				height	= i;
				width	= n / height;
			}
		}

		for (i = 0; i < height * width; ++i) {
			if (i - 1 > 0)		edges[i].push_back(i - 1);
			if (i - width > 0)	edges[i].push_back(i - width);
			if (i + 1 < n)		edges[i].push_back(i + 1);	
			if (i + width < n)	edges[i].push_back(i + width);
		}
		break;

	case HYPERCUBE:
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