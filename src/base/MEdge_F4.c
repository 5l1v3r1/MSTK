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
    upadj->nel = 0;
    upadj->elements = Set_New(10);
  }

  void ME_Delete_F4(MEdge_ptr e) {
    MEdge_UpAdj_F4 *upadj;

    upadj = (MEdge_UpAdj_F4 *) e->upadj;
    Set_Delete(upadj->elements);
    MSTK_free(upadj);
  }

  int ME_Num_Faces_F4(MEdge_ptr e) {
    Set_ptr efaces;
    int nf;

#ifdef DEBUG
    MSTK_Report("ME_Num_Faces",
       	"May be inefficient to call this routine with this representation",
		MESG);
#endif
    
    efaces = ME_Faces_F4(e);
    nf = efaces ? Set_Num_Entries(efaces) : 0;
    if (efaces) Set_Delete(efaces);

    return nf;
  }


  /* Edge could be connected up to regions or isolated faces - so we
     can't just return e->upadj->nel */
  int ME_Num_Regions_F4(MEdge_ptr e) {
    int nr;
    Set_ptr eregions;
    
    eregions = ME_Regions_F4(e);
    nr = eregions ? Set_Num_Entries(eregions) : 0;
    if (eregions) Set_Delete(eregions);
    
    return nr;
  }


  Set_ptr ME_Faces_F4(MEdge_ptr e) {
    MEdge_UpAdj_F4 *upadj;
    Set_ptr efaces, rfaces;
    int nf, nrf, nel, mkr, i, j, dim;
    MFace_ptr rface;
    MEntity_ptr ent;

    upadj = (MEdge_UpAdj_F4 *) e->upadj;

    efaces = Set_New(10);
    nf = 0;
    mkr = MSTK_GetMarker();

    nel = upadj->nel;
    for (i = 0; i < nel; i++) {
      ent = (MEntity_ptr) Set_Entry(upadj->elements,i);
      dim = MEnt_Dim(ent);
      if (dim == 2) {
	if (!MEnt_IsMarked(ent,j)) {
	  MEnt_Mark(ent,mkr);
	  Set_Add(efaces,ent);
	  nf++;
	}
      }
      else if (dim == 3) {
	rfaces = MR_Faces((MRegion_ptr)ent);
	nrf = Set_Num_Entries(rfaces);

	for (j = 0; j < nrf; j++) {
	  rface = Set_Entry(rfaces,j);

	  if (!MEnt_IsMarked(rface,j)) {
	    if (MF_UsesEntity(rface,e,1)) {
	      MEnt_Mark(rface,mkr);
	      Set_Add(efaces,rface);
	      nf++;
	    }
	  }

	}

	Set_Delete(rfaces);
      }
    }
    if (nf) {
      Set_Unmark(efaces,mkr);
      MSTK_FreeMarker(mkr);
      return efaces;
    }
    else {
      Set_Delete(efaces);
      return 0;
    }

  }


  Set_ptr ME_Regions_F4(MEdge_ptr e) {
    int nr, nel, i;
    MEntity_ptr ent;
    MEdge_UpAdj_F4 *upadj;
    Set_ptr eregs;

    upadj = (MEdge_UpAdj_F4 *) e->upadj;
    
    nr = 0;
    eregs = Set_New(10);

    nel = upadj->nel;
    for (i = 0; i < nel; i++) {
      ent = Set_Entry(upadj->elements,i);
      if (MEnt_Dim(ent) == 3) {
	Set_Add(eregs,ent);
	nr++;
      }
    }

    if (nr)
      return eregs;
    else {
      Set_Delete(eregs);
      return 0;
    }
  }

  void ME_Add_Face_F4(MEdge_ptr e, MFace_ptr f) {
    MEdge_UpAdj_F4 *upadj;

    upadj = (MEdge_UpAdj_F4 *) e->upadj;

    if (upadj->elements == NULL)
      upadj->elements = Set_New(10);
    Set_Add(upadj->elements,f);
    (upadj->nel)++;
  }

  void ME_Rem_Face_F4(MEdge_ptr e, MFace_ptr f) {
    MEdge_UpAdj_F4 *upadj;
    Set_ptr fregs;
    int ok;

    upadj = (MEdge_UpAdj_F4 *) e->upadj;

    ok = Set_Rem(upadj->elements,f);
    if (ok) (upadj->nel)--;
  }

  void ME_Add_Region_F4(MEdge_ptr e, MRegion_ptr r) {
    MEdge_UpAdj_F4 *upadj;

    upadj = (MEdge_UpAdj_F4 *) e->upadj;
    if (upadj->elements == NULL)
      upadj->elements = Set_New(10);
    Set_Add(upadj->elements,r);
    (upadj->nel)++;
  }

  void ME_Rem_Region_F4(MEdge_ptr e, MRegion_ptr r) {
    MEdge_UpAdj_F4 *upadj;
    int ok;

    upadj = (MEdge_UpAdj_F4 *) e->upadj;
    ok = Set_Rem(upadj->elements,r);
    if (ok) (upadj->nel)--;
  }

#ifdef __cplusplus
}
#endif

