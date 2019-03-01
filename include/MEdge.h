/* 
Copyright 2019 Triad National Security, LLC. All rights reserved.

This file is part of the MSTK project. Please see the license file at
the root of this repository or at
https://github.com/MeshToolkit/MSTK/blob/master/LICENSE
*/

#ifndef _H_MEdge
#define _H_MEdge

#include "MSTK_defines.h"
#include "MSTK_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _H_MEdge_Private
#define _H_MEntity_Private

#include "MEntity.h"

  typedef struct MEdge {

    /*------------------------------------------------------------*/
    /* Common data for all mesh entities */

    Mesh_ptr mesh;
    List_ptr AttInsList;

    unsigned int dim_id;
    unsigned int rtype_gdim_gid;
    unsigned int marker;

#ifdef MSTK_HAVE_MPI
    unsigned int ptype_masterparid;
    unsigned int globalid;  /* if -ve, it represents local id on master proc */
#endif
    /*------------------------------------------------------------*/

    void *adj; /* pointer to entity adjacency structure */

    MVertex_ptr vertex[2];

  } MEdge, *MEdge_ptr;

  /*----- Adjacency definitions --------*/

  typedef struct MEdge_Adj_F1 {
    List_ptr efaces;
  } MEdge_Adj_F1;

  typedef struct MEdge_Adj_F4 {
    List_ptr elements;
  } MEdge_Adj_F4;

  typedef struct MEdge_Adj_RN {  /* POOR PLACE TO PUT HASH TABLE LINK */
    MEdge_ptr hnext;             /* MUST CHANGE IT ?                  */
    int lock;
  } MEdge_Adj_RN;
#else
  typedef void *MEdge_ptr;
#endif

  MEdge_ptr ME_New(Mesh_ptr mesh);
  void ME_Delete(MEdge_ptr medge, int keep);
  void ME_Set_GEntity(MEdge_ptr medge, GEntity_ptr gent);
  void ME_Set_GEntDim(MEdge_ptr medge, int gdim);
  void ME_Set_GEntID(MEdge_ptr medge, int gid);
  int  ME_Derive_GInfo(MEdge_ptr medge);
  void ME_Set_ID(MEdge_ptr medge, int id);

  void ME_Set_Vertex(MEdge_ptr medge, int i, MVertex_ptr vertex);
  
  int ME_ID(MEdge_ptr medge);
  int ME_GEntDim(MEdge_ptr medge);
  int ME_GEntID(MEdge_ptr medge);
  GEntity_ptr ME_GEntity(MEdge_ptr medge);
  MVertex_ptr ME_Vertex(MEdge_ptr medge, int i);
  MVertex_ptr ME_OppVertex(MEdge_ptr e, MVertex_ptr v);

  int ME_Num_Faces(MEdge_ptr medge);
  int ME_Num_Regions(MEdge_ptr medge);
  List_ptr ME_Faces(MEdge_ptr medge);
  List_ptr ME_Regions(MEdge_ptr medge);
  int ME_UsesEntity(MEdge_ptr medge, MEntity_ptr mentity, int etype);

  /* Calling applications can only set the representation type for the
     entire mesh, not for individual mesh entities */
  void ME_Set_RepType(MEdge_ptr medge, RepType rtype);

  void ME_Add_Face(MEdge_ptr medge, MFace_ptr mface);
  void ME_Add_Region(MEdge_ptr medge, MRegion_ptr mregion);
  void ME_Rem_Face(MEdge_ptr medge, MFace_ptr mface);
  void ME_Rem_Region(MEdge_ptr medge, MRegion_ptr mregion);

  MEdge_ptr MEs_Merge(MEdge_ptr e1, MEdge_ptr e2, int topoflag);

  void ME_Lock(MEdge_ptr e);
  void ME_UnLock(MEdge_ptr e);
  int ME_IsLocked(MEdge_ptr e);

  PType ME_PType(MEdge_ptr e);

  /* Is the entity on the partition boundary? */
  int ME_OnParBoundary(MEdge_ptr e);

  int   ME_MasterParID(MEdge_ptr e);

  int   ME_GlobalID(MEdge_ptr e);

#ifdef MSTK_HAVE_MPI

  void  ME_Set_PType(MEdge_ptr e, PType ptype);

  /* Mark/Unmark the entity as being on the partition boundary */
  void ME_Flag_OnParBoundary(MEdge_ptr e);
  void ME_Unflag_OnParBoundary(MEdge_ptr e);
  void  ME_Set_MasterParID(MEdge_ptr e, int masterparid);
  void  ME_Set_GlobalID(MEdge_ptr e, int globalid);
  MEdge_ptr ME_GhostNew(Mesh_ptr mesh);

#endif

#ifdef __cplusplus
}
#endif

#endif
