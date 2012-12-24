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

inline static void alacv_get_nodes(std::vector<int> &node_set, bool ready)
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
typedef unsigned char byte_t;
typedef std::vector<byte_t> byte_vector;

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

	rsize_t send_size = sendcount * sendtype_size;
	byte_vector pack_buffer(send_size + (sizeof(int) << 1), 0x00);
	pack_buffer.shrink_to_fit();

	for (int i = 0; i < size; ++i) 
	{
		if (i == rank)
		{
			if (memcpy_s(reinterpret_cast<byte_t*>(recvbuf) + rank * send_size, send_size, reinterpret_cast<byte_t*>(sendbuf) + rank * send_size, send_size) != 0) return OWN_ERROR_INTERNAL;
			continue; 
		}

		int pack_position = 0;
		MPI_Pack(&i, 1, MPI_INT, &pack_buffer.front(), pack_buffer.size(), &pack_position, comm);
		MPI_Pack(&rank, 1, MPI_INT, &pack_buffer.front(), pack_buffer.size(), &pack_position, comm);
		MPI_Pack(reinterpret_cast<byte_t*>(sendbuf) + i * send_size, sendcount, sendtype, &pack_buffer.front(), pack_buffer.size(), &pack_position, comm);

		if ((ret_result =
			MPI_Send(&pack_buffer.front(), pack_position, MPI_PACKED, _paths[rank][i].at(1), 0, comm))
		!= MPI_SUCCESS) return ret_result; 

		//std::cout << "From " << rank << " To " << i << " (" << send_size << " bytes)" << std::endl;
	}

	int ntimes_to_recieve = size - 1;

	std::vector<int> not_ready_nodes;
	alacv_get_nodes(not_ready_nodes, false);
	do {
		int op_flag;

		MPI_Status recv_status;
		if ((ret_result = MPI_Iprobe(MPI_ANY_SOURCE, MPI_ANY_TAG, comm, &op_flag, &recv_status)) != MPI_SUCCESS) return ret_result;

		if (op_flag != 0)
		{
			if ((ret_result = MPI_Recv(&pack_buffer.front(), pack_buffer.size(), MPI_PACKED, MPI_ANY_SOURCE, MPI_ANY_TAG, comm, &recv_status)) != MPI_SUCCESS) return ret_result;

			int data_destination, data_source;

			int pack_position = 0;
			MPI_Unpack(&pack_buffer.front(), pack_buffer.size(), &pack_position, &data_destination, 1, MPI_INT, comm);
			MPI_Unpack(&pack_buffer.front(), pack_buffer.size(), &pack_position, &data_source, 1, MPI_INT, comm);

			if (data_destination != rank)
			{
				if ((ret_result = 
					MPI_Send(&pack_buffer.front(), pack_buffer.size(), MPI_PACKED, _paths[rank][data_destination].at(1), 0, comm))
				!= MPI_SUCCESS) return ret_result;

				//std::cout << "Retranslating from " << rank << " to " << _paths[rank][data_destination].at(1) << " source " << data_source <<  " destination " << data_destination << " (" << pack_buffer.size() << " bytes)" << std::endl;
			}
			else
			{
				//int data_source;

				rsize_t recv_size = recvcount * recvtype_size;
				byte_vector temp_recvbuf(recv_size, 0x00);
				temp_recvbuf.shrink_to_fit();

				///MPI_Unpack(&pack_buffer.front(), pack_buffer.size(), &pack_position, &data_source, 1, MPI_INT, comm);
				MPI_Unpack(&pack_buffer.front(), pack_buffer.size(), &pack_position, &temp_recvbuf.front(), temp_recvbuf.size(), recvtype, comm);

				if (memcpy_s(reinterpret_cast<byte_t*>(recvbuf) + data_source * recv_size, recv_size, &temp_recvbuf.front(), temp_recvbuf.size()) != 0) return OWN_ERROR_INTERNAL;

				//std::cout << "Recieving data from " << data_source << " at " << rank << " (times to recieve: " << ntimes_to_recieve - 1 << " " << temp_recvbuf.size() << "bytes)" << std::endl;
				if (--ntimes_to_recieve == 0)
				{
					alacv_set(rank);
					//for (int i = 0; i < size; ++i) 
					//{
					//	if (i == rank) continue; 
					//	if ((ret_result = MPI_Send(&rank, 1, MPI_INT, i, 0, _comm_broadcast)) != MPI_SUCCESS) return ret_result;
					//}

					__foreach(_GRAPH_PATH::iterator, neighbour, _edges[rank])
					{
						if ((ret_result = MPI_Send(&rank, 1, MPI_INT, *neighbour, 0, _comm_broadcast)) != MPI_SUCCESS) return ret_result;
					}

					alacv_get_nodes(not_ready_nodes, false);

					//std::cout << "'Job done' state broadcasting " << rank << std::endl;
				}
			}
		}

		if ((ret_result = MPI_Iprobe(MPI_ANY_SOURCE, MPI_ANY_TAG, _comm_broadcast, &op_flag, &recv_status)) != MPI_SUCCESS) return ret_result;

		if (op_flag != 0)
		{
			int ready_node_rank;
			if ((ret_result = MPI_Recv(&ready_node_rank, recv_status.count, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, _comm_broadcast, &recv_status)) != MPI_SUCCESS) return ret_result;

			__foreach(_GRAPH_PATH::iterator, neighbour, _edges[rank])
			{
				if (*neighbour == recv_status.MPI_SOURCE) continue;
				if ((ret_result = MPI_Send(&ready_node_rank, 1, MPI_INT, *neighbour, 0, _comm_broadcast)) != MPI_SUCCESS) return ret_result;
			}

			alacv_set(ready_node_rank);
			alacv_get_nodes(not_ready_nodes, false);

			//std::cout << "spinning retranslation cycle at " << rank << " ready node is: " << ready_node_rank << " (";
			//__foreach(std::vector<int>::iterator, entry, not_ready_nodes)
			//{
			//	if (entry != not_ready_nodes.begin()) std::cout << ", ";
			//	std::cout << *entry;
			//}
			//std::cout << ")" << std::endl;
		}

	} while(!not_ready_nodes.empty());
	
	alacv_release();

	MPI_Barrier(_comm_broadcast);
	MPI_Comm_free(&_comm_broadcast);

	return ret_result;
}