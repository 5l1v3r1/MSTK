#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "MSTK.h"

#include "exodusII.h"
#include "exodusII_ext.h"


#ifdef __cplusplus
extern "C" {
#endif


  /* Right now we are creating an attribute for material sets, side
     sets and node sets. We are ALSO creating meshsets for each of
     these entity sets. We are also setting mesh geometric entity IDs
     - Should we pick one of the first two? */


int MESH_ImportFromExodusII(Mesh_ptr mesh, const char *filename) {

  char mesg[256], funcname[256]="MESH_ImportFromExodusII";
  char title[256], elem_type[256], sidesetname[256], nodesetname[256];
  char matsetname[256];
  char **elem_blknames;
  int i, j, k, k1;
  int comp_ws = sizeof(double), io_ws = 0;
  int exoid, status;
  int ndim, nnodes, nelems, nelblock, nnodesets, nsidesets;
  int nedges, nedge_blk, nfaces, nface_blk, nelemsets;
  int nedgesets, nfacesets, nnodemaps, nedgemaps, nfacemaps, nelemmaps;
  int *elem_blk_ids, *connect, *node_map, *elem_map, *nnpe;
  int nelnodes, neledges, nelfaces, ntotnodes, ntotedges, ntotfaces;
  int nelem_i, natts;
  int *sideset_ids, *ss_elem_list, *ss_side_list, *nodeset_ids, *ns_node_list;
  int num_nodes_in_set, num_sides_in_set, num_df_in_set;

  double *xvals, *yvals, *zvals, xyz[3];
  float version;

  int exo_nrf[3] = {4,5,6};
  int exo_nrfverts[3][6] =
    {{3,3,3,3,0,0},{4,4,4,3,3,0},{4,4,4,4,4,4}};
  int exo_rfverts[3][6][4] =
    {{{0,1,3,-1},{1,2,3,-1},{2,0,3,-1},{2,1,0,-1},{-1,-1,-1,-1},{-1,-1,-1,-1}},
     {{0,1,4,3},{1,2,5,4},{2,0,3,5},{2,1,0,-1},{3,4,5,-1},{-1,-1,-1,-1}},
     {{0,1,5,4},{1,2,6,5},{2,3,7,6},{3,0,4,7},{3,2,1,0},{4,5,6,7}}};

  List_ptr fedges, rfaces;
  MVertex_ptr mv, *fverts, *rverts;
  MEdge_ptr me;
  MFace_ptr mf;
  MRegion_ptr mr;
  MAttrib_ptr nmapatt, elblockatt, nodesetatt, sidesetatt;
  MSet_ptr faceset, nodeset, sideset, matset;
  
  ex_init_params exopar;
  
  
  exoid = ex_open(filename, EX_READ, &comp_ws, &io_ws, &version);
  
  if (exoid < 0) {
    sprintf(mesg,"Cannot open/read Exodus II file %s\n",filename);
    MSTK_Report(funcname,mesg,MSTK_FATAL);
  }
  
  
  status = ex_get_init_ext(exoid, &exopar);
  if (status < 0) {
    sprintf(mesg,"Error while reading Exodus II file %s\n",filename);
    MSTK_Report(funcname,mesg,MSTK_FATAL);
  }
  
  
  strcpy(title,exopar.title);
  ndim = exopar.num_dim;
  nnodes = exopar.num_nodes;
  nedges = exopar.num_edge;
  nedge_blk = exopar.num_edge_blk;
  nfaces = exopar.num_face;
  nface_blk = exopar.num_face_blk;
  nelems = exopar.num_elem;
  nelblock = exopar.num_elem_blk;
  nnodesets = exopar.num_node_sets;
  nedgesets = exopar.num_edge_sets;
  nfacesets = exopar.num_face_sets;
  nelemsets = exopar.num_elem_sets;
  nsidesets = exopar.num_side_sets;
  nnodemaps = exopar.num_node_maps;
  nedgemaps = exopar.num_edge_maps;
  nfacemaps = exopar.num_face_maps;
  nelemmaps = exopar.num_elem_maps;
  
  
  
  
  
  /* read the vertex information */
  
  xvals = (double *) malloc(nnodes*sizeof(double));
  yvals = (double *) malloc(nnodes*sizeof(double));  
  if (ndim == 2)
    zvals = (double *) calloc(nnodes,sizeof(double));
  else
    zvals = (double *) malloc(nnodes*sizeof(double));
  
  status = ex_get_coord(exoid, xvals, yvals, zvals);
  if (status < 0) {
    sprintf(mesg,"Error while reading vertex info in Exodus II file %s\n",filename);
    MSTK_Report(funcname,mesg,MSTK_FATAL);
  }


  for (i = 0; i < nnodes; i++) {
    
    mv = MV_New(mesh);
    
    xyz[0] = xvals[i]; xyz[1] = yvals[i]; xyz[2] = zvals[i];
    MV_Set_Coords(mv,xyz);
  }

  free(xvals);
  free(yvals);
  free(zvals);
  

  /* read node number map - store it as an attribute to spit out later
     if necessary */
  
  node_map = (int *) malloc(nnodes*sizeof(int));
  
  status = ex_get_node_num_map(exoid, node_map);
  if (status < 0) {
    sprintf(mesg,"Error while reading node map in Exodus II file %s\n",filename);
    MSTK_Report(funcname,mesg,MSTK_FATAL);
  }
  
  if (status == 0) {
    
    nmapatt = MAttrib_New(mesh, "node_map", INT, MVERTEX);
    
    for (i = 0; i < nnodes; i++) {
      mv = MESH_Vertex(mesh, i);
      MEnt_Set_AttVal(mv, nmapatt, node_map[i], 0.0, NULL);
    }
    
  }
  

  /* Read node sets */

  if (nnodesets) {
    nodeset_ids = (int *) malloc(nnodesets*sizeof(int));

    status = ex_get_node_set_ids(exoid,nodeset_ids);
    if (status < 0) {
      sprintf(mesg,
	      "Error while reading nodeset IDs in Exodus II file %s\n",
	      filename);
      MSTK_Report(funcname,mesg,MSTK_FATAL);
    }
    
    
    for (i = 0; i < nnodesets; i++) {

      sprintf(nodesetname,"nodeset_%-d",nodeset_ids[i]);

      nodesetatt = MAttrib_New(mesh,nodesetname,INT,MVERTEX);

      nodeset = MSet_New(mesh,nodesetname,MVERTEX);
    
      status = ex_get_node_set_param(exoid,nodeset_ids[i],&num_nodes_in_set,
				     &num_df_in_set);
      
      ns_node_list = (int *) malloc(num_nodes_in_set*sizeof(int));
      
      status = ex_get_node_set(exoid,nodeset_ids[i],ns_node_list);
      if (status < 0) {
	sprintf(mesg,
		"Error while reading nodes in nodeset %-d in Exodus II file %s\n",nodeset_ids[i],filename);
	MSTK_Report(funcname,mesg,MSTK_FATAL);
      }
      
      
      for (j = 0; j < num_nodes_in_set; j++) {
	mv = MESH_VertexFromID(mesh,ns_node_list[j]);
	
	/* Set attribute value for this node */
	
	MEnt_Set_AttVal(mv,nodesetatt,nodeset_ids[i],0.0,NULL);

	/* Add node to a nodeset */

	MSet_Add(nodeset,mv);
      }
      
      free(ns_node_list);
    }
    
    free(nodeset_ids);
      
  }



  /* Read face blocks if they are there - these will be used by
     polyhedral elements - There should only be one per EXODUS II
     specification and the following coding */

  if (nface_blk) {
    int nfblock, ntotnodes, *face_blk_ids;    

    face_blk_ids = (int *) MSTK_malloc(nface_blk*sizeof(int));

    status = ex_get_ids(exoid, EX_FACE_BLOCK, face_blk_ids);
    if (status < 0) {
      sprintf(mesg,"Error while reading element block ids in Exodus II file %s\n",filename);
      MSTK_Report(funcname,mesg,MSTK_FATAL);
  }
    for (i = 0; i < nface_blk; i++) {

      status = ex_get_block(exoid, EX_FACE_BLOCK, face_blk_ids[i], "nsided",
			    &nfblock, &ntotnodes, NULL, NULL, NULL);

      if (status < 0) {
	sprintf(mesg,
		"Error while reading face block info in Exodus II file %s\n",
		filename);
	MSTK_Report(funcname,mesg,MSTK_FATAL);
      }

      connect = (int *) MSTK_malloc(ntotnodes*sizeof(int));

      status = ex_get_conn(exoid, EX_FACE_BLOCK, face_blk_ids[i], connect,
			   NULL, NULL);

      if (status < 0) {
	sprintf(mesg,
		"Error while reading face block info in Exodus II file %s\n",
		filename);
	MSTK_Report(funcname,mesg,MSTK_FATAL);
      }

      
      nnpe = (int *) MSTK_malloc(nfblock*sizeof(int));

      status = ex_get_entity_count_per_polyhedra(exoid, EX_FACE_BLOCK,
						 face_blk_ids[i], nnpe);
      if (status < 0) {
	sprintf(mesg,
		"Error while reading face block info in Exodus II file %s\n",
		filename);
	MSTK_Report(funcname,mesg,MSTK_FATAL);
      }

      int max = 0;
      for (j = 0; j < nfblock; j++) 
	if (nnpe[j] > max) max = nnpe[j];


      fverts = (MVertex_ptr *) MSTK_malloc(max*sizeof(MVertex_ptr));

      faceset = MSet_New(mesh,"faceset",MFACE);

      int offset = 0;
      for (j = 0; j < nfblock; j++) {
	mf = MF_New(mesh);

	for (k = 0; k < nnpe[j]; k++) 
	  fverts[k] = MESH_VertexFromID(mesh,connect[offset+k]);

	MF_Set_Vertices(mf,nnpe[j],fverts);
	MF_Set_GEntDim(mf,3);  /* For now assume all are interior faces */
	                       /* Build classification will fix that    */
	offset += nnpe[j];

	MSet_Add(faceset,mf);
      }

      MSTK_free(fverts);
      MSTK_free(connect);
      MSTK_free(nnpe);
    }

  }



  
  /* Get element block IDs and names */
  
  
  elem_blk_ids = (int *) malloc(nelblock*sizeof(int));
  status = ex_get_ids(exoid, EX_ELEM_BLOCK, elem_blk_ids);
  if (status < 0) {
    sprintf(mesg,"Error while reading element block ids in Exodus II file %s\n",filename);
    MSTK_Report(funcname,mesg,MSTK_FATAL);
  }
  
  elem_blknames = (char **) malloc(nelblock*sizeof(char *));
  for (i = 0; i < nelblock; i++)
    elem_blknames[i] = (char *) malloc(256*sizeof(char));
  status = ex_get_names(exoid, EX_ELEM_BLOCK, elem_blknames);
  if (status < 0) {
    sprintf(mesg,"Error while reading element block ids in Exodus II file %s\n",filename);
    MSTK_Report(funcname,mesg,MSTK_FATAL);
  }
  
  

  if (ndim == 1) {
    
    MSTK_Report("MESH_ImportFromExodusII","Not implemented",MSTK_FATAL);
    
  }
  else if (ndim == 2) {
    
    elblockatt = MAttrib_New(mesh, "elemblock", INT, MFACE);
    
    
    /* Read each element block */
    
    for (i = 0; i < nelblock; i++) {
      
      status = ex_get_block(exoid, EX_ELEM_BLOCK, elem_blk_ids[i], 
			    elem_type, &nelem_i, &nelnodes, 
			    &neledges, &nelfaces, &natts);
      if (status < 0) {
	sprintf(mesg,
		"Error while reading element block %s in Exodus II file %s\n",
		elem_blknames[i],filename);
	MSTK_Report(funcname,mesg,MSTK_FATAL);
      }

      sprintf(matsetname,"matset_%-d",elem_blk_ids[i]);
      matset = MSet_New(mesh,matsetname,MFACE);
      
      if (strcmp(elem_type,"NSIDED") == 0 || 
	  strcmp(elem_type,"nsided") == 0) {

	/* In this case the nelnodes parameter is actually total number of 
	   nodes referenced by all the polygons (counting duplicates) */

	ntotnodes = nelnodes;
	
	connect = (int *) MSTK_malloc(ntotnodes*sizeof(int));

	status = ex_get_conn(exoid, EX_ELEM_BLOCK, elem_blk_ids[i], connect,
			   NULL, NULL);

	if (status < 0) {
	  sprintf(mesg,
		  "Error while reading face block info in Exodus II file %s\n",
		  filename);
	  MSTK_Report(funcname,mesg,MSTK_FATAL);
	}

      
	nnpe = (int *) MSTK_malloc(nelem_i*sizeof(int));

	status = ex_get_entity_count_per_polyhedra(exoid, EX_ELEM_BLOCK,
						   elem_blk_ids[i], nnpe);
	if (status < 0) {
	  sprintf(mesg,
		  "Error while reading face block info in Exodus II file %s\n",
		  filename);
	  MSTK_Report(funcname,mesg,MSTK_FATAL);
	}

	int max = 0;
	for (j = 0; j < nelem_i; j++) 
	  if (nnpe[j] > max) max = nnpe[j];

	fverts = (MVertex_ptr *) MSTK_malloc(max*sizeof(MVertex_ptr));

	int offset = 0;
	for (j = 0; j < nelem_i; j++) {
	  mf = MF_New(mesh);

	  for (k = 0; k < nnpe[j]; k++) 
	    fverts[k] = MESH_VertexFromID(mesh,connect[offset+k]);
	  
	  MF_Set_Vertices(mf,nnpe[j],fverts);

	  MF_Set_GEntID(mf, elem_blk_ids[i]);
	  MF_Set_GEntDim(mf, 2);

	  MSet_Add(matset,mf);
	  MEnt_Set_AttVal(mf,elblockatt,elem_blk_ids[i],0.0,NULL);

	  offset += nnpe[j];
	}
	
	MSTK_free(fverts);
	MSTK_free(connect);
	MSTK_free(nnpe);

      }
      else {
	
	/* Get the connectivity of all elements in this block */

	connect = (int *) calloc(nelnodes*nelem_i,sizeof(int));
	
	status = ex_get_elem_conn(exoid, elem_blk_ids[i], connect);
	if (status < 0) {
	  sprintf(mesg,"Error while reading element block %s in Exodus II file %s\n",elem_blknames[i],filename);
	  MSTK_Report(funcname,mesg,MSTK_FATAL);
	}
	

	/* Create the MSTK faces */
	
	fverts = (MVertex_ptr *) calloc(nelnodes,sizeof(MVertex_ptr));
	
	for (j = 0; j < nelem_i; j++) {
	  
	  mf = MF_New(mesh);
	  
	  for (k = 0; k < nelnodes; k++)
	    fverts[k] = MESH_VertexFromID(mesh,connect[nelnodes*j+k]);
	  
	  MF_Set_Vertices(mf,nelnodes,fverts);

	  MF_Set_GEntID(mf, elem_blk_ids[i]);
	  MF_Set_GEntDim(mf, 2);

	  MSet_Add(matset,mf);
	  MEnt_Set_AttVal(mf,elblockatt,elem_blk_ids[i],0.0,NULL);

	}
	
	free(fverts);

	
      } /* if (strcmp(elem_type,"NSIDED") ... else */
      
    } /* for (i = 0; i < nelblock; i++) */
    

    /* Read side sets */

    if (nsidesets) {

      sideset_ids = (int *) malloc(nsidesets*sizeof(int));
      
      status = ex_get_side_set_ids(exoid,sideset_ids);
      if (status < 0) {
	sprintf(mesg,
		"Error while reading sideset IDs %s in Exodus II file %s\n",
		elem_blknames[i],filename);
	MSTK_Report(funcname,mesg,MSTK_FATAL);
      }
      
      
      for (i = 0; i < nsidesets; i++) {

	sprintf(sidesetname,"sideset_%-d",sideset_ids[i]);
	
	sidesetatt = MAttrib_New(mesh,sidesetname,INT,MEDGE);
	sideset = MSet_New(mesh,sidesetname,MEDGE);
      
	status = ex_get_side_set_param(exoid,sideset_ids[i],&num_sides_in_set,
				       &num_df_in_set);
	
	ss_elem_list = (int *) malloc(num_sides_in_set*sizeof(int));
	ss_side_list = (int *) malloc(num_sides_in_set*sizeof(int));
	
	status = ex_get_side_set(exoid,sideset_ids[i],ss_elem_list,ss_side_list);
	if (status < 0) {
	  sprintf(mesg,
		  "Error while reading elements in sideset %-d in Exodus II file %s\n",sideset_ids[i],filename);
	  MSTK_Report(funcname,mesg,MSTK_FATAL);
	}
	
	
	for (j = 0; j < num_sides_in_set; j++) {
	  mf = MESH_FaceFromID(mesh,ss_elem_list[j]);
	  
	  fedges = MF_Edges(mf,1,0);
	  me = List_Entry(fedges,ss_side_list[j]-1);
	  List_Delete(fedges);
	  
	  /* Set attribute value for this edge */
	  
	  MEnt_Set_AttVal(me,sidesetatt,sideset_ids[i],0.0,NULL);

	  /* Add the edge to a set */

	  MSet_Add(sideset,me);
	  
	  /* Interpret sideset attribute as classification info for the edge */
	  
	  ME_Set_GEntDim(me,1);
	  ME_Set_GEntID(me,sideset_ids[i]);
	}
	
	free(ss_elem_list);
	free(ss_side_list);
	
      }

    }
    
    
    /* read element number map - store it as an attribute to spit out
       later if necessary */
    
    elem_map = (int *) malloc(nelems*sizeof(int));
    
    status = ex_get_elem_num_map(exoid, elem_map);
    if (status < 0) {
      sprintf(mesg,"Error while reading element map in Exodus II file %s\n",filename);
      MSTK_Report(funcname,mesg,MSTK_FATAL);
    }
    
    if (status == 0) {
      
      nmapatt = MAttrib_New(mesh, "elem_map", INT, MFACE);
      
      for (i = 0; i < nelems; i++) {
	mf = MESH_Face(mesh, i);
	MEnt_Set_AttVal(mf, nmapatt, elem_map[i], 0.0, NULL);
      }
      
    }
    
  }
  else if (ndim == 3) {


    /* Check if this is a strictly solid mesh, strictly surface mesh
       or mixed */

    int surf_elems=0, solid_elems=0;
    int mesh_type=0;  /* 1 - Surface, 2 - Solid, 3 - mixed */

    for (i = 0; i < nelblock; i++) {
      
      status = ex_get_block(exoid, EX_ELEM_BLOCK, elem_blk_ids[i], 
			    elem_type, &nelem_i, &nelnodes, 
			    &neledges, &nelfaces, &natts);
      if (status < 0) {
	sprintf(mesg,
		"Error while reading element block %s in Exodus II file %s\n",
		elem_blknames[i],filename);
	MSTK_Report(funcname,mesg,MSTK_FATAL);
      }

      if (strncasecmp(elem_type,"NFACED",6) == 0 ||
	  strncasecmp(elem_type,"TETRA",5) == 0 ||
	  strncasecmp(elem_type,"WEDGE",5) == 0 ||
	  strncasecmp(elem_type,"HEX",3) == 0)
	solid_elems = 1;
      else if (strncasecmp(elem_type,"NSIDED",6) == 0 ||
	       strncasecmp(elem_type,"TRIANGLE",8) == 0 ||
	       strncasecmp(elem_type,"QUAD",4) == 0 ||
	       strncasecmp(elem_type,"SHELL",5) == 0)
	surf_elems = 1;      
    }

    if (surf_elems && !solid_elems)
      mesh_type = 1;
    else if (!surf_elems && solid_elems)
      mesh_type = 2;
    else if (surf_elems && solid_elems)
      mesh_type = 3;
    else {
      MSTK_Report(funcname,"Mesh has neither surface nor solid elements?",MSTK_FATAL);
    }
    
    if (mesh_type == 1) 
      elblockatt = MAttrib_New(mesh, "elemblock", INT, MFACE);
    else if (mesh_type == 2)
      elblockatt = MAttrib_New(mesh, "elemblock", INT, MREGION);
    else if (mesh_type == 3)
      elblockatt = MAttrib_New(mesh, "elemblock", INT, MALLTYPE);
    

    for (i = 0; i < nelblock; i++) {
      
      status = ex_get_block(exoid, EX_ELEM_BLOCK, elem_blk_ids[i], 
			    elem_type, &nelem_i, &ntotnodes, 
			    &ntotedges, &ntotfaces, &natts);
      if (status < 0) {
	sprintf(mesg,
		"Error while reading element block %s in Exodus II file %s\n",
		elem_blknames[i],filename);
	MSTK_Report(funcname,mesg,MSTK_FATAL);
      }
      
      sprintf(matsetname,"matset_%-d",elem_blk_ids[i]);
      if (mesh_type == 1)
	matset = MSet_New(mesh,matsetname,MFACE);
      else if (mesh_type == 2)
	matset = MSet_New(mesh,matsetname,MREGION);
      else if (mesh_type == 3)
	matset = MSet_New(mesh,matsetname,MALLTYPE);

      
      if (strcmp(elem_type,"NFACED") == 0 || 
	  strcmp(elem_type,"nfaced") == 0) {
	
	/* In this case the nelnodes parameter is actually total number of 
	   nodes referenced by all the polyhedra (counting duplicates) */

	ntotfaces = nelfaces;
	
	connect = (int *) MSTK_malloc(ntotfaces*sizeof(int));

	status = ex_get_conn(exoid, EX_ELEM_BLOCK, elem_blk_ids[i], NULL, NULL,
			     connect);

	if (status < 0) {
	  sprintf(mesg,
		  "Error while reading elem block info in Exodus II file %s\n",
		  filename);
	  MSTK_Report(funcname,mesg,MSTK_FATAL);
	}

      
	nnpe = (int *) MSTK_malloc(nelem_i*sizeof(int));
	
	status = ex_get_entity_count_per_polyhedra(exoid, EX_FACE_BLOCK,
						   elem_blk_ids[i], nnpe);
	if (status < 0) {
	  sprintf(mesg,
		  "Error while reading elem block info in Exodus II file %s\n",
		  filename);
	  MSTK_Report(funcname,mesg,MSTK_FATAL);
	}

	int max = 0;
	for (j = 0; j < nelem_i; j++) 
	  if (nnpe[j] > max) max = nnpe[j];

	MFace_ptr *rfarr = (MFace_ptr *) MSTK_malloc(max*sizeof(MFace_ptr));
	int *rfdirs = (int *) MSTK_malloc(max*sizeof(int *));

	int offset = 0;
	for (j = 0; j < nelem_i; j++) {
	  mr = MR_New(mesh);

	  for (k = 0; k < nnpe[j]; k++) {
	    rfarr[k] = MSet_Entry(faceset,connect[offset+k]-1);
	    rfdirs[k] = 1;   /* THIS IS WRONG - EXODUS HAS TO GIVE US THIS INFO */
	  }
	  
	  MR_Set_Faces(mr, nnpe[j], rfarr, rfdirs);

	  MR_Set_GEntID(mr, elem_blk_ids[i]);	  
	  MR_Set_GEntDim(mr, 3);

	  MEnt_Set_AttVal(mr,elblockatt,elem_blk_ids[i],0.0,NULL);
	  MSet_Add(matset,mr);

	  offset += nnpe[j];
	}
	
	MSTK_free(rfarr);
	MSTK_free(connect);
	MSTK_free(nnpe);

      }
      else if (strncasecmp(elem_type,"TETRA",5) == 0 ||
	       strncasecmp(elem_type,"WEDGE",5) == 0 ||
	       strncasecmp(elem_type,"HEX",3) == 0) {
	int nrf, eltype;
	MFace_ptr face;
	
	/* Get the connectivity of all elements in this block */
	
	connect = (int *) calloc(nelnodes*nelem_i,sizeof(int));
	
	status = ex_get_elem_conn(exoid, elem_blk_ids[i], connect);
	if (status < 0) {
	  sprintf(mesg,"Error while reading element block %s in Exodus II file %s\n",elem_blknames[i],filename);
	  MSTK_Report(funcname,mesg,MSTK_FATAL);
	}
	
	
	/* Create the MSTK regions */
	
	rverts = (MVertex_ptr *) calloc(nelnodes,sizeof(MVertex_ptr));
	
	if (strncasecmp(elem_type,"TETRA",5) == 0) {
	  eltype = 0;
	  nrf = 4;
	  if (nelnodes > 4) {
	    MSTK_Report(funcname,"Higher order tets not supported",MSTK_WARN);
	    continue;
	  }
	}
	else if (strncasecmp(elem_type,"WEDGE",5) == 0) {
	  eltype = 1;
	  nrf = 5;
	  if (nelnodes > 6) {
	    MSTK_Report(funcname,"Higher order prisms/wedges not supported",MSTK_WARN);
	    continue;
	  }
	}
	else if (strncasecmp(elem_type,"HEX",3) == 0) {
	  eltype = 2;
	  nrf = 6;
	  if (nelnodes > 8) {
	    MSTK_Report(funcname,"Higher order hexes not supported",MSTK_WARN);
	    continue;
	  }
	}
	else {
	  sprintf(mesg,"Unrecognized or unsupported solid element type: %s",
		  elem_type);
	  MSTK_Report(funcname,mesg,MSTK_WARN);
	  continue;
	}
	
	fverts = (MVertex_ptr *) MSTK_malloc(6*sizeof(MVertex_ptr));

	MFace_ptr *rfarr = (MFace_ptr *) MSTK_malloc(nrf*sizeof(MFace_ptr));
	int *rfdirs = (int *) MSTK_malloc(nrf*sizeof(int *));

	for (j = 0; j < nelem_i; j++) {
	  
	  mr = MR_New(mesh);
	  
	  /* Exodus II and MSTK node conventions are the same but the
	     face conventions are not - so instead of using
	     MR_Set_Vertices we will create the faces individually and
	     then do MR_Set_Faces */
	  
	  for (k = 0; k < nelnodes; k++)
	    rverts[k] = MESH_VertexFromID(mesh,connect[nelnodes*j+k]);
	  

	  for (k = 0; k < exo_nrf[eltype]; k++) {
	    int nfv;

	    nfv = exo_nrfverts[eltype][k];
	    for (k1 = 0; k1 < nfv; k1++)
	      fverts[k1] = rverts[exo_rfverts[eltype][k][k1]];

	    if ((face = MVs_CommonFace(nfv,fverts))) {
	      List_ptr fregs;

	      rfarr[k] = face;
	      fregs = MF_Regions(face);
	      if (!fregs || !List_Num_Entries(fregs)) {
		rfdirs[k] = 1;
	      }
	      else {
		if (List_Num_Entries(fregs) == 1) {
		  int dir;
		  MRegion_ptr freg;

		  freg = List_Entry(fregs,0);
		  dir = MR_FaceDir(freg,face);
		  rfdirs[k] = !dir;
		}
		else if (List_Num_Entries(fregs) == 2) {
		  MSTK_Report("MESH_ImportFromExodusII",
			      "Face already connected two faces",MSTK_FATAL);
		}
	      }
	    }
	    else {
	      face = MF_New(mesh);
	      MF_Set_Vertices(face,exo_nrfverts[eltype][k],fverts);
	      rfarr[k] = face;
	      rfdirs[k] = 1;
	    }
	  }
	  
	  MR_Set_Faces(mr,nrf,rfarr,rfdirs);

	  MR_Set_GEntID(mr, elem_blk_ids[i]);
	  MR_Set_GEntDim(mr, 3);

	  MEnt_Set_AttVal(mr,elblockatt,elem_blk_ids[i],0.0,NULL);
	  MSet_Add(matset,mr);
	}
	
	free(rverts);

	free(connect);
	
      } 
      else if (strncasecmp(elem_type,"NSIDED",6) == 0) {
	
	/* In this case the nelnodes parameter is actually total number of 
	   nodes referenced by all the polygons (counting duplicates) */

	ntotnodes = nelnodes;
	
	connect = (int *) MSTK_malloc(ntotnodes*sizeof(int));

	status = ex_get_conn(exoid, EX_FACE_BLOCK, elem_blk_ids[i], connect,
			   NULL, NULL);

	if (status < 0) {
	  sprintf(mesg,
		  "Error while reading face block info in Exodus II file %s\n",
		  filename);
	  MSTK_Report(funcname,mesg,MSTK_FATAL);
	}

      
	nnpe = (int *) MSTK_malloc(nelem_i*sizeof(int));

	status = ex_get_entity_count_per_polyhedra(exoid, EX_FACE_BLOCK,
						   elem_blk_ids[i], nnpe);
	if (status < 0) {
	  sprintf(mesg,
		  "Error while reading face block info in Exodus II file %s\n",
		  filename);
	  MSTK_Report(funcname,mesg,MSTK_FATAL);
	}

	int max = 0;
	for (j = 0; j < nelem_i; j++) 
	  if (nnpe[j] > max) max = nnpe[j];

	fverts = (MVertex_ptr *) MSTK_malloc(max*sizeof(MVertex_ptr));

	int offset = 0;
	for (j = 0; j < nelem_i; j++) {
	  mf = MF_New(mesh);

	  for (k = 0; k < nnpe[j]; k++) 
	    fverts[k] = MESH_VertexFromID(mesh,connect[offset+k]);
	  
	  MF_Set_Vertices(mf,nnpe[j],fverts);

	  MF_Set_GEntID(mf, elem_blk_ids[i]);
	  MF_Set_GEntID(mf, 2);

	  if (mesh_type == 1 && mesh_type == 3) {
	    MEnt_Set_AttVal(mf,elblockatt,elem_blk_ids[i],0.0,NULL);
	    MSet_Add(matset,mf);
	  }

	  offset += nnpe[j];
	}
	
	MSTK_free(fverts);
	MSTK_free(connect);
	MSTK_free(nnpe);

      }
      else if (strncasecmp(elem_type,"TRIANGLE",8) == 0 ||
	       strncasecmp(elem_type,"QUAD",4) == 0 ||
	       strncasecmp(elem_type,"SHELL3",6) == 0 ||
	       strncasecmp(elem_type,"SHELL4",6) == 0) {
	
	/* Get the connectivity of all elements in this block */
	
	connect = (int *) calloc(nelnodes*nelem_i,sizeof(int));
	
	status = ex_get_elem_conn(exoid, elem_blk_ids[i], connect);
	if (status < 0) {
	  sprintf(mesg,"Error while reading element block %s in Exodus II file %s\n",elem_blknames[i],filename);
	  MSTK_Report(funcname,mesg,MSTK_FATAL);
	}
	
	
	/* Create the MSTK faces */
	
	fverts = (MVertex_ptr *) calloc(nelnodes,sizeof(MVertex_ptr));
	
	for (j = 0; j < nelem_i; j++) {
	  
	  mf = MF_New(mesh);
	  
	  for (k = 0; k < nelnodes; k++)
	    fverts[k] = MESH_VertexFromID(mesh,connect[nelnodes*j+k]);
	  
	  MF_Set_Vertices(mf,nelnodes,fverts);

	  MF_Set_GEntID(mf, elem_blk_ids[i]);
	  MF_Set_GEntID(mf, 2);

	  if (mesh_type == 1 && mesh_type == 3) {
	    MEnt_Set_AttVal(mf,elblockatt,elem_blk_ids[i],0.0,NULL);
	    MSet_Add(matset,mf);
	  }

	}
	
	free(fverts);

	
      } /* if (strcmp(elem_type,"NFACED") ... else */
      
      
    } /* for (i = 0; i < nelblock; i++) */
    
    
    
    /* Read side sets */

    if (nsidesets) {

      sideset_ids = (int *) malloc(nsidesets*sizeof(int));

      status = ex_get_side_set_ids(exoid,sideset_ids);
      if (status < 0) {
	sprintf(mesg,
		"Error while reading sideset IDs %s in Exodus II file %s\n",
		elem_blknames[i],filename);
	MSTK_Report(funcname,mesg,MSTK_FATAL);
      }
      
      
      for (i = 0; i < nsidesets; i++) {

	sprintf(sidesetname,"sideset_%-d",sideset_ids[i]);
	
	sidesetatt = MAttrib_New(mesh,sidesetname,INT,MFACE);

	sideset = MSet_New(mesh,sidesetname,MFACE);
      
	status = ex_get_side_set_param(exoid,sideset_ids[i],&num_sides_in_set,
				       &num_df_in_set);
	
	ss_elem_list = (int *) malloc(num_sides_in_set*sizeof(int));
	ss_side_list = (int *) malloc(num_sides_in_set*sizeof(int));

	status = ex_get_side_set(exoid,sideset_ids[i],ss_elem_list,ss_side_list);
	if (status < 0) {
	  sprintf(mesg,
		  "Error while reading elements in sideset %-d in Exodus II file %s\n",sideset_ids[i],filename);
	  MSTK_Report(funcname,mesg,MSTK_FATAL);
	}
	
	
	for (j = 0; j < num_sides_in_set; j++) {
	  int eltype;
	  List_ptr rverts;

	  mr = MESH_RegionFromID(mesh,ss_elem_list[j]);
	  
	  rfaces = MR_Faces(mr);       /* No particular order */
	  rverts = MR_Vertices(mr); /* Ordered for standard elements */

	  if (List_Num_Entries(rverts) == 4)
	    eltype = 0;
	  else if (List_Num_Entries(rverts) == 6)
	    eltype = 1;
	  else if (List_Num_Entries(rverts) == 8)
	    eltype = 2;
	  else
	    eltype = 3; /* Polyhedron */

	  if (eltype == 3) { 
	    mf = List_Entry(rfaces,ss_side_list[j]-1);
	  }
	  else {
	    int lfnum;
	    lfnum = ss_side_list[j]-1;

	    /* Since we made the faces of the region according the
	       convention that Exodus II uses, the local face number
	       of the Exodus II element matches that of the MSTK element */

	    mf = List_Entry(rfaces,lfnum);
	  }
	  if (!mf)
	    MSTK_Report(funcname,"Could not find sideset face",MSTK_FATAL);
	  else {
	    List_ptr fregs;
	    fregs = MF_Regions(mf);
	    if (List_Num_Entries(fregs) == 2) {
	      MSTK_Report("MESH_ImportFromExodusII",
			  "Interior face identified as being on the boundary",
			  MSTK_ERROR);
	    }
	    if (fregs) List_Delete(fregs);
	  }
	  List_Delete(rfaces);
	  
	  /* Set attribute value for this face */
	  
	  MEnt_Set_AttVal(mf,sidesetatt,sideset_ids[i],0.0,NULL);

	  /* Add the face to a set */

	  MSet_Add(sideset,mf);
	  
	  /* Interpret sideset attribute as classification info for the edge */
	  
	  MF_Set_GEntDim(mf,2);
	  MF_Set_GEntID(mf,sideset_ids[i]);
	}
	
	free(ss_elem_list);
	free(ss_side_list);
	
      }

    }


    
    
    /* read element number map - store it as an attribute to spit out
       later if necessary */
    
    elem_map = (int *) malloc(nelems*sizeof(int));
    
    status = ex_get_elem_num_map(exoid, elem_map);
    if (status < 0) {
      sprintf(mesg,"Error while reading element map in Exodus II file %s\n",filename);
      MSTK_Report(funcname,mesg,MSTK_FATAL);
    }
    
    if (status == 0) {
      
      if (mesh_type == 1) {
	nmapatt = MAttrib_New(mesh, "elem_map", INT, MFACE);
	
	for (i = 0; i < nelems; i++) {
	  mf = MESH_Face(mesh, i);
	  MEnt_Set_AttVal(mf, nmapatt, elem_map[i], 0.0, NULL);
	}
      }
      else if (mesh_type == 2) {
	nmapatt = MAttrib_New(mesh, "elem_map", INT, MREGION);
	
	for (i = 0; i < nelems; i++) {
	  mr = MESH_Region(mesh, i);
	  MEnt_Set_AttVal(mr, nmapatt, elem_map[i], 0.0, NULL);
	}
      }
      else {
	MSTK_Report("MESH_ImportFromExodusII",
	       "Element maps not supported for mixed surface-solid meshes",
		    MSTK_WARN);
      }
      
    }

    free(elem_map);
  } /* ndim = 3 */

  free(elem_blk_ids);
  free(elem_blknames);


  ex_close(exoid);

  return 1;

}


#ifdef __cplusplus
}
#endif
