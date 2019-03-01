/* 
Copyright 2019 Triad National Security, LLC. All rights reserved.

This file is part of the MSTK project. Please see the license file at
the root of this repository or at
https://github.com/MeshToolkit/MSTK/blob/master/LICENSE
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "MSTK.h"
#include "MSTK_private.h"
#ifdef __cplusplus
extern "C" {
#endif



  /* 
     This function is a collective call

     It builds connection information across processors based on vertex global id


     Author(s): Duo Wang, Rao Garimella
  */

  int MESH_BuildConnection(Mesh_ptr submesh, int topodim, MSTK_Comm comm) {
  int i, j, nv, nr, nbv, mesh_info[10];
  MVertex_ptr mv;
  List_ptr boundary_verts;
  int *loc, *global_mesh_info, *list_boundary_vertex, *recv_list_vertex;
  int iloc,  global_id, max_nbv, index_nbv;

  int rank, num;
  MPI_Comm_rank(comm,&rank);
  MPI_Comm_size(comm,&num);

  for (i = 0; i < 10; i++) mesh_info[i] = 0;
  nv = MESH_Num_Vertices(submesh);
  nr = MESH_Num_Regions(submesh);
  mesh_info[1] = nv;

  /* calculate number of boundary vertices */ 
  nbv = 0;
  boundary_verts = List_New(10);
  for(i = 0; i < nv; i++) {
    mv = MESH_Vertex(submesh,i);
    if (MV_PType(mv) != PINTERIOR) {
      List_Add(boundary_verts,mv);
      nbv++;
    }
  }
  mesh_info[4] = nbv;
  
  /* sort boundary vertices based on global ID, for binary search */
  List_Sort(boundary_verts,nbv,sizeof(MVertex_ptr),compareGlobalID);

  global_mesh_info = (int *)malloc(10*num*sizeof(int));
  MPI_Allgather(mesh_info,10,MPI_INT,global_mesh_info,10,MPI_INT,comm);

  max_nbv = 0;
  for(i = 0; i < num; i++)
    if(max_nbv < global_mesh_info[10*i+4])
      max_nbv = global_mesh_info[10*i+4];

  list_boundary_vertex = (int *)malloc(max_nbv*sizeof(int));
  recv_list_vertex = (int *)malloc(num*max_nbv*sizeof(int));

  /* only global ID are sent */
  index_nbv = 0;
  for(i = 0; i < nbv; i++) {
    mv = List_Entry(boundary_verts,i);
    list_boundary_vertex[index_nbv] = MV_GlobalID(mv);
    index_nbv++;
  }

  /* gather boundary vertices */
  MPI_Allgather(list_boundary_vertex,max_nbv,MPI_INT,recv_list_vertex,max_nbv,MPI_INT,comm);

  for(i = 0; i < nbv; i++) {
    mv = List_Entry(boundary_verts,i);
    global_id = MV_GlobalID(mv);
    /* 
       check which processor has the vertex of the same global ID
       Different from assigning global id, we check all the other processors, from large to small
       by rank, whenever a vertex is found, mark it as neighbors, the masterparid is the smallest 
       rank processor among all the neighbors
    */
    for(j = num-1; j >= 0; j--) {
      if(j == rank) continue;
      loc = (int *)bsearch(&global_id,
			   &recv_list_vertex[max_nbv*j],
			   global_mesh_info[10*j+4],
			   sizeof(int),
			   compareINT);
      /* if found the vertex on previous processors */
      if(loc) {
	iloc = (int)(loc - &recv_list_vertex[max_nbv*j]);	

	MESH_Flag_Has_Ghosts_From_Prtn(submesh,j,MVERTEX);
	MESH_Flag_Has_Ghosts_From_Prtn(submesh,j,MEDGE);
	if(nr) {
	  MESH_Flag_Has_Ghosts_From_Prtn(submesh,j,MFACE);
	}
      }
    }
  }

  List_Delete(boundary_verts);
  free(global_mesh_info);
  free(list_boundary_vertex);
  free(recv_list_vertex);
  return 1;
}
  
#ifdef __cplusplus
}
#endif

