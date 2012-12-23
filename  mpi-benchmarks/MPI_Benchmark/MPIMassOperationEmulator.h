#pragma once

int __stdcall OWN_Alltoall(void* sendbuf, int sendcount, MPI_Datatype sendtype, void* recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm);

#define OWN_TOPOLOGY_VIOLATION	(MPI_ERR_LASTCODE + 1)
#define OWN_ERROR_INTERNAL		(MPI_ERR_LASTCODE + 2)