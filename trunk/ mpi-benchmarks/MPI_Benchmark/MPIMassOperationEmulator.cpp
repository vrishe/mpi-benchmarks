#include "MPIMassOperationEmulator.h"
#include "TopologyGraphModule.h"

extern _GRAPH_EDGES	 _edges;
extern _GRAPH_PATHES _pathes;

inline int OWN_Send(void* buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm) {
	int rank;
	MPI_Comm_rank(comm, &rank);
	
	MPI_Send(buf, count, datatype, _pathes[rank][dest].at(1), tag, comm);
}

int __stdcall OWN_AllToAll(void* sendbuf, int sendcount, MPI_Datatype sendtype, void* recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm) {
	int rank, size;
	MPI_Comm_size(comm, &size); 
	MPI_Comm_rank(comm, &rank);

	int sendTypeSize;
	MPI_Type_size(sendtype, &sendTypeSize);
	size_t offset = ( sendcount / size ) * sendTypeSize;
	/*
	for (int i = 0; i < size; ++i)
		if (i != rank)
			OWN_Send(sendbuf + offset, sendcount, sendtype, i, i, comm);
	for (int i = 0; i < size; ++i)
		if (i != rank) {
			
			MPI_Recv(recvbuf, 20, MPI_CHAR, 0, type, MPI_COMM_WORLD, &status);
		}
		*/
	//printf( "Message from process = %d : %.14s\n", rank,message);

	return 0;
}