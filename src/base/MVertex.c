#define _H_MVertex_Private

#include <string.h>
#include "MVertex.h"
#include "MVertex_jmp.h"
#include "MSTK_private.h"
#include "MSTK_malloc.h"

#ifdef __cplusplus
extern "C" {
#endif

  MVertex_ptr MV_New(Mesh_ptr mesh) {
    MVertex_ptr v;
    RepType RTYPE;
    
    v = (MVertex_ptr) MSTK_malloc(sizeof(MVertex));

    MEnt_Init_CmnData(v);
    MEnt_Set_Mesh(v,mesh);
    MEnt_Set_Dim(v,0);
    MEnt_Set_GEntDim(v,4);
    MEnt_Set_GEntID(v,0);

    v->xyz[0] = v->xyz[1] = v->xyz[2] = 0.0;
    v->upadj = NULL;
    v->sameadj = NULL;

    RTYPE = mesh ? MESH_RepType(mesh) : F1;
    MV_Set_RepType(v,RTYPE);

    if (mesh) MESH_Add_Vertex(mesh,v);

    return v;
  } 

  void MV_Delete(MVertex_ptr v, int keep) {
    RepType RTYPE = MEnt_RepType(v);
    Mesh_ptr mesh;

    (*MV_Delete_jmp[RTYPE])(v,keep);

    if (MEnt_Dim(v) != MDELETED) {
      mesh = MEnt_Mesh(v);
      MESH_Rem_Vertex(mesh,v);
      MEnt_Set_DelFlag(v);
    }

    if (!keep) {
      MEnt_Free_CmnData(v);
      MSTK_free(v);
    }
  }

  void MV_Restore(MVertex_ptr v) {
    RepType RTYPE = MEnt_RepType(v);
    Mesh_ptr mesh = MEnt_Mesh(v);
    
#ifdef DEBUG
    if (MEnt_Dim(v) != MDELETED) {
      MSTK_Report("MV_Restore",
		  "Trying to restore vertex that is not deleted",WARN);
      return;
    }
#endif

    MEnt_Rem_DelFlag(v);

    MESH_Add_Vertex(mesh,v);

    (*MV_Restore_jmp[RTYPE])(v);
  }

  void MV_Destroy_For_MESH_Delete(MVertex_ptr v) {
    RepType RTYPE = MEnt_RepType(v);

    (*MV_Destroy_For_MESH_Delete_jmp[RTYPE])(v);

    MEnt_Free_CmnData(v);

    MSTK_free(v);
  }

  void MV_Set_RepType(MVertex_ptr v, RepType RTYPE) {
    MEnt_Set_RepType_Data(v,RTYPE);
    (*MV_Set_RepType_jmp[RTYPE])(v);
  }

  void MV_Set_Coords(MVertex_ptr v, double *xyz) {
    v->xyz[0] = xyz[0];
    v->xyz[1] = xyz[1];
    v->xyz[2] = xyz[2];
  }

  void MV_Set_GEntity(MVertex_ptr v, GEntity_ptr gent) {    
  }

  void MV_Set_GEntDim(MVertex_ptr v, int gdim) {
    MEnt_Set_GEntDim(v,gdim);
  }

  void MV_Set_GEntID(MVertex_ptr v, int gid) {
    MEnt_Set_GEntID(v,gid);
  }

  void MV_Set_ID(MVertex_ptr v, int id) {
    MEnt_Set_ID(v,id);
  }

  Mesh_ptr MV_Mesh(MVertex_ptr v) {
    return MEnt_Mesh(v);
  }

  int MV_ID(MVertex_ptr v) {
    return MEnt_ID(v);
  }

  int MV_GEntDim(MVertex_ptr v) {
    return MEnt_GEntDim(v);
  }

  int MV_GEntID(MVertex_ptr v) {
    return MEnt_GEntID(v);
  }

  GEntity_ptr MV_GEntity(MVertex_ptr v) {
    return MEnt_GEntity(v);
  }

  void MV_Coords(MVertex_ptr v, double *xyz) {
    xyz[0] = v->xyz[0]; xyz[1] = v->xyz[1]; xyz[2] = v->xyz[2];
  }   

  int MV_Num_AdjVertices(MVertex_ptr v) {
    RepType RTYPE = MEnt_RepType(v);
    return (*MV_Num_Edges_jmp[RTYPE])(v);
  }

  int MV_Num_Edges(MVertex_ptr v) {
    RepType RTYPE = MEnt_RepType(v);
    return (*MV_Num_Edges_jmp[RTYPE])(v);
  }

  int MV_Num_Faces(MVertex_ptr v) {
    RepType RTYPE = MEnt_RepType(v);
    return (*MV_Num_Faces_jmp[RTYPE])(v);
  }
  
  int MV_Num_Regions(MVertex_ptr v) {
    RepType RTYPE = MEnt_RepType(v);
    return (*MV_Num_Regions_jmp[RTYPE])(v);
  }

  List_ptr MV_AdjVertices(MVertex_ptr v) {
    RepType RTYPE = MEnt_RepType(v);
    return (*MV_AdjVertices_jmp[RTYPE])(v);
  }

  List_ptr MV_Edges(MVertex_ptr v) {
    RepType RTYPE = MEnt_RepType(v);
    return (*MV_Edges_jmp[RTYPE])(v);
  }

  List_ptr MV_Faces(MVertex_ptr v) {
    RepType RTYPE = MEnt_RepType(v);
    return (*MV_Faces_jmp[RTYPE])(v);
  }

  List_ptr MV_Regions(MVertex_ptr v) {
    RepType RTYPE = MEnt_RepType(v);
    return (*MV_Regions_jmp[RTYPE])(v);
  }

  void MV_Add_AdjVertex(MVertex_ptr v, MVertex_ptr adjv) {
    RepType RTYPE = MEnt_RepType(v);
    (*MV_Add_AdjVertex_jmp[RTYPE])(v,adjv);
  }

  void MV_Rem_AdjVertex(MVertex_ptr v, MVertex_ptr adjv) {
    RepType RTYPE = MEnt_RepType(v);
    (*MV_Rem_AdjVertex_jmp[RTYPE])(v,adjv);
  }

  void MV_Add_Edge(MVertex_ptr v, MEdge_ptr e) {
    RepType RTYPE = MEnt_RepType(v);
    (*MV_Add_Edge_jmp[RTYPE])(v,e);
  }

  void MV_Add_Face(MVertex_ptr v, MFace_ptr f) {
    RepType RTYPE = MEnt_RepType(v);
    (*MV_Add_Face_jmp[RTYPE])(v,f);
  }

  void MV_Add_Region(MVertex_ptr v, MRegion_ptr r) {
    RepType RTYPE = MEnt_RepType(v);
    (*MV_Add_Region_jmp[RTYPE])(v,r);
  }

  void MV_Rem_Edge(MVertex_ptr v, MEdge_ptr e) {
    RepType RTYPE = MEnt_RepType(v);
    (*MV_Rem_Edge_jmp[RTYPE])(v,e);
  }

  void MV_Rem_Face(MVertex_ptr v, MFace_ptr f) {
    RepType RTYPE = MEnt_RepType(v);
    (*MV_Rem_Face_jmp[RTYPE])(v,f);
  }

  void MV_Rem_Region(MVertex_ptr v, MRegion_ptr r) {
    RepType RTYPE = MEnt_RepType(v);
    (*MV_Rem_Region_jmp[RTYPE])(v,r);
  }

#ifdef __cplusplus
}
#endif
