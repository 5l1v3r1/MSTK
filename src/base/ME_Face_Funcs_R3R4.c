#define _H_MEdge_Private

#include "MSTK_malloc.h"
#include "MEdge.h"
#include "MEdge_jmp.h"
#include "MSTK_private.h"

#ifdef __cplusplus
extern "C" {
#endif

  int ME_Num_Faces_R3R4(MEdge_ptr e) {
    Set_ptr efaces;
    int nf;

#ifdef DEBUG
    MSTK_Report("ME_Num_Faces",
		"Inefficient to call this routine with this representation",
		MESG);
#endif

    efaces = ME_Faces(e);
    nf = efaces ? Set_Num_Entries(efaces) : 0;
    if (efaces) Set_Delete(efaces);

    return nf;
  }

  Set_ptr ME_Faces_R3R4(MEdge_ptr e) {
    int i, j, nef, nvf, nv, found;
    Set_ptr efaces, vfaces, fverts;
    MFace_ptr vface;
    MVertex_ptr ev[2];

    nef = 0;
    efaces = Set_New(10);

    for (i = 0; i < 2; i++)
      ev[i] = ME_Vertex(e,i);

    vfaces = MV_Faces(ev[0]);
    nvf = Set_Num_Entries(vfaces);

    /* Go through all faces connected to ev[0] and pick out ones which
       use this edge. Do this by checking if ev[0] and ev[1] are
       consecutive vertices in the face */
    for (i = 0; i < nvf; i++) {
      vface = Set_Entry(vfaces,i);
      
      /* Get vertices of i'th face connected to ev[0] */
      fverts = MF_Vertices(vface,1,0);
      nv = MF_Num_Vertices(vface);

      /* If the j'th vertex is ev[0], check if the next or the
         previous vertex is ev[1]; If it is then the edge is used by
         the face */

      found = 0;
      for (j = 0; j < nv; j++) {
	if (ev[0] == Set_Entry(fverts,j) &&
	    (ev[1] == Set_Entry(fverts,(j+1)%nv) ||
	     ev[1] == Set_Entry(fverts,(j+nv-1)%nv))) {
	  found = 1;
	  break;
	}
      }
      Set_Delete(fverts);

      if (found) {
	Set_Add(efaces,vface);
	nef++;
      }
    }
    Set_Delete(vfaces);

    if (nef)
      return efaces;
    else {
      Set_Delete(efaces);
      return 0;
    } 
  }

#ifdef __cplusplus
}
#endif

