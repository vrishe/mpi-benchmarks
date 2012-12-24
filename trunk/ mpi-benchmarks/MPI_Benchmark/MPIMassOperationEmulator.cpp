#include "MPIMassOperationEmulator.h"
#include "TopologyGraphModule.h"

extern _GRAPH_EDGES	 _edges;
extern _GRAPH_PATHES _paths;

#define __foreach(it, i, c) for(it i = c.begin(), _end_##i = c.end(); i != _end_##i; ++i)
// =======================================================================================================================================================================================================================
// Internal service code

static MPI_Comm _comm_broadcast = NULL;

// Some kind of barrier synchronization mechanism 

typedef unsigned int flag_t;
typedef std::vector<flag_t> alacv_t;

static alacv_t _alacrity_vector;
static std::size_t _max_flag_count;

#define FLAG_TYPE_BIT_COUNT (sizeof(flag_t) << 3)

inline void alacv_init(int comm_size)
{
	_ASSERT(comm_size > 0);
	_alacrity_vector = alacv_t(comm_size / FLAG_TYPE_BIT_COUNT + 1, 0U);
	_alacrity_vector.shrink_to_fit();

	_max_flag_count = comm_size;
}
inline void alacv_release() 
{ 
	_alacrity_vector.clear(); 
	_alacrity_vector.shrink_to_fit(); 

	_max_flag_count = 0;
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

	_ASSERT(index < static_cast<int>(_alacrity_vector.size()) && offset < FLAG_TYPE_BIT_COUNT);

	flag_t mask = static_cast<flag_t>(__ROTATE_LEFT(1, offset));
	_alacrity_vector[index] |= mask;
}

inline void alacv_unset(int rank)
{
	_ASSERT(rank > 0);	

	int index  = get_alacv_index(rank),
		offset = get_fbit_offset(rank);

	_ASSERT(index < static_cast<int>(_alacrity_vector.size()) && offset < FLAG_TYPE_BIT_COUNT);

	flag_t mask = static_cast<flag_t>(__ROTATE_LEFT(-2, offset));
	_alacrity_vector[index] &= mask;
}

inline void alacv_get_nodes(std::vector<int> &node_set, bool ready)
{
	node_set.clear();
	for(int i = 0, max_i = _alacrity_vector.size(); i < max_i; ++i)
	{
		flag_t flag_block = _alacrity_vector[i];
		for (int j = 0, max_j = min(_max_flag_count, FLAG_TYPE_BIT_COUNT); j < max_j; ++j)
		{
			if ((flag_block & (__ROTATE_LEFT(1, j))) == static_cast<unsigned int>(ready)) node_set.push_back(i * FLAG_TYPE_BIT_COUNT + j);
		}
	}
	node_set.shrink_to_fit();
}

// End of internal service code
// =======================================================================================================================================================================================================================

