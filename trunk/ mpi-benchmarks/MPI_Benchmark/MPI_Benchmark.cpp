// MPI_Benchmark.cpp: определяет точку входа для консольного приложения.
//

int _tmain(int argc, _TCHAR* argv[])
{
	char message[20];
	int  i, rank, size, type = 99;
	MPI_Status status;

	MPI_Init(&argc, &argv);
	MPI_Comm_size(MPI_COMM_WORLD, &size);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);

	if (rank == 0)
	{
		strcpy(message, "Hello, world!");
		for (i = 1; i < size; ++i) MPI_Send(message, 14, MPI_CHAR, i, type, MPI_COMM_WORLD);
	} 
	else 
	{
		MPI_Recv(message, 20, MPI_CHAR, 0, type, MPI_COMM_WORLD, &status);
	}

	printf("Message from process = %d : %.14s\n", rank, message);
	return MPI_Finalize();
}

