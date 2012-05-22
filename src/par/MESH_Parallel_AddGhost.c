#define _H_Mesh_Private

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "Mesh.h"
#include "MSTK.h"
#include "MSTK_private.h"

#ifdef __cplusplus
extern "C" {
#endif



  /* 
     This function is a collective call

     It first creates a send_mesh of OVERLAP elements, then send it to neighbor processors
     also it receives all the send_mehses from neighbor processors and install them into current mesh
     It uses SendMesh and RecvMesh to communicate
     Assume OVERLAP elements is properly labeled

     Author(s): Duo Wang, Rao Garimella
  */

int MESH_Parallel_AddGhost_Face(Mesh_ptr submesh, int rank, int num, MPI_Comm comm);
int MESH_Parallel_AddGhost_Region(Mesh_ptr submesh, int rank, int num, MPI_Comm comm);


int MESH_Parallel_AddGhost(Mesh_ptr submesh, int rank, int num,  MPI_Comm comm) {
  int nf, nr;
  RepType rtype;

  rtype = MESH_RepType(submesh);
  nf = MESH_Num_Faces(submesh);
  nr = MESH_Num_Regions(submesh);
  if (nr)
    MESH_Parallel_AddGhost_Region(submesh, rank, num, comm);
  else if(nf) 
    MESH_Parallel_AddGhost_Face(submesh, rank, num, comm);
  else {
    MSTK_Report("MESH_Parallel_AddGhost()","only send volume or surface mesh",MSTK_ERROR);
    exit(-1);
  }
  return 1;
}


  /* 
     Send 1-ring Faces to neighbor processors, and receive them 
     First update the parallel adjancy information, 
  */
int MESH_Parallel_AddGhost_Face(Mesh_ptr submesh, int rank, int num, MPI_Comm comm) {
  int i, num_recv_procs, index_recv_mesh;
  Mesh_ptr send_mesh;
  Mesh_ptr *recv_meshes;

  /* build the 1-ring layer send mesh */
  send_mesh = MESH_New(MESH_RepType(submesh));
  MESH_BuildSubMesh(submesh,send_mesh);

  /* 
     first update parallel adjancy information
     any two processor that has vertex connection now has all connection
  */
  for (i = 0; i < num; i++) {
    if(i == rank) continue;
    if( MESH_Has_Ghosts_From_Prtn(submesh,i,MVERTEX) ) {
      MESH_Flag_Has_Ghosts_From_Prtn(submesh,i,MALLTYPE);
      MESH_Flag_Has_Overlaps_On_Prtn(submesh,i,MALLTYPE);
    }
    if( MESH_Has_Overlaps_On_Prtn(submesh,i,MVERTEX) ) {
      MESH_Flag_Has_Overlaps_On_Prtn(submesh,i,MALLTYPE);
      MESH_Flag_Has_Ghosts_From_Prtn(submesh,i,MALLTYPE);
    }
  }
  /* allocate meshes to receive from other processors */
  num_recv_procs = MESH_Num_GhostPrtns(submesh);
  recv_meshes = (Mesh_ptr*)MSTK_malloc(num_recv_procs*sizeof(Mesh_ptr));
  for(i = 0; i < num_recv_procs; i++)
    recv_meshes[i] = MESH_New(MESH_RepType(submesh));

  /* printf(" number of recv_procs %d,on rank %d\n", num_recv_procs, rank); */

  index_recv_mesh = 0;
  for (i = 0; i < num; i++) {
    if(i == rank) continue;
    if(i < rank) {     
      if( MESH_Has_Ghosts_From_Prtn(submesh,i,MFACE) ) 
	MESH_RecvMesh(recv_meshes[index_recv_mesh++],2,i,rank,comm);
      if( MESH_Has_Overlaps_On_Prtn(submesh,i,MFACE) ) 
	MESH_SendMesh(send_mesh,i,comm);
    }
    if(i > rank) {     
      if( MESH_Has_Overlaps_On_Prtn(submesh,i,MFACE) ) 
	MESH_SendMesh(send_mesh,i,comm);
      if( MESH_Has_Ghosts_From_Prtn(submesh,i,MFACE) ) 
	MESH_RecvMesh(recv_meshes[index_recv_mesh++],2,i,rank,comm);
    }
  }

  /* install the recv_meshes */
  MESH_ConcatSubMesh(submesh, num_recv_procs, recv_meshes);

  /* delete recvmeshes */
  for (i = 0; i < num_recv_procs; i++) 
    MESH_Delete(recv_meshes[i]);

  return 1;
}


int MESH_Parallel_AddGhost_Region(Mesh_ptr submesh, int rank, int num, MPI_Comm comm) {
  int i, num_recv_procs, index_recv_mesh;
  Mesh_ptr send_mesh;
  Mesh_ptr *recv_meshes;

  /* build the 1-ring outside layer send mesh */
  send_mesh = MESH_New(MESH_RepType(submesh));
  MESH_BuildSubMesh(submesh,send_mesh);

  /* 
     first update parallel adjancy information
     any two processor that has vertex connection now has all connection
  */
  for (i = 0; i < num; i++) {
    if(i == rank) continue;
    if( MESH_Has_Ghosts_From_Prtn(submesh,i,MVERTEX) ) {
      MESH_Flag_Has_Ghosts_From_Prtn(submesh,i,MALLTYPE);
      MESH_Flag_Has_Overlaps_On_Prtn(submesh,i,MALLTYPE);
    }
    if( MESH_Has_Overlaps_On_Prtn(submesh,i,MVERTEX) ) {
      MESH_Flag_Has_Overlaps_On_Prtn(submesh,i,MALLTYPE);
      MESH_Flag_Has_Ghosts_From_Prtn(submesh,i,MALLTYPE);
    }
  }
  /* allocate meshes to receive from other processors */
  num_recv_procs = MESH_Num_GhostPrtns(submesh);
  recv_meshes = (Mesh_ptr*)MSTK_malloc(num_recv_procs*sizeof(Mesh_ptr));
  for(i = 0; i < num_recv_procs; i++)
    recv_meshes[i] = MESH_New(MESH_RepType(submesh));

  /* printf(" 3D number of recv_procs %d,on rank %d\n", num_recv_procs, rank); */

  index_recv_mesh = 0;
  for (i = 0; i < num; i++) {
    if(i == rank) continue;
    if(i < rank) {     
      if( MESH_Has_Ghosts_From_Prtn(submesh,i,MREGION) ) 
	MESH_RecvMesh(recv_meshes[index_recv_mesh++],3,i,rank,comm);
      if( MESH_Has_Overlaps_On_Prtn(submesh,i,MREGION) ) 
	MESH_SendMesh(send_mesh,i,comm);
    }
    if(i > rank) {     
      if( MESH_Has_Overlaps_On_Prtn(submesh,i,MREGION) ) 
	MESH_SendMesh(send_mesh,i,comm);
      if( MESH_Has_Ghosts_From_Prtn(submesh,i,MREGION) ) 
	MESH_RecvMesh(recv_meshes[index_recv_mesh++],3,i,rank,comm);
    }
  }
  
  /* concatenate the recv_meshes */
  MESH_ConcatSubMesh(submesh, num_recv_procs, recv_meshes);

  /* delete recvmeshes */
  for (i = 0; i < num_recv_procs; i++) 
    MESH_Delete(recv_meshes[i]);

  return 1;

}

#ifdef __cplusplus
}
#endif

