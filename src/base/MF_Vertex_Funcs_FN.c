#define _H_MFace_Private

#include "MFace.h"
#include "MFace_jmp.h"
#include "MSTK_private.h"
#include "MSTK_malloc.h"

#ifdef __cplusplus
extern "C" {
#endif

  void MF_Set_Vertices_FN(MFace_ptr f, int n, MVertex_ptr *v) {
    int i, vgdim[MAXPV2], vgid[MAXPV2], fedirs[MAXPV2], egdim, egid;
    MEdge_ptr fedges[MAXPV2];
    Mesh_ptr mesh = MF_Mesh(f);

    for (i = 0; i < n; i++) {
      vgdim[i] = MV_GEntDim(v[i]);
      vgid[i] = MV_GEntID(v[i]);
    }

    for (i = 0; i < n; i++) {
      egdim = 4;
      egid = 0;

      fedges[i] = MVs_CommonEdge(v[i],v[(i+1)%n]);
      if (fedges[i]) {
	fedirs[i] = (ME_Vertex(fedges[i],0) == v[i]) ? 1 : 0;
      }
      else {
	fedirs[i] = 1;
	fedges[i] = ME_New(mesh);
	
	ME_Set_Vertex(fedges[i],0,v[i]);
	ME_Set_Vertex(fedges[i],1,v[(i+1)%n]);

	if (vgdim[i] > vgdim[(i+1)%n]) {
	  egdim = vgdim[i];
	  egid = vgid[i];
	}
	else if (vgdim[(i+1)%n] > vgdim[i]) {
	  egdim = vgdim[(i+1)%n];
	  egid = vgid[i];
	}
	else { /* vgdim[i] == vgdim[(i+1)%n] */
	  if (vgdim[i] == 0) {
	    /* Both vertices are classified on model vertices. Cannot
	       say on what entity, the edge should be classified */
	    MSTK_Report("MF_Set_Vertices_FN",
			"Cannot determine edge classification. Guessing...",
			WARN);
	    egdim = 1;
	    egid = 0;
	  }
	  else {
	    egdim = vgdim[i];
	    egid = vgid[i];
	  }
	}

	ME_Set_GEntDim(fedges[i],egdim);
	ME_Set_GEntID(fedges[i],egid);
      }
    }

    MF_Set_Edges(f, n, fedges, fedirs);
  }


  List_ptr MF_Vertices_FN(MFace_ptr f, int dir, MVertex_ptr v0) {
    int i, k=0, ne, edir, fnd=0;
    List_ptr fverts;
    MEdge_ptr e;
    MVertex_ptr v;
    MFace_DownAdj_FN *downadj;

    downadj = (MFace_DownAdj_FN *) f->downadj;
    ne = downadj->ne;
    fverts = List_New(ne);

    if (!v0) {
      for (i = 0; i < ne; i++) {
	k = dir ? i : ne-1-i;
	e = List_Entry(downadj->fedges,k);
	edir = ((downadj->edirs)>>k) & 1;
	v = ME_Vertex(e,edir^dir);
	List_Add(fverts,v);
      }
    }
    else {
      fnd = 0;
      for (i = 0; i < ne; i++) {
        e = List_Entry(downadj->fedges,i);
        edir = ((downadj->edirs)>>i) & 1;
        if (ME_Vertex(e,edir^dir) == v0) {
          fnd = 1;
          k = i;
          break;
        }
      }

      if (!fnd)
        MSTK_Report("MF_Edges_F1","Cannot find vertex in face!!",FATAL);

      for (i = 0; i < ne; i++) {
	e = dir ? List_Entry(downadj->fedges,(k+i)%ne) :
	  List_Entry(downadj->fedges,(k+ne-i)%ne);
	edir = dir ? ((downadj->edirs)>>(k+i)%ne) & 1 :
	  ((downadj->edirs)>>(k+ne-i)%ne) & 1;
	v = ME_Vertex(e,edir^dir);
	List_Add(fverts,v);
      }
    }

    return fverts;
  }
	

  int MF_UsesVertex_FN(MFace_ptr f, MVertex_ptr v) {
    int ne, i;
    MEdge_ptr e;
    MFace_DownAdj_FN *downadj;

    /* We have to check only ne-1 edges since they form a loop */
    downadj = (MFace_DownAdj_FN *) f->downadj;
    ne = downadj->ne;
    for (i = 0; i < ne-1; i++) {
      e = List_Entry(downadj->fedges,i);
      if (ME_UsesEntity(e,v,0))
	return 1;
    }

    return 0;
  }


#ifdef __cplusplus
}
#endif
