#define _H_MFace_Private

#include "MFace.h"
#include "MSTK_private.h"
#include "MSTK_malloc.h"

#ifdef __cplusplus
extern "C" {
#endif

  List_ptr MF_Regions_R1R2(MFace_ptr f) {
    MFace_DownAdj_RN *downadj = (MFace_DownAdj_RN *) f->downadj;
    MVertex_ptr v, rv;
    MRegion_ptr r;
    List_ptr vregs, rverts, fregs;
    int nfv, nr, idx, i, fnd=0, idx1;
    
    v = List_Entry(downadj->fvertices,0);
    vregs = MV_Regions(v);
    if (!vregs)
      return NULL;

      nfv = List_Num_Entries(downadj->fvertices);
    nr = 0;
    fregs = List_New(2);

    idx = 0;
    while (nr < 2 && (r = List_Next_Entry(vregs,&idx))) {
      rverts = MR_Vertices(r);

      for (i = 1; i < nfv; i++) {
	v = List_Entry(downadj->fvertices,i);
	idx1 = 0;
	fnd = 0;
	while ((rv = List_Next_Entry(rverts,&idx1))) {
	  if (rv == v) {
	    fnd = 1;
	    break;
	  }
	}
	if (!fnd)
	  break;
      }
      
      List_Delete(rverts);

      if (fnd) {
	List_Add(fregs,r);
	nr++;
      }
    }
    return 0;    
  }

  MRegion_ptr MF_Region_R1R2(MFace_ptr f, int dir) {
    List_ptr fregs;
    int nr;
    MRegion_ptr reg;

#ifdef DEBUG
    MSTK_Report("MF_Region_R1R2",
		"May be more efficient to call MF_Regions",WARN);
#endif

    fregs = MF_Regions_R1R2(f);
    if (!fregs)
      return NULL;

    nr = List_Num_Entries(fregs);

    reg = List_Entry(fregs,0);
    if (MR_FaceDir(reg,f) == !dir)
      return reg;

    if (nr == 2) {
      reg = List_Entry(fregs,1);
      if (MR_FaceDir(reg,f) == !dir)
	return reg;
    }

    return NULL;
  }


#ifdef __cplusplus
}
#endif