#define ROOT_RANK 0
int __stdcall OWN_Alltoall(void* sendbuf, int sendcount, MPI_Datatype sendtype, void* recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm) {
	int ret_result = 0;
	
	int rank, size;
	if ((ret_result = MPI_Comm_size(comm, &size)) != MPI_SUCCESS) return ret_result; 
	if ((ret_result = MPI_Comm_rank(comm, &rank)) != MPI_SUCCESS) return ret_result;

	if (_edges.size() != size) return OWN_TOPOLOGY_VIOLATION;

	int sendtype_size, recvtype_size;
	MPI_Type_size(sendtype, &sendtype_size);
	MPI_Type_size(recvtype, &recvtype_size);

	if ((ret_result = MPI_Comm_dup(comm, &_comm_broadcast)) != MPI_SUCCESS) return ret_result;

	alacv_init(size);

	//std::cout << "Sending essential data @ " << rank << std::endl;
	for (int i = 0; i < size; ++i) 
	{
		if (i == rank) continue; 
		if ((ret_result =
			MPI_Send(reinterpret_cast<unsigned char*>(sendbuf) + i * sendcount * sendtype_size, sendcount, sendtype, _paths[rank][i].at(1), i, comm))
		!= MPI_SUCCESS) return ret_result; 

		std::cout << "From " << rank << " To " << i << std::endl;
	}

	std::vector<unsigned char> temp_recvbuf(recvcount * recvtype_size, 0x00);
	temp_recvbuf.shrink_to_fit();

	int ntimes_to_recieve = size - 1;

	std::vector<int> not_ready_nodes;
	alacv_get_nodes(not_ready_nodes, false);
	do {
		int op_flag;

		MPI_Status recv_status;
		if ((ret_result = MPI_Iprobe(MPI_ANY_SOURCE, MPI_ANY_TAG, comm, &op_flag, &recv_status)) != MPI_SUCCESS) return ret_result;

		if (op_flag != 0)
		{
			//std::cout << "Processing data message @ " << rank << std::endl;

			if ((ret_result = MPI_Recv(temp_recvbuf.begin()._Ptr, recvcount, recvtype, MPI_ANY_SOURCE, MPI_ANY_TAG, comm, MPI_STATUS_IGNORE)) != MPI_SUCCESS) return ret_result;
			if (recv_status.MPI_TAG != rank)
			{
				if ((ret_result = 
					MPI_Send(temp_recvbuf.begin()._Ptr, recvcount, recvtype, _paths[rank][recv_status.MPI_TAG].at(1), recv_status.MPI_TAG, comm))
				!= MPI_SUCCESS) return ret_result;

				std::cout << "Retranslating from " << rank << " to " << _paths[rank][recv_status.MPI_TAG].at(1) << " source " << recv_status.MPI_SOURCE <<  " destination " << recv_status.MPI_TAG << std::endl;
			}
			else
			{
				rsize_t dst_size = recvcount * recvtype_size;
				if (memcpy_s(reinterpret_cast<unsigned char*>(recvbuf) + recv_status.MPI_SOURCE * dst_size, dst_size, temp_recvbuf.data(), temp_recvbuf.size()) != 0) return OWN_ERROR_INTERNAL;

				std::cout << "Recieving data from " << recv_status.MPI_SOURCE << " at " << rank << " (times to recieve: " << ntimes_to_recieve - 1 << ")" << std::endl;
				if (--ntimes_to_recieve == 0)
				{
					alacv_set(rank);
					for (int i = 0; i < size; ++i) 
					{
						if (i == rank) continue; 
						if ((ret_result = MPI_Send(&rank, 1, MPI_INT, i, 0, _comm_broadcast)) != MPI_SUCCESS) return ret_result;
					}
					alacv_get_nodes(not_ready_nodes, false);

					std::cout << "'Job done' state broadcasting " << rank << std::endl;
				}
			}
		}

		if ((ret_result = MPI_Iprobe(MPI_ANY_SOURCE, MPI_ANY_TAG, _comm_broadcast, &op_flag, &recv_status)) != MPI_SUCCESS) return ret_result;

		if (op_flag != 0)
		{
			//std::cout << "Processing service cycle @ " << rank << std::endl;

			int ready_node_rank;
			if ((ret_result = MPI_Recv(&ready_node_rank, recv_status.count, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, _comm_broadcast, &recv_status)) != MPI_SUCCESS) return ret_result;

			alacv_set(ready_node_rank);
			alacv_get_nodes(not_ready_nodes, false);

			std::cout << "spinning retranslation cycle at " << rank << " ready node is: " << ready_node_rank;
			__foreach(std::vector<int>::iterator, entry, not_ready_nodes)
			{
				if (entry != not_ready_nodes.begin()) std::cout << ", ";
				std::cout << *entry;
			}
			std::cout << std::endl;
		}

	} while(!not_ready_nodes.empty());

	alacv_release();

	MPI_Comm_free(&_comm_broadcast);

	return ret_result;
}