#include "MPIMassOperationEmulator.h"
#include "TopologyGraphModule.h"

extern _GRAPH_EDGES	 _edges;
extern _GRAPH_PATHES _pathes;

#define __foreach(it, i, c) for(it i = c.begin(), _end_##i = c.end(); i != _end_##i; ++i)
// =======================================================================================================================================================================================================================
// Internal service code

// Some kind of barrier synchronization mechanism 

typedef unsigned int flag_t;
typedef std::vector<flag_t> alacv_t;

static alacv_t _alacrity_vector;

#define FLAG_TYPE_BIT_COUNT (sizeof(flag_t) << 3)

inline void alacv_init(int comm_size)
{
	_ASSERT(comm_size > 0);
	_alacrity_vector = alacv_t(comm_size / FLAG_TYPE_BIT_COUNT + 1, 0x00000000U);
	_alacrity_vector.shrink_to_fit();
}
inline void alacv_release() 
{ 
	_alacrity_vector.clear(); 
	_alacrity_vector.shrink_to_fit(); 
}

#define __ROTATE_LEFT(x, n)  (((x) << (n)) | ((x) >> ((sizeof(x) << 3)-(n))))
#define __ROTATE_RIGHT(x, n) (((x) >> (n)) | ((x) << ((sizeof(x) << 3)-(n))))

inline int get_alacv_index(int rank) { return rank / FLAG_TYPE_BIT_COUNT; }
inline int get_fbit_offset(int rank) { return rank % FLAG_TYPE_BIT_COUNT; }
inline void alacv_set(int rank)
{
	_ASSERT(rank > 0);	

	int index  = get_alacv_index(rank),
		offset = get_fbit_offset(rank);

	_ASSERT(index < _alacrity_vector.size() && offset < FLAG_TYPE_BIT_COUNT);

	flag_t mask = static_cast<flag_t>(__ROTATE_LEFT(1, offset));
	_alacrity_vector[index] |= mask;
}

inline void alacv_unset(int rank)
{
	_ASSERT(rank > 0);	

	int index  = get_alacv_index(rank),
		offset = get_fbit_offset(rank);

	_ASSERT(index < _alacrity_vector.size() && offset < FLAG_TYPE_BIT_COUNT);

	flag_t mask = static_cast<flag_t>(__ROTATE_LEFT(-2, offset));
	_alacrity_vector[index] &= mask;
}

inline void alacv_get_nodes(std::vector<int> &node_set, bool ready)
{
	node_set.clear();
	for(int i = 0, max_i = _alacrity_vector.size(); i < max_i; ++i)
	{
		for (int j = 0; j < FLAG_TYPE_BIT_COUNT; ++j)
		{
			if ((_alacrity_vector[i] | (__ROTATE_LEFT(1, j))) == ready) node_set.push_back(j * FLAG_TYPE_BIT_COUNT + j);
		}
	}
	node_set.shrink_to_fit();
}

// End of internal service code
// =======================================================================================================================================================================================================================

inline int OWN_Send(void* buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm) {
	int rank;
	MPI_Comm_rank(comm, &rank);
	
	MPI_Send(buf, count, datatype, _pathes[rank][dest].at(1), tag, comm);
}

int __stdcall OWN_Alltoall(void* sendbuf, int sendcount, MPI_Datatype sendtype, void* recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm) {
	int ret_result = 0;
	
	int rank, size;
	if ((ret_result = MPI_Comm_size(comm, &size)) != MPI_SUCCESS) return ret_result; 
	if ((ret_result = MPI_Comm_rank(comm, &rank)) != MPI_SUCCESS) return ret_result;

	alacv_init(size);

	// Broadcast will be here

	alacv_release();

	return ret_result;
}