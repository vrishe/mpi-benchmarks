#include "MPIMassOperationEmulator.h"
#include "TopologyGraphModule.h"

extern _GRAPH_EDGES	 _edges;
extern _GRAPH_PATHES _paths;

// =======================================================================================================================================================================================================================
// Internal service code

static MPI_Comm _comm_broadcast = NULL;

// Some kind of barrier synchronization mechanism 

typedef unsigned int flag_t;
typedef std::vector<flag_t> alacv_t;

static alacv_t _alacrity_vector;
static std::size_t _max_flag_count;

#define FLAG_TYPE_BIT_COUNT (sizeof(flag_t) << 3)

inline static void alacv_init(int comm_size)
{
	_ASSERT(comm_size > 0);
	_alacrity_vector = alacv_t(comm_size / FLAG_TYPE_BIT_COUNT + 1, 0U);
	_alacrity_vector.shrink_to_fit();

	_max_flag_count = comm_size;
}
inline static void alacv_release() 
{ 
	_alacrity_vector.clear(); 
	_alacrity_vector.shrink_to_fit(); 

	_max_flag_count = 0;
}

#define __ROTATE_LEFT(x, n)  (((x) << (n)) | ((x) >> ((sizeof(x) << 3)-(n))))
#define __ROTATE_RIGHT(x, n) (((x) >> (n)) | ((x) << ((sizeof(x) << 3)-(n))))

inline static int get_alacv_index(int rank) { return rank / FLAG_TYPE_BIT_COUNT; }
inline static int get_fbit_offset(int rank) { return rank % FLAG_TYPE_BIT_COUNT; }
inline static void alacv_set(int rank)
{
	_ASSERT(rank > 0);	

	int index  = get_alacv_index(rank),
		offset = get_fbit_offset(rank);

	_ASSERT(index < static_cast<int>(_alacrity_vector.size()) && offset < FLAG_TYPE_BIT_COUNT);

	flag_t mask = static_cast<flag_t>(__ROTATE_LEFT(1, offset));
	_alacrity_vector[index] |= mask;
}

inline static void alacv_unset(int rank)
{
	_ASSERT(rank > 0);	

	int index  = get_alacv_index(rank),
		offset = get_fbit_offset(rank);

	_ASSERT(index < static_cast<int>(_alacrity_vector.size()) && offset < FLAG_TYPE_BIT_COUNT);

	flag_t mask = static_cast<flag_t>(__ROTATE_LEFT(-2, offset));
	_alacrity_vector[index] &= mask;
}

inline static bool alacv_state(int rank)
{
	_ASSERT(rank > 0);

	int index  = get_alacv_index(rank),
		offset = get_fbit_offset(rank);

	_ASSERT(index < static_cast<int>(_alacrity_vector.size()) && offset < FLAG_TYPE_BIT_COUNT);

	return static_cast<bool>(_alacrity_vector[index] & static_cast<flag_t>(__ROTATE_LEFT(1, offset)));
}

inline static bool alacv_state_global(bool ready)
{
	for(std::size_t i = 0, max_i = _alacrity_vector.size(); i < max_i; ++i)
	{
		flag_t flag_block = _alacrity_vector[i];
		for (std::size_t j = 0, max_j = min(_max_flag_count, FLAG_TYPE_BIT_COUNT); j < max_j; ++j)
		{
			if ((flag_block & static_cast<flag_t>(__ROTATE_LEFT(1, j))) != static_cast<unsigned int>(ready)) return false;
		}
	}
	
	return true;
}

inline static bool alacv_state_several(bool ready)
{
	int in_state_count = 0;
	for(std::size_t i = 0, max_i = _alacrity_vector.size(); i < max_i; ++i)
	{
		flag_t flag_block = _alacrity_vector[i];
		for (std::size_t j = 0, max_j = min(_max_flag_count, FLAG_TYPE_BIT_COUNT); j < max_j; ++j)
		{
			if ((flag_block & static_cast<flag_t>(__ROTATE_LEFT(1, j))) == static_cast<unsigned int>(ready)) ++in_state_count;
		}
	}
	return in_state_count != 0;
}

inline static void alacv_get_nodes(std::vector<int> &node_set, bool ready)
{
	node_set.clear();
	for(std::size_t i = 0, max_i = _alacrity_vector.size(); i < max_i; ++i)
	{
		flag_t flag_block = _alacrity_vector[i];
		for (std::size_t j = 0, max_j = min(_max_flag_count, FLAG_TYPE_BIT_COUNT); j < max_j; ++j)
		{
			if ((flag_block & (__ROTATE_LEFT(1, j))) == static_cast<unsigned int>(ready)) node_set.push_back(i * FLAG_TYPE_BIT_COUNT + j);
		}
	}
	node_set.shrink_to_fit();
}

