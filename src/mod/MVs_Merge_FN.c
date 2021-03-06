/* 
Copyright 2019 Triad National Security, LLC. All rights reserved.

This file is part of the MSTK project. Please see the license file at
the root of this repository or at
https://github.com/MeshToolkit/MSTK/blob/master/LICENSE
*/

#include <stdlib.h>
#include <stdio.h>
#include "MVertex_jmp.h"
#include "MSTK_private.h"


MVertex_ptr MVs_Merge_FN(MVertex_ptr v1, MVertex_ptr v2, int topoflag) {
  int idx, gdim, gid, nsets;
  MEdge_ptr   edge;
  Mesh_ptr    mesh;
  List_ptr    vedges2;
  MSet_ptr    mset;

  mesh = MV_Mesh(v1);
  gid = MV_GEntID(v1);
  gdim = MV_GEntDim(v1);

  if (mesh != MF_Mesh(v2)) {
    MSTK_Report("MVs_Join","Vertices not from same mesh",MSTK_ERROR);
    return 0;
  }
  else if (topoflag) { /* Make sure model topology is not violated */
    if (gid != MV_GEntID(v2) || gdim != MV_GEntDim(v2)) {
      MSTK_Report("MFs_Join","Faces not from same geometric entity",MSTK_ERROR);
      return 0;
    }
  }

  vedges2 = MV_Edges(v2);

  if (vedges2) {
    idx = 0;
    while ((edge = List_Next_Entry(vedges2,&idx)))
      ME_Replace_Vertex(edge,v2,v1);

    List_Delete(vedges2);
  }

  nsets = MESH_Num_MSets(mesh);
  if(nsets) {
    idx = 0;
    while ((mset = (MSet_ptr) MESH_Next_MSet(mesh,&idx))) {
      if (MSet_Contains(mset, v2)) {
        if(MSet_Contains(mset, v1))
          MSet_Rem(mset, v2);
        else
          MSet_Replace(mset, v2, v1);
      }
    }
  }

  MV_Delete(v2,1);

  return v1;
}
