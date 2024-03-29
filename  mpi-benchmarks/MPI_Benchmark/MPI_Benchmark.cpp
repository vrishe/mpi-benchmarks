// MPI_Benchmark.cpp: ���������� ����� ����� ��� ����������� ����������.
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
#define REPS_UNKNOWN	 (0)
#define REPS_DEFAULT	 (1)

inline static _TOPOLOGY test_topology_arg(_TCHAR *arg, std::size_t n_chars)
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

inline static int test_mode_arg(_TCHAR *arg)
{
	int mode = MODE_UNKNOWN;
	if (arg != NULL)
	{
		_TCHAR *mode_id[] = { _T("own"), _T("o"), _T("lib"), _T("l") };
		bool error_state = *arg != '/';

		if (!error_state)
		{
			if      (_tcscmp(mode_id[0], arg + 1) == 0 || _tcscmp(mode_id[1], arg + 1) == 0) mode = MODE_OWN;
			else if (_tcscmp(mode_id[2], arg + 1) == 0 || _tcscmp(mode_id[3], arg + 1) == 0) mode = MODE_LIB;
		}
	}
	return mode;
}

inline static int test_reps_arg(_TCHAR *arg, std::size_t n_chars)
{
	int mode = REPS_UNKNOWN;
	if (arg != NULL)
	{
		_TCHAR *reps_id[] = { _T("c") };

		std::size_t reps_id_len = 0;
		bool error_state = *arg != '-' || (_tcsncmp(reps_id[0], arg + 1, reps_id_len = min(_tcslen(reps_id[0]), n_chars - 2)) != 0) || *(arg + (++reps_id_len)++) != _T(':');

		if (!error_state) mode = _ttoi(arg + reps_id_len);
	}
	return mode;
}

typedef int (__stdcall *INTERCHANGER) (void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm);

int _tmain(int argc, _TCHAR* argv[])
{
	int size = 0,
		rank = -1;

	try {
		_TOPOLOGY topology	= TOPOLOGY_DEFAULT;
		int       mode		= MODE_DEFAULT;
		int       reps		= REPS_DEFAULT;

		for (size_t i = 1, max_i = static_cast<size_t>(argc); i < max_i; ++i)
		{
			size_t argv_str_len = _tcslen(argv[i]);
			for (std::size_t j = 0; j < argv_str_len; ++j) argv[i][j] = _totlower(argv[i][j]);

			if (topology == TOPOLOGY_DEFAULT) { _TOPOLOGY topology_param = test_topology_arg(argv[i], argv_str_len); if (topology_param != TOPOLOGY_UNKNOWN) { topology = topology_param; continue; } }
			if (mode     == MODE_DEFAULT)     { int       mode_param	 = test_mode_arg(argv[i]);					 if (mode_param != MODE_UNKNOWN)		 { mode = mode_param; continue; } }
			if (reps	 == REPS_DEFAULT)	  { int		  reps_param	 = test_reps_arg(argv[i], argv_str_len);	 if (reps_param > REPS_UNKNOWN)			 { reps = reps_param; continue; } }

			throw std::string("Unknown parameter: ").append(argv[i]);
		}
		if (topology == TOPOLOGY_UNKNOWN || mode == MODE_UNKNOWN || reps <= REPS_UNKNOWN) {	throw std::string("Program parameters are wrong!");	}

		MPI_Datatype error_code = 0;
		if ((error_code = MPI_Init(&argc, &argv)) != MPI_SUCCESS) { throw std::string("There was a problem with MPI initialization!"); }

		// Obtaining of necessary self-descriptive data
		MPI_Comm_size(MPI_COMM_WORLD, &size); 
		MPI_Comm_rank(MPI_COMM_WORLD, &rank);

		// Obtaining of necessary environment-decriptive data
		if ((error_code = GenerateTopologyGraph(size, topology, _edges)) != 0) { throw std::string("Failed to generate topology graph on such a number of nodes!"); }

		MPI_Comm	 MPI_Comm_new = NULL; 
		INTERCHANGER FNC_AllToAll = NULL;
		// Program flow fork
		switch (mode)
		{
		case -1:	
			{
				FNC_AllToAll = OWN_Alltoall;
				MPI_Comm_new = MPI_COMM_WORLD;

				TraceAllGraphPathes(_edges, _paths);

				MPI_Barrier(MPI_COMM_WORLD);

				_tcout << time(NULL) << " @ process: " << rank << std::endl;
			}
			break;

		case 1:		
			{
				FNC_AllToAll = MPI_Alltoall;

				vector<int> indices, edges;
				AdjacencyDataConversion(_edges, indices, edges);
				if ((error_code = MPI_Graph_create(
					MPI_COMM_WORLD, 
					_edges.size(), 
					reinterpret_cast<int*>(indices.begin()._Ptr), 
					reinterpret_cast<int*>(edges.begin()._Ptr), 
					true, 
					&MPI_Comm_new
				)) != MPI_SUCCESS) { throw std::string("Failed to create topology communicator with the data provided!"); }
			}
			break;
		}

		if (FNC_AllToAll == NULL) // !=
		{	
			int *send_buffer = new int[size],
				*recv_buffer = new int[size];

			std::fill_n(send_buffer, size, rank);
			std::fill_n(recv_buffer, size, 0x00);

			double start_time = MPI_Wtime();
			for (int i = 0; i < reps; ++i) 
			{
				if ((error_code = FNC_AllToAll(send_buffer, 1, MPI_INT, recv_buffer, 1, MPI_INT, MPI_Comm_new)) != MPI_SUCCESS)
				{
					throw std::string("Failed to complete All-to-All transmission!");
				}
			}
			double end_time = MPI_Wtime();

			_tcout << _T("Process #") << rank << _T(":");
			for (int i = 0; i < size; ++i) _tcout << " " << recv_buffer[i];
			_tcout << std::endl << "From " << start_time << " to " << end_time << std::endl;

			delete[] send_buffer;
			delete[] recv_buffer;
		}
	}
	catch(std::string error_message)
	{
		if (rank != -1) { _tcout << _T("Process #") << rank << _T(": "); }
		_tcout << error_message.begin()._Ptr << std::endl;
	}

	MPI_Barrier(MPI_COMM_WORLD);
	return MPI_Finalize();
}

