/* 
Copyright 2019 Triad National Security, LLC. All rights reserved.

This file is part of the MSTK project. Please see the license file at
the root of this repository or at
https://github.com/MeshToolkit/MSTK/blob/master/LICENSE
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "metis.h"

#include "MSTK.h"

/* Get the partitioning of mesh using Metis - Doesn't actually do 
   anything to the mesh */

#ifdef __cplusplus
extern "C" {
#endif


int MESH_PartitionWithMetis(Mesh_ptr mesh, int nparts, int **part) {

  MEdge_ptr fedge;
  MFace_ptr mf, oppf, rface;
  MRegion_ptr mr, oppr;
  List_ptr fedges, efaces, rfaces, fregions;
  int  i, ncells, ipos;
  int  nv, ne, nf, nr, nfe, nef, nfr, nrf, idx, idx2;
  idx_t ngraphvtx, numflag, nedgecut, numparts, ncons;
  idx_t wtflag, metisopts[METIS_NOPTIONS];
  idx_t *vsize, *idxpart;
  idx_t  *xadj, *adjncy, *vwgt, *adjwgt;
  real_t *tpwgts, *ubvec;
  

  /* First build a nodal graph of the mesh in the format required by
     metis */

  nv = MESH_Num_Vertices(mesh);
  ne = MESH_Num_Edges(mesh);
  nf = MESH_Num_Faces(mesh);
  nr = MESH_Num_Regions(mesh);

  ipos = 0;
  
  if (nr == 0) {
    
    if (nf == 0) {
      fprintf(stderr,"Cannot partition wire meshes\n");
      exit(-1);
    }

    xadj = (idx_t *) malloc((nf+1)*sizeof(idx_t));
    adjncy = (idx_t *) malloc(2*ne*sizeof(idx_t));
    ncells = nf;

    /* Surface mesh */

    idx = 0; i = 0;
    xadj[i] = ipos;
    while ((mf = MESH_Next_Face(mesh,&idx))) {
      
      fedges = MF_Edges(mf,1,0);
      nfe = List_Num_Entries(fedges);
      
      idx2 = 0;
      while ((fedge = List_Next_Entry(fedges,&idx2))) {
	
	efaces = ME_Faces(fedge);
	nef = List_Num_Entries(efaces);
	
	if (nef == 1) {
	  continue;          /* boundary edge; nothing to do */
	}
	else {
          int j;
          for (j = 0; j < nef; j++) {
            oppf = List_Entry(efaces,j);
            if (oppf != mf) {
              adjncy[ipos] = MF_ID(oppf)-1;
              ipos++;
            }
          }
	}
	
	List_Delete(efaces);
	
      }
      
      List_Delete(fedges);
      
      i++;
      xadj[i] = ipos;
    }

  }
  else {

    xadj = (idx_t *) malloc((nr+1)*sizeof(idx_t));
    adjncy = (idx_t *) malloc(2*nf*sizeof(idx_t));
    ncells = nr;

    /* Volume mesh */

    idx = 0; i = 0;
    xadj[i] = ipos;
    while ((mr = MESH_Next_Region(mesh,&idx))) {
      
      rfaces = MR_Faces(mr);
      nrf = List_Num_Entries(rfaces);
      
      idx2 = 0;
      while ((rface = List_Next_Entry(rfaces,&idx2))) {
	
	fregions = MF_Regions(rface);
	nfr = List_Num_Entries(fregions);
	
	if (nfr > 1) {
	  oppr = List_Entry(fregions,0);
	  if (oppr == mr)
	    oppr = List_Entry(fregions,1);
	  
	  adjncy[ipos] = MR_ID(oppr)-1;
	  ipos++;
	}
        List_Delete(fregions);	
	
      }
      
      List_Delete(rfaces);
      
      i++;
      xadj[i] = ipos;
    }

  }
  


  /* Partition the graph */
  
  wtflag = 0;        /* No weights are specified */
  vwgt = adjwgt = NULL;

  numflag = 0;    /* C style numbering of elements (nodes of the dual graph) */
  ngraphvtx = ncells; /* we want the variable to be of type idxtype or idx_t */
  numparts = nparts;  /* we want the variable to be of type idxtype or idx_t */

  idxpart = (idx_t *) malloc(ncells*sizeof(idx_t));

  ncons = 1;  /* Number of constraints */
  vsize = NULL;  
  tpwgts = NULL;
  ubvec = NULL;

  METIS_SetDefaultOptions(metisopts);
  metisopts[METIS_OPTION_NUMBERING] = 0;

  if (nparts <= 8)
    METIS_PartGraphRecursive(&ngraphvtx,&ncons,xadj,adjncy,vwgt,vsize,adjwgt,
			     &numparts,tpwgts,ubvec,metisopts,&nedgecut,
                             idxpart);
  else
    METIS_PartGraphKway(&ngraphvtx,&ncons,xadj,adjncy,vwgt,vsize,adjwgt,
                        &numparts,tpwgts,ubvec,metisopts,&nedgecut,idxpart);


  free(xadj);
  free(adjncy);


  
  *part = (int *) malloc(ncells*sizeof(int));
  for (i = 0; i < ncells; i++)
    (*part)[i] = (int) idxpart[i];

  free(idxpart);
  return 1;

}

#ifdef __cplusplus
  }
#endif