// End of internal service code
// =======================================================================================================================================================================================================================
#define OWN_SERVICE_TAG INT_MAX
int __stdcall OWN_Barrier(MPI_Comm comm)
{
	int ret_result = MPI_SUCCESS;
	
	int current_node_rank, comm_size;
	if ((ret_result = MPI_Comm_size(comm, &comm_size)) != MPI_SUCCESS) return ret_result; 
	if ((ret_result = MPI_Comm_rank(comm, &current_node_rank)) != MPI_SUCCESS) return ret_result;

	alacv_init(comm_size);

	alacv_set(current_node_rank);
	__foreach(_GRAPH_PATH::iterator, neighbour, _edges[current_node_rank])
	{
		if ((ret_result = MPI_Send(&current_node_rank, 1, MPI_INT, *neighbour, OWN_SERVICE_TAG, comm)) != MPI_SUCCESS) return ret_result;
	}

	int op_flag;
	MPI_Status recv_status;

	do {
		if ((ret_result = MPI_Iprobe(MPI_ANY_SOURCE, OWN_SERVICE_TAG, comm, &op_flag, &recv_status)) != MPI_SUCCESS) return ret_result;

		if (op_flag != 0)
		{	
			int ready_node_rank = 0;
			if ((ret_result = MPI_Recv(&ready_node_rank, 1, MPI_INT, MPI_ANY_SOURCE, OWN_SERVICE_TAG, comm, &recv_status)) != MPI_SUCCESS) return ret_result;

			if (alacv_state(ready_node_rank)) continue;
			
			__foreach(_GRAPH_PATH::iterator, neighbour, _edges[current_node_rank])
			{
				int neighbour_node = *neighbour;
				if (neighbour_node == recv_status.MPI_SOURCE || neighbour_node == ready_node_rank) continue;
				if ((ret_result = MPI_Send(&ready_node_rank, 1, MPI_INT, neighbour_node, OWN_SERVICE_TAG, comm)) != MPI_SUCCESS) return ret_result;
			}

			alacv_set(ready_node_rank);
		}		
	} while(alacv_state_several(false));

	do {
		if ((ret_result = MPI_Iprobe(MPI_ANY_SOURCE, OWN_SERVICE_TAG, comm, &op_flag, &recv_status)) != MPI_SUCCESS) return ret_result;

		if (op_flag != 0)
		{
			int dummy_value;
			if ((ret_result = MPI_Recv(&dummy_value, 1, MPI_INT, MPI_ANY_SOURCE, OWN_SERVICE_TAG, comm, &recv_status)) != MPI_SUCCESS) return ret_result;
			continue;
		}
		break;
	} while(true);

	alacv_release();
	return ret_result;
}

typedef unsigned char byte_t;
typedef std::vector<byte_t> byte_vector;

