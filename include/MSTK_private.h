#ifndef _H_MSTK_PRIVATE
#define _H_MSTK_PRIVATE

#ifdef   __cplusplus
extern "C" {
#endif

  /* #include "gmtk.h" */
#include "MSTK_types.h"
#include "MSTK_util.h"
#include "List.h"
#include "MSTK_malloc.h"
#include "MSTK.h"

/* If MSTK_KEEP_DELETED is 1, then entities will only be marked as deleted */

extern int MSTK_KEEP_DELETED;

/* Don't want users to see this */

typedef enum MDelType {MDELREGION=-40, MDELFACE=-30, MDELEDGE=-20, MDELVERTEX=-10} MDelType;


/* THIS FILE HAS ADDITIONAL FUNCTIONS THAT THE NORMAL USER NEED NOT SEE */


  void       MESH_Add_Vertex(Mesh_ptr mesh, MVertex_ptr v);
  void       MESH_Add_Edge(Mesh_ptr mesh, MEdge_ptr e);
  void       MESH_Add_Face(Mesh_ptr mesh, MFace_ptr f);
  void       MESH_Add_Region(Mesh_ptr mesh, MRegion_ptr r);

  void       MESH_Rem_Vertex(Mesh_ptr mesh, MVertex_ptr v);
  void       MESH_Rem_Edge(Mesh_ptr mesh, MEdge_ptr e);
  void       MESH_Rem_Face(Mesh_ptr mesh, MFace_ptr f);
  void       MESH_Rem_Region(Mesh_ptr mesh, MRegion_ptr r);


/*
  void MV_Set_RepType(MVertex_ptr v, RepType rtype);
*/
  void MV_Add_Edge(MVertex_ptr mvertex, MEdge_ptr medge);
  void MV_Add_Face(MVertex_ptr mvertex, MFace_ptr mface);
  void MV_Add_Region(MVertex_ptr mvertex, MRegion_ptr mregion);
  void MV_Rem_Edge(MVertex_ptr mvertex, MEdge_ptr medge);
  void MV_Rem_Face(MVertex_ptr mvertex, MFace_ptr mface);
  void MV_Rem_Region(MVertex_ptr mvertex, MRegion_ptr mregion);

/*
  void ME_Set_RepType(MEdge_ptr medge, RepType rtype*/

  void ME_Add_Face(MEdge_ptr medge, MFace_ptr mface);
  void ME_Add_Region(MEdge_ptr medge, MRegion_ptr mregion);
  void ME_Rem_Face(MEdge_ptr medge, MFace_ptr mface);
  void ME_Rem_Region(MEdge_ptr medge, MRegion_ptr mregion);



/*
  void MF_Set_RepType(MFace_ptr f, RepType rtype);
*/
  /* Add, Remove AdjFace can be automatically called when faces are
     being created or deleted. They do not need user invocation */

  void MF_Add_AdjFace(MFace_ptr f, int enbr, MFace_ptr af);
  void MF_Rem_AdjFace(MFace_ptr f, int enbr, MFace_ptr af);


  /* Add, Remove Region can be called internally when a region is
     being created from a list of faces */

  void MF_Add_Region(MFace_ptr f, MRegion_ptr r, int side);
  void MF_Rem_Region(MFace_ptr f, MRegion_ptr r);


  /*
  void MR_Set_RepType(MRegion_ptr r, RepType rtype);
  */

  /* Adjacent region info will be updated by private functions when
     regions are created or deleted. There is no need for invocation
     by users/applications */

  void MR_Add_AdjRegion(MRegion_ptr r, int facenum, MRegion_ptr ar);
  void MR_Rem_AdjRegion(MRegion_ptr r, MRegion_ptr ar);


#ifdef __cplusplus
	   }
#endif

#endif
