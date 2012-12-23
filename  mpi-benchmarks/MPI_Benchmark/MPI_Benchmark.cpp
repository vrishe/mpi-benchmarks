// MPI_Benchmark.cpp: определяет точку входа для консольного приложения.
//

#include "TopologyGraphModule.h"
#include "MPIMassOperationEmulator.h"

_GRAPH_EDGES	_edges;
_GRAPH_PATHES	_paths;

#if (defined(UNICODE) || defined(_UNICODE))
#define _tcout std::wcout
#define _tcin  std::wcin
#else
#define _tcout std::cout
#define _tcin  std::cin
#endif

#define MODE_UNKNOWN      (0)
#define MODE_OWN		 (-1)
#define MODE_LIB		  (1)

#define TOPOLOGY_DEFAULT TOPOLOGY_UNKNOWN
#define MODE_DEFAULT	 MODE_LIB

_TOPOLOGY test_topology_arg(_TCHAR *arg, std::size_t n_chars)
{
	_TOPOLOGY topology = TOPOLOGY_UNKNOWN;
	if (arg != NULL)
	{
		_TCHAR *parameter_id[] = { _T("topology"), _T("t") };
		_TCHAR *topology_id[]  = { _T("grid"), _T("circle"), _T("hypercube") };

		std::size_t param_id_len = 0;
		bool error_state = *arg != '-' || (_tcsncmp(parameter_id[0], arg + 1, param_id_len = min(_tcslen(parameter_id[0]), n_chars - 2)) != 0 
								&& _tcsncmp(parameter_id[1], arg + 1, param_id_len =  min(_tcslen(parameter_id[1]), n_chars - 2)) != 0) || *(arg + (++param_id_len)++) != _T(':');
		if (!error_state)
		{
			if      (_tcscmp(topology_id[0], arg + param_id_len) == 0) topology = TOPOLOGY_GRID;
			else if (_tcscmp(topology_id[1], arg + param_id_len) == 0) topology = TOPOLOGY_CIRCLE;
			else if (_tcscmp(topology_id[2], arg + param_id_len) == 0) topology = TOPOLOGY_HYPERCUBE;
			else _tcout << _T("Topology ID specified is unknown!");
		}
	}
	return topology;
}

int test_mode_arg(_TCHAR *arg, std::size_t n_chars)
{
	int mode = MODE_UNKNOWN;
	if (arg != NULL)
	{
		_TCHAR *mode_id[] = { _T("own"), _T("o"), _T("lib"), _T("l") };
		bool error_state = *arg != '/';

		if      (_tcscmp(mode_id[0], arg + 1) == 0 || _tcscmp(mode_id[1], arg + 1) == 0) mode = MODE_OWN;
		else if (_tcscmp(mode_id[2], arg + 1) == 0 || _tcscmp(mode_id[3], arg + 1) == 0) mode = MODE_LIB;
	}
	return mode;
}

typedef int (__stdcall *INTERCHANGER) (void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm);

int _tmain(int argc, _TCHAR* argv[])
{
	_TOPOLOGY topology = TOPOLOGY_UNKNOWN;
	int       mode     = MODE_DEFAULT;

	for (size_t i = 1, max_i = static_cast<size_t>(argc); i < max_i; ++i)
	{
		size_t argv_str_len = _tcslen(argv[i]);
		for (std::size_t j = 0; j < argv_str_len; ++j) argv[i][j] = _totlower(argv[i][j]);

		if (topology == TOPOLOGY_UNKNOWN) { topology = test_topology_arg(argv[i], argv_str_len); continue; }
		if (mode     == MODE_DEFAULT)     { mode     = test_mode_arg(argv[i], argv_str_len);     continue; }

		_tcout << _T("Unknown parameter: ") << argv[i] << endl;
	}
	if (topology == TOPOLOGY_UNKNOWN || mode == MODE_UNKNOWN)
	{
		_tcout << _T("Program parameters are wrong!");
		return -1;
	}

	MPI_Datatype error_code = 0;
	if ((error_code = MPI_Init(&argc, &argv)) != MPI_SUCCESS) // MPI Initialization is here
	{
		_tcout << _T("There was a problem with MPI initialization!") << endl;
		return error_code;
	}

	// Obtaining of necessary self-descriptive data
	int size = 0;
	int	rank = -1;

	MPI_Comm_size(MPI_COMM_WORLD, &size); 
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);

	// Obtaining of necessary environment-decriptive data
	if ((error_code = GenerateTopologyGraph(size, topology, _edges)) != 0)
	{
		_tcout << _T("Failed to generate topology graph on such a number of nodes!") << endl;
		return error_code;
	}

	INTERCHANGER FNC_AllToAll = NULL;
	// Program flow fork
	switch (mode)
	{
	case -1:	
		{
			TraceAllGraphPathes(_edges, _paths);
			FNC_AllToAll = OWN_Alltoall;
		}
		break;

	case 1:		
		{
			if (rank == 0)
			{
				MPI_Comm mpi_comm_new; 
				vector<std::size_t> indices, edges;
				AdjacencyDataConversion(_edges, indices, edges);
				if ((error_code = MPI_Graph_create(
					MPI_COMM_WORLD, 
					_edges.size(), 
					reinterpret_cast<MPI_Datatype*>(indices.begin()._Ptr), 
					reinterpret_cast<MPI_Datatype*>(edges.begin()._Ptr), 
					true, 
					&mpi_comm_new
				)) != MPI_SUCCESS) {
					_tcout << _T("Failed to create topology communicator with the data provided!") << endl;
					return error_code;
				}
			}
			FNC_AllToAll = MPI_Alltoall;
		}
		break;
	}

	if (FNC_AllToAll != NULL)
	{	
		unsigned char *local_buffer = new unsigned char[size];

		// FNC_AllToAll will be called here!

		delete[] local_buffer;
	}

	return MPI_Finalize();
}

