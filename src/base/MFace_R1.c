#define _H_MFace_Private

#include "MFace.h"
#include "MFace_jmp.h"
#include "MSTK_private.h"
#include "MSTK_malloc.h"

#ifdef __cplusplus
extern "C" {
#endif

  void MF_Set_RepType_R1(MFace_ptr f) {
    MFace_Adj_R1 *adj;

    adj = f->adj = (MFace_Adj_R1 *) MSTK_malloc(sizeof(MFace_Adj_R1));
    adj->fvertices = NULL;
    adj->hnext = NULL;
    adj->lock = 0;
  }


  void MF_Delete_R1(MFace_ptr f, int keep) {
    MFace_Adj_R1 *adj;

    adj = (MFace_Adj_R1 *) f->adj;

    if (adj) {
      if (adj->fvertices) List_Delete(adj->fvertices);
      MSTK_free(adj);
    }

    if (!keep) {
      MEnt_Set_DelFlag((MEntity_ptr) f);
    }
    if (keep) 
      MSTK_Report("MF_Delete_R1","Deletion of faces is permanent in this representation",MSTK_ERROR);
  }

  void MF_Restore_R1(MFace_ptr f) {
#ifdef DEBUG
    MSTK_Report("MF_Restore_R1",
		"Function call not suitable for this representation",MSTK_WARN);
#endif
  }

  void MF_Destroy_For_MESH_Delete_R1(MFace_ptr f) {
    MFace_Adj_R1 *adj;

    adj = (MFace_Adj_R1 *) f->adj;

    if (adj) {
      if (adj->fvertices) List_Delete(adj->fvertices);
      MSTK_free(adj);
    }
  }

  void MF_Set_Edges_R1(MFace_ptr f, int n, MEdge_ptr *e, int *dir) {
#ifdef DEBUG
    MSTK_Report("MF_Set_Edges_R1",
		"Function call not suitable for this representation",MSTK_WARN);
#endif
  }

  void MF_Replace_Edges_R1(MFace_ptr f, int nold, MEdge_ptr *oldedges, int nnu, MEdge_ptr *nuedges) {
#ifdef DEBUG
    MSTK_Report("MF_Replace_Edges_R1",
		"Function call not suitable for this representation",MSTK_WARN);
#endif
  }

  void MF_Rem_Edge_R1(MFace_ptr f, MEdge_ptr e) {
#ifdef DEBUG
    MSTK_Report("MF_Rem_Edge_R1",
		"Function call not suitable for this representation",MSTK_WARN);
#endif
  }

  void MF_Replace_Edges_i_R1(MFace_ptr f, int nold, int i, int nnu, MEdge_ptr *nuedges) {
#ifdef DEBUG
    MSTK_Report("MF_Replace_Edges_i_R1",
		"Function call not suitable for this representation",MSTK_WARN);
#endif
  }


  void MF_Add_Region_R1(MFace_ptr f, MRegion_ptr r, int side) {
#ifdef DEBUG
    MSTK_Report("MF_Add_Region_R1",
		"Function call not suitable for this representation",MSTK_WARN);
#endif
  }

  void MF_Rem_Region_R1(MFace_ptr f, MRegion_ptr r) {
#ifdef DEBUG
    MSTK_Report("MF_Rem_Region_R1",
		"Function call not suitable for this representation",MSTK_WARN);
#endif
  }


  int MF_Num_AdjFaces_R1(MFace_ptr f) {
    MSTK_Report("MF_Num_AdjFaces_R1",
		"Not yet implemented for this representation",MSTK_WARN);
    return 0;
  }

  List_ptr MF_AdjFaces_R1(MFace_ptr f) {
    MSTK_Report("MF_AdjFaces_R1",
		"Not yet implemented for this representation",MSTK_WARN);
    return 0;
  }

  void MF_Add_AdjFace_R1(MFace_ptr f, int side, MFace_ptr af) {
#ifdef DEBUG
    MSTK_Report("MF_Add_AdjFace_R1",
		"Function call not suitable for this representation",MSTK_WARN);
#endif
  }

  void MF_Rem_AdjFace_R1(MFace_ptr f, int side, MFace_ptr af) {
#ifdef DEBUG
    MSTK_Report("MF_Rem_AdjFace_R1",
		"Function call not suitable for this representation",MSTK_WARN);
#endif
  }

  MFace_ptr MF_NextInHash_R1(MFace_ptr f) {
    MFace_ptr hnext = ((MFace_Adj_R1 *)f->adj)->hnext; 
    return hnext;
  }

  void MF_Set_NextInHash_R1(MFace_ptr f, MFace_ptr next) {
    MFace_Adj_R1 *adj = (MFace_Adj_R1 *)f->adj;
    adj->hnext = next;
  }

  void MF_HashKey_R1(MFace_ptr f, unsigned int *pn, void* **pp) {
    MFace_Adj_R1 *adj = (MFace_Adj_R1 *)f->adj;
    *pn = List_Num_Entries(adj->fvertices);
    *pp = List_Entries(adj->fvertices);
  }

  void MF_Lock_R1(MFace_ptr f) {
    MFace_Adj_R1 *adj = (MFace_Adj_R1 *)f->adj;
    Hash_Lock(&adj->lock);
  }

  void MF_UnLock_R1(MFace_ptr f) {
    MFace_Adj_R1 *adj = (MFace_Adj_R1 *)f->adj;
    Hash_UnLock(&adj->lock);
  }

  int MF_IsLocked_R1(MFace_ptr f) {
    MFace_Adj_R1 *adj = (MFace_Adj_R1 *)f->adj;
    return Hash_IsLocked(adj->lock);
  }

#ifdef __cplusplus
}
#endif
