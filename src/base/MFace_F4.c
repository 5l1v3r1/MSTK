#define _H_MFace_Private

#include "MFace.h"
#include "MFace_jmp.h"
#include "MSTK_private.h"
#include "MSTK_malloc.h"

#ifdef __cplusplus
extern "C" {
#endif


  void MF_Set_RepType_F4(MFace_ptr f) {
    MFace_DownAdj_FN *downadj;

    downadj = f->downadj = (MFace_DownAdj_FN *) MSTK_malloc(sizeof(MFace_DownAdj_FN));
    downadj->ne = 0;
    downadj->edirs = 0;
    downadj->fedges = NULL;
  }

  void MF_Delete_F4(MFace_ptr f, int keep) {
    MFace_DownAdj_FN *downadj;
    int i, ne;
    MEdge_ptr e;

    if (keep) {
      MSTK_KEEP_DELETED = 1;
      f->dim = MDELFACE;
    }
    else {
#ifdef DEBUG
      f->dim = MDELFACE;
#endif

      downadj = (MFace_DownAdj_FN *) f->downadj;
      List_Delete(downadj->fedges);
      MSTK_free(downadj);

      MSTK_free(f);
    }
  }

  void MF_Restore_F4(MFace_ptr f) {
    if (f->dim != MDELFACE)
      return;
    f->dim = MFACE;
  }

  void MF_Set_Edges_F4(MFace_ptr f, int n, MEdge_ptr *e, int *dir) {
    MF_Set_Edges_FN(f,n,e,dir);
  }

  void MF_Replace_Edges_F4(MFace_ptr f, int nold, MEdge_ptr *oldedges,  int nnu, MEdge_ptr *nuedges) {
    MF_Replace_Edges_FN(f,nold,oldedges,nnu,nuedges);
  }

  void MF_Replace_Edges_i_F4(MFace_ptr f, int nold, int i, int nnu, MEdge_ptr *nuedges) {
    MF_Replace_Edges_i_FN(f,nold,i,nnu,nuedges);
  }

  void MF_Set_Vertices_F4(MFace_ptr f, int n, MVertex_ptr *v) {
#ifdef DEBUG
    MSTK_Report("MF_Set_Vertices","Function call not suitable for this representation",WARN);
#endif
  }

  void MF_Replace_Vertex_i_F4(MFace_ptr f, int i, MVertex_ptr v) {
#ifdef DEBUG
    MSTK_Report("MF_Replace_Vertex","Function call not suitable for this representation",WARN);
#endif
  }

  void MF_Replace_Vertex_F4(MFace_ptr f, MVertex_ptr v, MVertex_ptr nuv) {
#ifdef DEBUG
    MSTK_Report("MF_Replace_Vertex","Function call not suitable for this representation",WARN);
#endif
  }

  void MF_Insert_Vertex_F4(MFace_ptr f, MVertex_ptr nuv, MVertex_ptr b4v) {
#ifdef DEBUG
    MSTK_Report("MF_Insert_Vertex","Function call not suitable for this representation",WARN);
#endif
  }

  void MF_Insert_Vertex_i_F4(MFace_ptr f, MVertex_ptr nuv, int i) {
#ifdef DEBUG
    MSTK_Report("MF_Insert_Vertex_i","Function call not suitable for this representation",WARN);
#endif
  }

  int MF_Num_Vertices_F4(MFace_ptr f) {
    return ((MFace_DownAdj_FN *)f->downadj)->ne;
  }

  int MF_Num_Edges_F4(MFace_ptr f) {
    return ((MFace_DownAdj_FN *)f->downadj)->ne;
  }

  int MF_Num_AdjFaces_F4(MFace_ptr f) {

#ifdef DEBUG
    MSTK_Report("MF_Num_AdjFaces",
		"Function call unsuitable for this representation",
		WARN);
#endif
    
    return 0;
  }

  List_ptr MF_Vertices_F4(MFace_ptr f, int dir, MVertex_ptr v0) {
    return MF_Vertices_FN(f,dir,v0);
  }
	

  List_ptr MF_Edges_F4(MFace_ptr f, int dir, MVertex_ptr v0) {
    return MF_Edges_FN(f,dir,v0);
  }

  int MF_EdgeDir_F4(MFace_ptr f, MEdge_ptr e) {
    return MF_EdgeDir_FN(f,e);
  }

  int MF_EdgeDir_i_F4(MFace_ptr f, int i) {
    return MF_EdgeDir_i_FN(f,i);
  }

  int MF_UsesEdge_F4(MFace_ptr f, MEdge_ptr e) {
    return MF_UsesEdge_FN(f,e);
  }

  int MF_UsesVertex_F4(MFace_ptr f, MVertex_ptr v) {
    return MF_UsesVertex_FN(f,v);
  }

  List_ptr MF_AdjFaces_F4(MFace_ptr f) {
#ifdef DEBUG
    MSTK_Report("MF_AdjFaces",
		"Function call not suitable for this representation",WARN);
#endif
    return NULL;
  }
  
  List_ptr MF_Regions_F4(MFace_ptr f) {
    List_ptr fregs, eregs;
    MRegion_ptr r;
    MEdge_ptr e;
    MFace_DownAdj_FN *downadj;
    int i, k=0, nr;
    
    downadj = (MFace_DownAdj_FN *) f->downadj;
    
    fregs = List_New(2);
    e = List_Entry(downadj->fedges,0);
    
    eregs = ME_Regions(e);
    if (eregs) {
      nr = List_Num_Entries(eregs);
      
      for (i = 0; i < nr; i++) {
	r = List_Entry(eregs,i);
	if (MR_UsesEntity(r,f,2)) {
	  List_Add(fregs,r);
	  k++;
	}
	if (k == 2)
	  break;
      }
      
      List_Delete(eregs);
    }
			
    
    if (k) 
      return fregs;
    else {
      List_Delete(fregs);
      return 0;
    }
  }
  
  MRegion_ptr MF_Region_F4(MFace_ptr f, int dir) {
    List_ptr eregs;
    MRegion_ptr r, r1;
    MEdge_ptr e;
    int nr, i, fdir;
    MFace_DownAdj_FN *downadj;
    
#ifdef DEBUG
    MSTK_Report("MF_Region_F4","More efficient to use MF_Regions",MESG);
#endif
    
    downadj = (MFace_DownAdj_FN *) f->downadj;
    e = List_Entry(downadj->fedges,0);
    
    eregs = ME_Regions(e);
    if (eregs) {
      nr = List_Num_Entries(eregs);
      
      r1 = 0;
      for (i = 0; i < nr; i++) {
	r = List_Entry(eregs,i);
	fdir = MR_FaceDir(r,f);
	if (fdir == !dir) {
	  r1 = r;
	  break;
	}
      }
      List_Delete(eregs);
	
      return r1;
    }
    
    return 0;
  }

  void MF_Add_AdjFace_F4(MFace_ptr f, int edgnum, MFace_ptr af) {
#ifdef DEBUG
    MSTK_Report("MF_Add_AdjFace",
		"Function call unsuitable for this representation",
		WARN);
#endif
  }

  void MF_Rem_AdjFace_F4(MFace_ptr f, int edgnum, MFace_ptr af) {
#ifdef DEBUG
    MSTK_Report("MF_Rem_AdjFace",
		"Function call unsuitable for this representation",
		WARN);
#endif
  }

  void MF_Add_Region_F4(MFace_ptr f, MRegion_ptr r, int side) {
#ifdef DEBUG
    MSTK_Report("MF_Add_Region",
		"Function call unsuitable for this representation",
		WARN);
#endif
  }

  void MF_Rem_Region_F4(MFace_ptr f, MRegion_ptr r) {
#ifdef DEBUG
    MSTK_Report("MF_Rem_Region",
		"Function call unsuitable for this representation",
		WARN);
#endif
  }

#ifdef __cplusplus
}
#endif
