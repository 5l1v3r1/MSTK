#define _H_MVertex_Private

#include "MVertex.h"
#include "MVertex_jmp.h"
#include "MSTK_private.h"
#include "MSTK_malloc.h"

#ifdef __cplusplus
extern "C" {
#endif


  void MV_Set_RepType_F4(MVertex_ptr v) {
    MVertex_Adj_F1F4 *adj;

    adj = v->adj = (MVertex_Adj_F1F4 *) MSTK_malloc(sizeof(MVertex_Adj_F1F4));
    adj->vedges = List_New(5);
  }

  void MV_Delete_F4(MVertex_ptr v, int keep) {
    MVertex_Adj_F1F4 *adj;

    if (!keep) {
      adj = (MVertex_Adj_F1F4 *) v->adj;
      if (adj) {
	if (adj->vedges)
	  List_Delete(adj->vedges);
	MSTK_free(adj);
      }
    }
  }

  void MV_Restore_F4(MVertex_ptr v) {
    MEnt_Set_Dim((MEntity_ptr) v,MVERTEX);
  }

  void MV_Destroy_For_MESH_Delete_F4(MVertex_ptr v) {
    MVertex_Adj_F1F4 *adj;

    adj = (MVertex_Adj_F1F4 *) v->adj;
    if (adj) {
      if (adj->vedges)
	List_Delete(adj->vedges);
      MSTK_free(adj);
    }
  }

  int MV_Num_AdjVertices_F4(MVertex_ptr v) {
    List_ptr vedges = ((MVertex_Adj_F1F4 *) v->adj)->vedges;
    return List_Num_Entries(vedges);
  }

  int MV_Num_Edges_F4(MVertex_ptr v) {
    List_ptr vedges = ((MVertex_Adj_F1F4 *) v->adj)->vedges;
    return List_Num_Entries(vedges);
  }

  int MV_Num_Faces_F4(MVertex_ptr v) {
    List_ptr vfaces;
    int nf;

#ifdef DEBUG
    MSTK_Report("MV_Num_Faces",
		"Ineficient to call this routine with this representation",
		MESG);
#endif
    
    vfaces = MV_Faces_F4(v);
    nf = List_Num_Entries(vfaces);
    List_Delete(vfaces);
    return nf;
  }

  int MV_Num_Regions_F4(MVertex_ptr v) {
    List_ptr vregions;
    int nr;

#ifdef DEBUG
    MSTK_Report("MV_Num_Regions",
		"Inefficient to call this routine with this representation",
		MESG);
#endif
	
    vregions = MV_Regions_F4(v);
    nr = List_Num_Entries(vregions);
    List_Delete(vregions);
    return nr;
  }

  List_ptr MV_AdjVertices_F4(MVertex_ptr v) {
    MVertex_Adj_F1F4 *adj;
    List_ptr vedges, adjv;
    int ne, i;
    MEdge_ptr vedge;
    MVertex_ptr ov;

    adj = (MVertex_Adj_F1F4 *) v->adj;
    vedges = adj->vedges;
    if (vedges == 0)
      return 0;

    ne = List_Num_Entries(vedges);
    adjv = List_New(ne);
    for (i = 0; i < ne; i++) {
      vedge = List_Entry(vedges,i);
      ov = ME_OppVertex(vedge,v);
      List_Add(adjv,ov);
    }

    return adjv;
  }
    

  List_ptr MV_Edges_F4(MVertex_ptr v) {
    MVertex_Adj_F1F4 *adj;
    List_ptr vedges;

    adj = (MVertex_Adj_F1F4 *) v->adj;
    vedges = List_Copy(adj->vedges);
    return vedges;
  }

  List_ptr MV_Faces_F4(MVertex_ptr v) {
    MVertex_Adj_F1F4 *adj;
    int i, j, k, ne, nf, nr, n, mkr;
    List_ptr vedges, eregions, rfaces, efaces, vfaces;
    MEdge_ptr edge;
    MFace_ptr face;
    MRegion_ptr region;

    adj = (MVertex_Adj_F1F4 *) v->adj;
    vedges = adj->vedges;
    ne = List_Num_Entries(vedges);

    n = 0;
    vfaces = List_New(ne);
    mkr = MSTK_GetMarker();

    for (i = 0; i < ne; i++) {
      edge = List_Entry(vedges,i);
      eregions = ME_Regions(edge);
      if (eregions) {
	nr = List_Num_Entries(eregions);
	
	for (j = 0; j < nr; j++) {
	  region = List_Entry(eregions,j);
	  
	  rfaces = MR_Faces(region);
	  nf = List_Num_Entries(rfaces);
	  
	  for (k = 0; k < nf; k++) {
	    face = List_Entry(rfaces,k);

	    if (!MEnt_IsMarked(face,mkr)) {
	      if (MF_UsesEntity(face,(MEntity_ptr) v,0)) {		
		MEnt_Mark(face,mkr);
		List_Add(vfaces,face);
		n++;
	      }
	    }
	  }
	  List_Delete(rfaces);
	}
	List_Delete(eregions);
      }
      else {
	/* perhaps the edge has boundary faces (not connected to regions) */
	efaces = ME_Faces(edge);
	if (efaces) {
	  nf = List_Num_Entries(efaces);
	  
	  for (k = 0; k < nf; k++) {
	    face = List_Entry(efaces,k);
	    if (!MEnt_IsMarked(face,mkr)) {
	      if (MF_UsesEntity(face,(MEntity_ptr) v,0)) {
		MEnt_Mark(face,mkr);
		List_Add(vfaces,face);
		n++;
	      }
	    }
	  }
	  List_Delete(efaces);
	}
      }
    }
    List_Unmark(vfaces,mkr);
    MSTK_FreeMarker(mkr);
    if (n > 0)
      return vfaces;
    else {
      List_Delete(vfaces);
      return 0;
    }
  }

  List_ptr MV_Regions_F4(MVertex_ptr v) {
    MVertex_Adj_F1F4 *adj;
    int i, j, ne, nr, n, mkr;
    List_ptr vedges, eregions, vregions;
    MEdge_ptr edge;
    MRegion_ptr region;

    adj = (MVertex_Adj_F1F4 *) v->adj;
    vedges = adj->vedges;
    ne = List_Num_Entries(vedges);

    n = 0;
    vregions = List_New(ne);
    mkr = MSTK_GetMarker();

    for (i = 0; i < ne; i++) {
      edge = List_Entry(vedges,i);

      eregions = ME_Regions(edge);
      if (eregions) {
	nr = List_Num_Entries(eregions);
	
	for (j = 0; j < nr; j++) {
	  region = List_Entry(eregions,j);

	  if (!MEnt_IsMarked(region,mkr)) {
	    MEnt_Mark(region,mkr);
	    List_Add(vregions,region);
	    n++;
	  }
	}
	
	List_Delete(eregions);
      }
    }
    List_Unmark(vregions,mkr);
    MSTK_FreeMarker(mkr);

    if (n > 0)
      return vregions;
    else {
      List_Delete(vregions);
      return 0;
    }
  }

  void MV_Add_AdjVertex_F4(MVertex_ptr v, MVertex_ptr av) {
#ifdef DEBUG
    MSTK_Report("MV_Add_AdjVertex","Function call not suitable for this representation",WARN);
#endif
  }

  void MV_Rem_AdjVertex_F4(MVertex_ptr v, MVertex_ptr av) {
#ifdef DEBUG
    MSTK_Report("MV_Rem_AdjVertex","Function call not suitable for this representation",WARN);
#endif
  }

  void MV_Add_Edge_F4(MVertex_ptr v, MEdge_ptr e) {
    MVertex_Adj_F1F4 *adj;

    adj = (MVertex_Adj_F1F4 *) v->adj;

    if (adj->vedges == NULL)
      adj->vedges = List_New(10);
    List_ChknAdd(adj->vedges,e);
  }

  void MV_Rem_Edge_F4(MVertex_ptr v, MEdge_ptr e) {
    MVertex_Adj_F1F4 *adj;

    adj = (MVertex_Adj_F1F4 *) v->adj;

    if (adj->vedges == NULL)
      return;
    List_Rem(adj->vedges,e);
  }

  void MV_Add_Face_F4(MVertex_ptr v, MFace_ptr f) {
#ifdef DEBUG
    MSTK_Report("MV_Add_Face","Function call not suitable for this representation",WARN);
#endif
  }

  void MV_Rem_Face_F4(MVertex_ptr v, MFace_ptr f) {
#ifdef DEBUG
    MSTK_Report("MV_Rem_Face","Function call not suitable for this representation",WARN);
#endif
  }

  void MV_Add_Region_F4(MVertex_ptr v, MRegion_ptr f) {
#ifdef DEBUG
    MSTK_Report("MV_Add_Region","Function call not suitable for this representation",WARN);
#endif
  }

  void MV_Rem_Region_F4(MVertex_ptr v, MRegion_ptr f) {
#ifdef DEBUG
    MSTK_Report("MV_Rem_Region","Function call not suitable for this representation",WARN);
#endif
  }

#ifdef __cplusplus
}
#endif
