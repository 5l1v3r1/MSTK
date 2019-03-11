/* 
Copyright 2019 Triad National Security, LLC. All rights reserved.

This file is part of the MSTK project. Please see the license file at
the root of this repository or at
https://github.com/MeshToolkit/MSTK/blob/master/LICENSE
*/

#include <UnitTest++.h>
#include <TestReporterStdout.h>

#include <mpi.h>

int main(int argc, char *argv[])
{
  int status;

  MPI_Init(&argc, &argv);

  status = UnitTest::RunAllTests ();

  MPI_Finalize();

  return status;
}

