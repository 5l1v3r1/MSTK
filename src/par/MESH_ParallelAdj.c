/* 
Copyright 2019 Triad National Security, LLC. All rights reserved.

This file is part of the MSTK project. Please see the license file at
the root of this repository or at
https://github.com/MeshToolkit/MSTK/blob/master/LICENSE
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "MSTK.h"
#include "MSTK_private.h"

#ifdef __cplusplus
extern "C" {
#endif

  /* 
     this function updates (or intializes) inter-partition
     relationships of the mesh

     for applications that do not involve topology change,
     this routine can be called only once during initialization

     Author(s): Duo Wang, Rao Garimella
  */  

  int MESH_Update_ParallelAdj(Mesh_ptr mesh, MSTK_Comm comm) {
  int i, idx, nv, ne, nf, nr, local_ov_num[4];
  MVertex_ptr mv;
  MEdge_ptr me;
  MFace_ptr mf;
  MRegion_ptr mr;
  MType mtype;

  int myprtn, numprtns;
  MPI_Comm_rank(comm,&myprtn);
  MPI_Comm_size(comm,&numprtns);

  if (numprtns == 1) return 1;
  
  /* set ghost adjacencies */
  idx = 0;
  while ((mv = MESH_Next_GhostVertex(mesh,&idx)))
    MESH_Flag_Has_Ghosts_From_Prtn(mesh,MV_MasterParID(mv),MVERTEX);
  idx = 0;
  while ((me = MESH_Next_GhostEdge(mesh,&idx)))
    MESH_Flag_Has_Ghosts_From_Prtn(mesh,ME_MasterParID(me),MEDGE);
  idx = 0;
  while ((mf = MESH_Next_GhostFace(mesh,&idx)))
    MESH_Flag_Has_Ghosts_From_Prtn(mesh,MF_MasterParID(mf),MFACE);
  idx = 0;
  while ((mr = MESH_Next_GhostRegion(mesh,&idx)))
    MESH_Flag_Has_Ghosts_From_Prtn(mesh,MR_MasterParID(mr),MREGION);


  /* derive which processors this processor has overlaps with */

  int *local_par_adj = (int *) malloc(numprtns*sizeof(int));
  int *global_par_adj = (int *) malloc(numprtns*numprtns*sizeof(int));

  for (i = 0; i < numprtns; i++) {
    local_par_adj[i] = 0;

    for (mtype = MVERTEX; mtype <= MREGION; mtype++) {
      int j = MESH_Has_Ghosts_From_Prtn(mesh,i,mtype);
      local_par_adj[i] |= j<<(2*mtype);
    }
  }
     
  /* At this point, it is assumed that this processor ('prtn') has
     knowledge of all the processors that it has ghost entities from
     and what type of entities they are. We do an MPI_Allgather so
     that the processor can find out the reverse info, i.e., which
     processors are expecting ghost entities from this processor and
     what type of entities. This info then goes in as the overlap
     entity info for this processor */
  for (i = 0; i < numprtns*numprtns; i++) global_par_adj[i] = 0;
  MPI_Allgather(local_par_adj,numprtns,MPI_INT,global_par_adj,numprtns,MPI_INT,comm);

  /* Now set overlap adjacency flags */

  unsigned int ovnum = 0;
  unsigned int *prtnums = (unsigned int *) malloc(numprtns*sizeof(unsigned int));
  for (i = 0; i < numprtns; i++) {
    for (mtype = MVERTEX; mtype <= MREGION; mtype++) {

      int j = global_par_adj[i*numprtns + myprtn] & 1<<(2*mtype);
      if (j)
        MESH_Flag_Has_Overlaps_On_Prtn(mesh,i,mtype);
    }
  }


  /* Right now the model we use is that every partition sends ALL its
     overlap entity data to any partition that asks for it */
  /* So, if a processor 'i' has ghosts from partition 'j', it needs to
     know the total number of overlap entities on partition 'j' in
     order to allocate sufficient receive buffers */

  int *global_ov_num = (int *) malloc(4*numprtns*sizeof(int));

  /* local overlap entity numbers */
  local_ov_num[0] = MESH_Num_OverlapVertices(mesh);
  local_ov_num[1] = MESH_Num_OverlapEdges(mesh);
  local_ov_num[2] = MESH_Num_OverlapFaces(mesh);
  local_ov_num[3] = MESH_Num_OverlapRegions(mesh);


  MPI_Allgather(local_ov_num,4,MPI_INT,global_ov_num,4,MPI_INT,comm);

  /* Set how many entities a partition can expect to receive from
     another partititon whether it is used on this partition or not */
  MESH_Init_Par_Recv_Info(mesh);
  for(i = 0; i < numprtns; i++) {
    if (MESH_Has_Ghosts_From_Prtn(mesh,i,MANYTYPE)) {
      for (mtype = MVERTEX; mtype <= MREGION; mtype++) 
        MESH_Set_Num_Recv_From_Prtn(mesh,i,mtype,global_ov_num[4*i+mtype]);
    }
  }

  free(global_ov_num);
  free(local_par_adj);
  free(global_par_adj);
  free(prtnums);

  MESH_Mark_ParallelAdj_Current(mesh);

 return 1;
}


  /* Given information about the partitions from with each partition
   * will receive info from, derive information about partitions to
   * which each partition has to send info to */

