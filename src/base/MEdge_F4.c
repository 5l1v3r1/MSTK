#define _H_MEdge_Private

#include "MEdge.h"
#include "MEdge_jmp.h"
#include "MSTK_private.h"
#include "MSTK_malloc.h"

#ifdef __cplusplus
extern "C" {
#endif

  void ME_Set_RepType_F4(MEdge_ptr e) {
    MEdge_UpAdj_F4 *upadj;

    upadj = e->upadj = (MEdge_UpAdj_F4 *) MSTK_malloc(sizeof(MEdge_UpAdj_F4));
    upadj->elements = List_New(5);
  }

  void ME_Delete_F4(MEdge_ptr e, int keep) {
    MEdge_UpAdj_F4 *upadj;

    if (MEnt_Dim(e) != MDELETED) { /* if edge hasnt been temporarily deleted */
      MV_Rem_Edge(e->vertex[0],e);
      MV_Rem_Edge(e->vertex[1],e);
    }

    if (!keep) {
      upadj = (MEdge_UpAdj_F4 *) e->upadj;
      if (upadj) {
	if (upadj->elements)
	  List_Delete(upadj->elements);
	MSTK_free(upadj);
      }
    }
  }

  void ME_Restore_F4(MEdge_ptr e) {
 
    MEnt_Set_Dim(e,MEDGE);

    MV_Add_Edge(e->vertex[0],e);
    MV_Add_Edge(e->vertex[1],e);
  }

  void ME_Destroy_For_MESH_Delete_F4(MEdge_ptr e) {
    MEdge_UpAdj_F4 *upadj;

    upadj = (MEdge_UpAdj_F4 *) e->upadj;
    if (upadj) {
      if (upadj->elements)
	List_Delete(upadj->elements);
      MSTK_free(upadj);
    }
  }

  int ME_Num_Faces_F4(MEdge_ptr e) {
    List_ptr efaces;
    int nf;

#ifdef DEBUG
    MSTK_Report("ME_Num_Faces",
       	"May be inefficient to call this routine with this representation",
		MESG);
#endif
    
    efaces = ME_Faces_F4(e);
    nf = efaces ? List_Num_Entries(efaces) : 0;
    if (efaces) List_Delete(efaces);

    return nf;
  }


  /* Edge could be connected up to regions or isolated faces - so we
     can't just return e->upadj->nel */
  int ME_Num_Regions_F4(MEdge_ptr e) {
    int nr;
    List_ptr eregions;
    
    eregions = ME_Regions_F4(e);
    nr = eregions ? List_Num_Entries(eregions) : 0;
    if (eregions) List_Delete(eregions);
    
    return nr;
  }


  List_ptr ME_Faces_F4(MEdge_ptr e) {
    MEdge_UpAdj_F4 *upadj;
    List_ptr efaces, rfaces;
    int nf, nrf, nel, mkr, i, j, dim;
    MFace_ptr rface;
    MEntity_ptr ent;

    upadj = (MEdge_UpAdj_F4 *) e->upadj;

    nel = List_Num_Entries(upadj->elements);

    efaces = List_New(nel);
    nf = 0;
    mkr = MSTK_GetMarker();

    for (i = 0; i < nel; i++) {
      ent = (MEntity_ptr) List_Entry(upadj->elements,i);
      dim = MEnt_Dim(ent);
      if (dim == MFACE) {
	if (!MEnt_IsMarked(ent,mkr)) {
	  MEnt_Mark(ent,mkr);
	  List_Add(efaces,ent);
	  nf++;
	}
      }
      else if (dim == MREGION) {
	rfaces = MR_Faces((MRegion_ptr)ent);
	nrf = List_Num_Entries(rfaces);

	for (j = 0; j < nrf; j++) {
	  rface = List_Entry(rfaces,j);

	  if (!MEnt_IsMarked(rface,mkr)) {
	    if (MF_UsesEntity(rface,e,1)) {
	      MEnt_Mark(rface,mkr);
	      List_Add(efaces,rface);
	      nf++;
	    }
	  }

	}

	List_Delete(rfaces);
      }
    }
    if (nf) {
      List_Unmark(efaces,mkr);
      MSTK_FreeMarker(mkr);
      return efaces;
    }
    else {
      List_Delete(efaces);
      return 0;
    }

  }


  List_ptr ME_Regions_F4(MEdge_ptr e) {
    int nr, nel, i;
    MEntity_ptr ent;
    MEdge_UpAdj_F4 *upadj;
    List_ptr eregs;

    upadj = (MEdge_UpAdj_F4 *) e->upadj;
    
    nel = List_Num_Entries(upadj->elements);

    nr = 0;
    eregs = List_New(nel);

    for (i = 0; i < nel; i++) {
      ent = List_Entry(upadj->elements,i);
      if (MEnt_Dim(ent) == MREGION) {
	List_Add(eregs,ent);
	nr++;
      }
    }

    if (nr)
      return eregs;
    else {
      List_Delete(eregs);
      return 0;
    }
  }

  void ME_Add_Face_F4(MEdge_ptr e, MFace_ptr f) {
    MEdge_UpAdj_F4 *upadj;

    upadj = (MEdge_UpAdj_F4 *) e->upadj;

    if (upadj->elements == NULL)
      upadj->elements = List_New(10);
    List_Add(upadj->elements,f);
  }

  void ME_Rem_Face_F4(MEdge_ptr e, MFace_ptr f) {
    MEdge_UpAdj_F4 *upadj;
    List_ptr fregs;
    int ok;

    upadj = (MEdge_UpAdj_F4 *) e->upadj;

    List_Rem(upadj->elements,f);
  }

  void ME_Add_Region_F4(MEdge_ptr e, MRegion_ptr r) {
    MEdge_UpAdj_F4 *upadj;

    upadj = (MEdge_UpAdj_F4 *) e->upadj;
    if (upadj->elements == NULL)
      upadj->elements = List_New(10);
    List_Add(upadj->elements,r);
  }

  void ME_Rem_Region_F4(MEdge_ptr e, MRegion_ptr r) {
    MEdge_UpAdj_F4 *upadj;
    int ok;

    upadj = (MEdge_UpAdj_F4 *) e->upadj;
    ok = List_Rem(upadj->elements,r);
  }

#ifdef __cplusplus
}
#endif