#define ROOT_RANK 0
int __stdcall OWN_Alltoall(void* sendbuf, int sendcount, MPI_Datatype sendtype, void* recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm) {
	int ret_result = MPI_SUCCESS;
	
	int rank, size;
	if ((ret_result = MPI_Comm_size(comm, &size)) != MPI_SUCCESS) return ret_result; 
	if ((ret_result = MPI_Comm_rank(comm, &rank)) != MPI_SUCCESS) return ret_result;

	if (_edges.size() != size) return OWN_TOPOLOGY_VIOLATION;

	int sendtype_size, recvtype_size;
	MPI_Type_size(sendtype, &sendtype_size);
	MPI_Type_size(recvtype, &recvtype_size);

	if ((ret_result = MPI_Comm_dup(comm, &_comm_broadcast)) != MPI_SUCCESS) return ret_result;

	alacv_init(size);

	rsize_t send_size = sendcount * sendtype_size,
			recv_size = recvcount * recvtype_size;

	int mpi_int_type_size;
	MPI_Type_size(MPI_INT, &mpi_int_type_size);
	byte_vector pack_buffer(send_size + (mpi_int_type_size << 1), 0x00);
	pack_buffer.shrink_to_fit();

	for (int i = 0; i < size; ++i) 
	{
		if (i == rank)
		{
			if (memcpy_s(reinterpret_cast<byte_t*>(recvbuf) + rank * send_size, send_size, reinterpret_cast<byte_t*>(sendbuf) + rank * send_size, send_size) != 0) return OWN_ERROR_INTERNAL;
			continue; 
		}

		int pack_position = 0;
		MPI_Pack(&rank, 1, MPI_INT, &pack_buffer.front(), pack_buffer.size(), &pack_position, comm);
		MPI_Pack(reinterpret_cast<byte_t*>(sendbuf) + i * send_size, sendcount, sendtype, &pack_buffer.front(), pack_buffer.size(), &pack_position, comm);

		if ((ret_result =
			MPI_Send(&pack_buffer.front(), pack_position, MPI_PACKED, _paths[rank][i].at(1), i, comm))
		!= MPI_SUCCESS) return ret_result; 
	}

	int ntimes_to_recieve = size - 1;
	
	int op_flag;
	MPI_Status recv_status;

	//std::vector<int> not_ready_nodes;
	//alacv_get_nodes(not_ready_nodes, false);
	do {
		if ((ret_result = MPI_Iprobe(MPI_ANY_SOURCE, MPI_ANY_TAG, comm, &op_flag, &recv_status)) != MPI_SUCCESS) return ret_result;

		if (op_flag != 0)
		{
			if ((ret_result = MPI_Recv(&pack_buffer.front(), pack_buffer.size(), MPI_PACKED, MPI_ANY_SOURCE, MPI_ANY_TAG, comm, &recv_status)) != MPI_SUCCESS) return ret_result;

			if (recv_status.MPI_TAG != rank)
			{
				if ((ret_result = 
					MPI_Send(&pack_buffer.front(), pack_buffer.size(), MPI_PACKED, _paths[rank][recv_status.MPI_TAG].at(1), recv_status.MPI_TAG, comm))
				!= MPI_SUCCESS) return ret_result;
			}
			else
			{
				int data_source;

				int pack_position = 0;
				MPI_Unpack(&pack_buffer.front(), pack_buffer.size(), &pack_position, &data_source, 1, MPI_INT, comm);
				MPI_Unpack(&pack_buffer.front(), pack_buffer.size(), &pack_position, reinterpret_cast<byte_t*>(recvbuf) + data_source * recv_size, recvcount, recvtype, comm);

				if (--ntimes_to_recieve == 0)
				{
					__foreach(_GRAPH_PATH::iterator, neighbour, _edges[rank])
					{
						if ((ret_result = MPI_Send(NULL, 0, MPI_INT, *neighbour, rank, _comm_broadcast)) != MPI_SUCCESS) return ret_result;
					}

					alacv_set(rank);
				}
			}
		}

		if ((ret_result = MPI_Iprobe(MPI_ANY_SOURCE, MPI_ANY_TAG, _comm_broadcast, &op_flag, &recv_status)) != MPI_SUCCESS) return ret_result;

		if (op_flag != 0)
		{
			if ((ret_result = MPI_Recv(NULL, 0, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, _comm_broadcast, &recv_status)) != MPI_SUCCESS) return ret_result;

			int ready_node_rank = recv_status.MPI_TAG;
			if (alacv_state(ready_node_rank)) continue;
			//std::vector<int>::iterator last_item = not_ready_nodes.end();
			//if (std::find(not_ready_nodes.begin(), last_item, ready_node_rank) == last_item) continue;

			__foreach(_GRAPH_PATH::iterator, neighbour, _edges[rank])
			{
				int neighbour_node = *neighbour;
				if (neighbour_node == recv_status.MPI_SOURCE || neighbour_node == ready_node_rank) continue;
				if ((ret_result = MPI_Send(NULL, 0, MPI_INT, neighbour_node, ready_node_rank, _comm_broadcast)) != MPI_SUCCESS) return ret_result;
			}

			alacv_set(ready_node_rank);
			//alacv_get_nodes(not_ready_nodes, false);
		}

	} while(alacv_state_several(false));
	
	do {
		if ((ret_result = MPI_Iprobe(MPI_ANY_SOURCE, MPI_ANY_TAG, _comm_broadcast, &op_flag, &recv_status)) != MPI_SUCCESS) return ret_result;
		if (op_flag != 0)
		{
			if ((ret_result = MPI_Recv(NULL, 0, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, _comm_broadcast, &recv_status)) != MPI_SUCCESS) return ret_result;
			continue;
		}
		break;
	} while(true);

	alacv_release();

	//MPI_Barrier(comm);
	MPI_Comm_free(&_comm_broadcast);

	return ret_result;
}