int MESH_Get_OverlapAdj_From_GhostAdj(Mesh_ptr mesh, MSTK_Comm comm) {
  int i, idx, nv, ne, nf, nr, local_ov_num[4];
  MVertex_ptr mv;
  MEdge_ptr me;
  MFace_ptr mf;
  MRegion_ptr mr;
  MType mtype;

  int myprtn, numprtns;
  MPI_Comm_rank(comm,&myprtn);
  MPI_Comm_size(comm,&numprtns);

  if (numprtns == 1) return 1;
  
  /* derive which processors this processor has overlaps with */

  int *local_par_adj = (int *) malloc(numprtns*sizeof(int));
  int *global_par_adj = (int *) malloc(numprtns*numprtns*sizeof(int));

  for (i = 0; i < numprtns; i++) {
    local_par_adj[i] = 0;

    for (mtype = MVERTEX; mtype <= MREGION; mtype++) {
      int j = MESH_Has_Ghosts_From_Prtn(mesh,i,mtype);
      local_par_adj[i] |= j<<(2*mtype);
    }
  }
     
  /* At this point, it is assumed that this processor ('prtn') has
     knowledge of all the processors that it has ghost entities from
     and what type of entities they are. We do an MPI_Allgather so
     that the processor can find out the reverse info, i.e., which
     processors are expecting ghost entities from this processor and
     what type of entities. This info then goes in as the overlap
     entity info for this processor */
  for (i = 0; i < numprtns*numprtns; i++) global_par_adj[i] = 0;
  MPI_Allgather(local_par_adj,numprtns,MPI_INT,global_par_adj,numprtns,MPI_INT,comm);

  /* Now set overlap adjacency flags */

  unsigned int ovnum = 0;
  unsigned int *prtnums = (unsigned int *) malloc(numprtns*sizeof(unsigned int));
  for (i = 0; i < numprtns; i++) {
    for (mtype = MVERTEX; mtype <= MREGION; mtype++) {

      int j = global_par_adj[i*numprtns + myprtn] & 1<<(2*mtype);
      if (j)
        MESH_Flag_Has_Overlaps_On_Prtn(mesh,i,mtype);
    }
  }


  free(local_par_adj);
  free(global_par_adj);
  free(prtnums);

  /* DON'T MARK PARALLEL ADJACENCY INFO AS CURRENT AS YET BECAUSE THE ACTUAL
     NUMBER OF ENTITIES TO BE SENT AND RECEIVED IS NOT FINALIZED */
  /* MESH_Mark_ParallelAdj_Current(mesh); */

 return 1;
}


#ifdef __cplusplus
}
#endif

