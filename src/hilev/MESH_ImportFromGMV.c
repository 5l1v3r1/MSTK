#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <MSTK.h>


#ifdef __cplusplus
extern "C" {
#endif


int MESH_ImportFromGMV(Mesh_ptr mesh, const char *filename) {
  MRegion_ptr mr;
  MFace_ptr rfaces[MAXPF3], mf, *vface=NULL;
  MEdge_ptr fedges[MAXPV3*2], me, *vedge=NULL;
  MVertex_ptr vtx[MAXPV3], mv;
  int **rfv_template=NULL;
  int i, j, k, k1, ic, nnodes, nvtx, nrf, nfe, vid, evid[2];
  int status, pnum, opp_pnum, matinfo=0, onwhat;
  int rfdirs[MAXPF3], fedirs[MAXPV2], *vfdir=NULL, *vedir=NULL, fid, opp_fid;
  int done, found, ncells, edgnum, fcnum, num_faces, ileft, iright;
  int **vface2d_data=NULL, **vface3d_data=NULL, vface2d=0, vface3d=0;
  int cells_present=0, faces_present=0, non_vface_cells=0;
  double (*xyzarr)[3], xyz[3];
  char temp_str[256], cell_str[256], data_type[256];

  int ftmpl[2][4] = {{0,1,2,-1},{0,1,2,3}};
  int rtmpl[5][8] = {{3,0,1,2,-1,-1,-1,-1},
		     {4,0,1,2,3,-1,-1,-1},
		     {3,4,5,0,1,2,-1,-1},
		     {-1,-1,-1,-1,-1,-1,-1,-1},  /* dummy 7 node element */
		     {4,5,6,7,0,1,2,3}};
  FILE *fp;

  /* OPEN FILE */

  if (!(fp = fopen(filename,"r"))) {
    MSTK_Report("MESH_InitFromGMV","Cannot open GMV file",ERROR);
    return 0;
  }



  /* READ HEADER */


  status = fscanf(fp,"%s %s",temp_str,data_type);
  if (status == EOF) {
    MSTK_Report("MESH_InitFromGMV",
                "Premature end of file before any mesh data is read",ERROR);
    return 0;
  }
    
  if (strncmp(temp_str,"gmvinput",8) != 0) {
    MSTK_Report("MESH_InitFromGMV","Not a GMV file",ERROR);
    return 0;
  }

  if (strcmp(data_type,"ascii") != 0) {
    MSTK_Report("MESH_InitFromGMV",
		"Non-ASCII GMV files not yet supoorted",ERROR);
    return 0;
  }
  


  status = fscanf(fp,"%s",temp_str);
  if (status == EOF) {
    MSTK_Report("MESH_ImportFromGMV",
		"Premature end of file while looking for node data",
		ERROR);
    return 0;
  }


  /* READ COMMENTS IF THEY EXIST */

  if (strcmp(temp_str,"comments") == 0) {
    do {
      status = fscanf(fp,"%s",temp_str);
      if (status == EOF) {
	MSTK_Report("MESH_ImportFromGMV",
		    "Premature end of file while reading comments",
		    ERROR);
	return 0;
      }
    }
    while (strcmp(temp_str,"endcomm") != 0);

    status = fscanf(fp,"%s",temp_str);
    if (status == EOF) {
      MSTK_Report("MESH_ImportFromGMV",
		  "Premature end of file while reading comments",
		  ERROR);
      return 0;
    }
  }



  /* READ NODES */

  if (strcmp(temp_str,"nodes") == 0) {
    status = fscanf(fp,"%d",&nnodes);
    if (status == EOF) {
      MSTK_Report("MESH_ImportFromGMV",
		  "Premature end of file while reading nodes",ERROR);
      return 0;
    }

    xyzarr = (double (*)[3]) malloc(nnodes*sizeof(double [3]));

    for (j = 0; j < 3; j++) 
      for (i = 0; i < nnodes; i++) {
	status = fscanf(fp,"%lf",&(xyzarr[i][j]));
	if (status == EOF) {
	  MSTK_Report("MESH_ImportFromGMV",
		      "Premature end of file while reading nodes",ERROR);
	  return 0;
	}
      }

    for (i = 0; i < nnodes; i++) {
      mv = MV_New(mesh);
      MV_Set_Coords(mv,xyzarr[i]);

      MV_Set_ID(mv,i+1);
    }

    free(xyzarr);
  }
  else if (strcmp(temp_str,"nodev") == 0) {
    status = fscanf(fp,"%d",&nnodes);
    if (status == EOF) {
      MSTK_Report("MESH_ImportFromGMV",
		  "Premature end of file while reading nodes",ERROR);
      return 0;
    }

    for (i = 0; i < nnodes; i++) {
      status = fscanf(fp,"%lf %lf %lf",&(xyz[0]),&(xyz[1]),&xyz[2]);
      if (status == EOF) {
	MSTK_Report("MESH_ImportFromGMV","Premature end of file",ERROR);
	return 0;
      }

      mv = MV_New(mesh);
      MV_Set_Coords(mv,xyz);

      MV_Set_ID(mv,i+1);
    }
  }
  else {
    MSTK_Report("MESH_ImportFromGMV","Expect node info first",ERROR);
    fclose(fp);
    return 0;
  }


  
  /* READ THE REST OF THE FILE */


  done = 0;
  while (!done) {
    status = fscanf(fp,"%s",temp_str);
    if (status == EOF)
      MSTK_Report("MESH_ImportFromGMV","End of file but 'endgmv' not found?",
		  WARN);


    if (strcmp(temp_str,"cells") == 0) {

      /* READ CELLS */
      
      if (faces_present)
	MSTK_Report("MESH_ImportFromGMV",
		    "Cannot mix 'cells' and 'faces' in the same GMV file",
		    WARN);

      cells_present = 1;

      status = fscanf(fp,"%d",&ncells);
      if (status == EOF) {
	MSTK_Report("MESH_ImportFromGMV",
		    "Premature end of file while reading cells",ERROR);
	return 0;
      }
     
      for (ic = 0; ic < ncells; ic++) {
	fscanf(fp,"%s",cell_str);
	
	if (strcmp(cell_str,"line") == 0) {  

	  /* LINE ELEMENTS */

	  if (vface2d || vface3d)
	    MSTK_Report("MESH_ImportFromGMV",
			"Cannot mix vface3d cells with other cells",WARN);


	  status = fscanf(fp,"%d %d %d",&nvtx,&evid[0],&evid[1]);
	  if (status == EOF) {
	    MSTK_Report("MESH_ImportFromGMV",
			"Premature end of file while reading 'line'",ERROR);
	    return 0;
	  }
 	  
	  for (j = 0; j < 2; j++) {
	    vtx[j] = MESH_VertexFromID(mesh,evid[j]);
	    if (!vtx[j]) {
	      MSTK_Report("MESH_ImportFromGMV",
			  "Cannot find vertex referenced by 'line'",ERROR);
	      return 0;
	    }
	  }
	  
	  me = ME_New(mesh);
	  for (j = 0; j < 2; j++)
	    ME_Set_Vertex(me,0,vtx[j]);

	  non_vface_cells = 1;
	}
	else if (strcmp(cell_str,"tri") == 0 || strcmp(cell_str,"quad") == 0) {

	  /* TRIANGLES, QUADRILATERALS */

	  if (vface2d || vface3d)
	    MSTK_Report("MESH_ImportFromGMV",
			"Must not mix vface3d cells with other cells",WARN);


	  status = fscanf(fp,"%d",&nvtx);
	  if (status == EOF) {
	    MSTK_Report("MESH_ImportFromGMV",
			"Premature end of file while reading cells",ERROR);
	    return 0;
	  }

	  if ((strcmp(cell_str,"tri") == 0 && nvtx != 3) ||
	      (strcmp(cell_str,"quad") == 0 && nvtx != 4)) {
	    MSTK_Report("MESH_ImportFromGMV",
			"Unexpected number of nodes specified for cell",
			ERROR);
	    return 0;
	  }

	  for (j = 0; j < nvtx; j++) {
	    status = fscanf(fp,"%d",&vid);
	    if (status == EOF) {
	      MSTK_Report("MESH_ImportFromGMV",
			  "Premature end of file while reading cells",ERROR);
	      return 0;
	    }

	    vtx[j] = MESH_VertexFromID(mesh,vid);
	    if (!vtx[j]) {
	      MSTK_Report("MESH_ImportFromGMV",
			  "Cannot find vertex referenced by cell",ERROR);
	      return 0;
	    }
	  }

	  mf = MF_New(mesh);

	  MF_Set_Vertices(mf,nvtx,vtx);

	  non_vface_cells = 1;
	}
	else if (strcmp(cell_str,"tet") == 0 || 
		 strcmp(cell_str,"pyramid") == 0 || 
		 strcmp(cell_str,"prism") == 0 ||
		 strcmp(cell_str,"hex") == 0) {

	  /* TETS, PYRAMIDS, TRIANGULAR PRISMS, HEXES */

	  if (vface2d || vface3d)
	    MSTK_Report("MESH_ImportFromGMV",
			"Must not mix vface3d cells with other cells",WARN);


	  status = fscanf(fp,"%d",&nvtx);
	  if (status == EOF) {
	    MSTK_Report("MESH_ImportFromGMV",
			"Premature end of file while reading cells",ERROR);
	    return 0;
	  }

	  if ((strcmp(cell_str,"tet") == 0 && nvtx != 4) ||
	      (strcmp(cell_str,"pyramid") == 0 && nvtx != 5) ||
	      (strcmp(cell_str,"prism") == 0 && nvtx != 6) ||
	      (strcmp(cell_str,"hex") == 0 && nvtx != 8)) {
	    MSTK_Report("MESH_ImportFromGMV",
			"Unexpected number of nodes specified for cell",
			ERROR);
	    return 0;
	  }
	  
	  for (j = 0; j < nvtx; j++) {
	    k = rtmpl[nvtx-4][j];

	    status = fscanf(fp,"%d",&vid);
	    if (status == EOF) {
	      MSTK_Report("MESH_ImportFromGMV",
			  "Premature end of file while reading cells",ERROR);
	      return 0;
	    }

	    vtx[k] = MESH_VertexFromID(mesh,vid);
	    if (!vtx[k]) {
	      MSTK_Report("MESH_ImportFromGMV",
			  "Cannot find vertex referenced by cell",ERROR);
	      return 0;
	    }
	  }

	  mr = MR_New(mesh);
	  MR_Set_Vertices(mr,nvtx,vtx,0,NULL);

	  non_vface_cells = 1;
	}
	else if (strcmp(cell_str,"general") == 0) {

	  if (!rfv_template) {
	    rfv_template = (int **) malloc(MAXPF3*sizeof(int *));
	    for (j = 0; j < MAXPF3; j++) 
	      rfv_template[j] = (int *) malloc(MAXPV2*sizeof(int));
	  }

	  /* GENERAL POLYHEDRA */

	  if (vface2d || vface3d)
	    MSTK_Report("MESH_ImportFromGMV",
			"Must not mix vface3d cells with other cells",WARN);

	  status = fscanf(fp,"%d",&nrf);
	  if (status == EOF) {
	    MSTK_Report("MESH_ImportFromGMV",
			"Premature end of file while reading cells",ERROR);
	    return 0;
	  }

	  for (j = 0; j < nrf; j++) {
	    fscanf(fp,"%d",&(rfv_template[j][0]));
	  }
	  
	  for (j = 0; j < MAXPV3; j++) vtx[j] = NULL;
	  
	  nvtx = 0;
	  for (j = 0; j < nrf; j++) {
	    for (k = 0; k < rfv_template[j][0]; k++) {
	      status = fscanf(fp,"%d",&vid);
	      if (status == EOF) {
		MSTK_Report("MESH_ImportFromGMV",
			    "Premature end of file while reading cells",ERROR);
		return 0;
	      }

	      found = 0;
	      for (k1= 0; k1 < nvtx && !found; k1++)
		if (MV_ID(vtx[k1]) == vid)
		  found = 1;

	      if (found) {
		rfv_template[j][k+1] = k1;
	      }
	      else {
		vtx[nvtx] = MESH_VertexFromID(mesh,vid);

		if (!vtx[nvtx]) {
		  MSTK_Report("MESH_ImportFromGMV",
			      "Cannot find vertex referenced by cell",ERROR);
		  return 0;
		}

		rfv_template[j][k+1] = nvtx;
		nvtx++;
	      }
	    }
	  }

	  if (nrf == 1) { /* A face specified in a non-conventional manner */
	    
	    mf = MF_New(mesh);
	    MF_Set_Vertices(mf,nvtx,vtx);
	  }
	  else {
	    mr = MR_New(mesh);
	    MR_Set_Vertices(mr,nvtx,vtx,nrf,rfv_template);	  
	  }

	  non_vface_cells = 1;
	}
	else if (strcmp(cell_str,"vface2d") == 0) {

	  /* FACES DESCRIBED IN TERMS OF EDGES */

	  if (non_vface_cells)
	    MSTK_Report("MESH_ImportFromGMV",
			"Must not mix vface2d cells with other cells",WARN);

	  if (!vface2d_data)
	    vface2d_data = (int **) malloc(ncells*sizeof(int));
	    
	  status = fscanf(fp,"%d",&nfe);
	  if (status == EOF) {
	    MSTK_Report("MESH_ImportFromGMV",
			"Premature end of file while reading cells",ERROR);
	    return 0;
	  }
	  
	  vface2d_data[ic] = (int *) malloc((nfe+1)*sizeof(int));
	  vface2d_data[ic][0] = nfe;

	  for (j = 0; j < nfe; j++) {
	    status = fscanf(fp,"%d",&edgnum);
	    if (status == EOF) {
	      MSTK_Report("MESH_ImportFromGMV",
			  "Premature end of file while reading cells",ERROR);
	      return 0;
	    }
	    
	    vface2d_data[ic][j+1] = edgnum;
	  }

	  vface2d = 1;
	}
	else if (strcmp(cell_str,"vface3d") == 0) {
	  
	  /* REGIONS DESCRIBED IN TERMS OF FACES */

	  if (non_vface_cells)
	    MSTK_Report("MESH_ImportFromGMV",
			"Must not mix vface3d cells with other cells",WARN);

	  if (!vface3d_data)
	    vface3d_data = (int **) malloc(ncells*sizeof(int *));
	  
	  status = fscanf(fp,"%d",&nrf);
	  if (status == EOF) {
	    MSTK_Report("MESH_ImportFromGMV",
			"Premature end of file while reading cells",ERROR);
	    return 0;
	  }

	  vface3d_data[ic] = (int *) malloc((nrf+1)*sizeof(int));
	  vface3d_data[ic][0] = nrf;

	  for (j = 0; j < nrf; j++) {
	    status = fscanf(fp,"%d",&fcnum);
	    if (status == EOF) {
	      MSTK_Report("MESH_ImportFromGMV",
			  "Premature end of file while reading cells",ERROR);
	      return 0;
	    }

	    vface3d_data[ic][j+1] = fcnum;
	  }

	  vface3d = 1;
	}
	else {
	  
	  /* We do not deal with higher order elements */

	  MSTK_Report("MESH_ImportFromGMV",
		      "Cell type not supported by GMV or MSTK",WARN);
	}
      }	

      if (rfv_template) {
	for (i = 0; i < MAXPF3; i++)
	  free(rfv_template[i]);
	free(rfv_template);
      }

      /* END READING CELLS (EXCEPT VFACES FOR VFACE2D and VFACE3D) */

    }
    else if (strcmp(temp_str,"vfaces") == 0) {

      /* EDGE/FACE DEFINITION FOR VFACE2D/VFACE3D */
      
      status = fscanf(fp,"%d",&num_faces);
      if (status == EOF) {
	MSTK_Report("MESH_ImportFromGMV",
		    "Premature end of file while reading vfaces",ERROR);
	return 0;
      }

      if (vface2d) {
	vedge = (MEdge_ptr *) malloc(num_faces*sizeof(MEdge_ptr));
	vedir = (int *) malloc(num_faces*sizeof(int));
      }
      else {
	vface = (MFace_ptr *) malloc(num_faces*sizeof(MFace_ptr));
	vfdir = (int *) malloc(num_faces*sizeof(int));
      }

      for (j = 0; j < num_faces; j++) {
	status = fscanf(fp,"%d %d %d %d %d",&nvtx,&pnum,&opp_fid,&opp_pnum,&fid);
	if (status == EOF) {
	  MSTK_Report("MESH_ImportFromGMV",
		      "Premature end of file while reading vfaces",ERROR);
	  return 0;
	}

	for (k = 0; k < nvtx; k++) {
	  status = fscanf(fp,"%d",&vid);
	  if (status == EOF) {
	    MSTK_Report("MESH_ImportFromGMV",
			"Premature end of file while reading vfaces",ERROR);
	    return 0;
	  }

	  vtx[k] = MESH_VertexFromID(mesh,vid);
	  if (!vtx[k]) {
	    MSTK_Report("MESH_ImportFromGMV",
			"Cannot find vertex referenced by vfaces",ERROR);
	    return 0;
	  }
	}

	if (vface2d) {
	  if (opp_fid < fid) {

	    /* Edge must have already been defined for adjacent face */

	    vedge[i] = MVs_CommonEdge(vtx[0],vtx[1]);

	    if (!vedge[j]) {
	      MSTK_Report("MESH_ImportFromGMV",
			  "Cannot find edge from adjacent face",ERROR);
	      return 0;
	    }

	    /* We won't set vedir[j] = 0, to leave open the possibility 
	       that someone will try to define non-manifold surface meshes */
	    vedir[j] = (ME_Vertex(vedge[j],0) == vtx[0]) ? 1 : 0;
	  }
	  else {
	    vedge[j] = ME_New(mesh);
	    ME_Set_Vertex(vedge[j],0,vtx[0]);
	    ME_Set_Vertex(vedge[j],1,vtx[1]);

	    vedir[j] = 1; 
	  }
	}
	else {
	  if (opp_fid < fid) {

	    /* Face must have already been defined for adjacent region */

	    vface[j] = MVs_CommonFace(nvtx,vtx);

	    if (!vface[j]) {
	      MSTK_Report("MESH_ImportFromGMV",
			  "Cannot find face from adjacent region",ERROR);
	      return 0;
	    }

	    /* Adjacent region was defined first. So it will first create the
	       face and use it in the positive sense. So, this region must use
	       it the opposite sense */

	    vfdir[j] = 0;
	  }
	  else {
	    vface[i] = MF_New(mesh);
	    MF_Set_Vertices(vface[j],nvtx,vtx);

	    vfdir[j] = 1;
	  }
	}
      }

      /* NOW THAT WE HAVE THE FACE INFO, LETS BUILD THE VFACE2D CELLS (faces) 
	 OR VFACE3D CELLS (POLYHEDRA) */

      if (vface2d) {
	for (ic = 0; ic < ncells; ic++) {
	  nfe = vface2d_data[ic][0];
	  
	  for (j = 0; j < nfe; j++) {
	    k = vface2d_data[ic][j+1];
	    fedges[j] = vedge[k];
	    fedirs[j] = vedir[k];
	  }

	  mf = MF_New(mesh);
	  MF_Set_Edges(mf,nfe,fedges,fedirs);

	  free(vface2d_data[ic]);
	}

	free(vface2d_data);
      }
      else if (vface3d) {
	for (ic = 0; ic < ncells; ic++) {
	  nrf = vface3d_data[ic][0];

	  for (j = 0; j < nrf; j++) {
	    k = vface3d_data[ic][j+1];
	    rfaces[j] = vface[k];
	    rfdirs[j] = vfdir[k];
	  }

	  mr = MR_New(mesh);
	  MR_Set_Faces(mr,nrf,rfaces,rfdirs);

	  free(vface3d_data[ic]);
	}

	free(vface3d_data);
      }
    }
    else if (strcmp(temp_str,"faces") == 0) {

      /* POLYGONAL FACES */

      if (cells_present) {
	MSTK_Report("MESH_ImportFromGMV",
		    "Cannot mix 'cells' and 'faces' in the same GMV file",
		    WARN);
	return 0;
      }

      faces_present = 1;

      /* we will use the vface3d data structures */

      status = fscanf(fp,"%d %d",&num_faces,&ncells);

      if (ncells) {
	vface3d_data = (int **) malloc(ncells*sizeof(int *));
	for (ic = 0; ic < ncells; ic++) {
	  vface3d_data[ic] = (int *) malloc((MAXPF3+1)*sizeof(int));
	  vface3d_data[ic][0] = 0;
	}
      }
      else
	vface3d_data = NULL;

      vface = (MFace_ptr *) malloc(num_faces*sizeof(MFace_ptr));

      for (i = 0; i < num_faces; i++) {	
	status = fscanf(fp,"%d",&nvtx);
	if (status == EOF) {
	  MSTK_Report("MESH_ImportFromGMV",
		      "Premature end of file while reading faces",ERROR);
	  return 0;
	}

	for (j = 0; j < nvtx; j++) {
	  status = fscanf(fp,"%d",&vid);
	  if (status == EOF) {
	    MSTK_Report("MESH_ImportFromGMV",
			"Premature end of file while reading faces",ERROR);
	    return 0;
	  }
	  
	  vtx[j] = MESH_VertexFromID(mesh,vid);
	  if (!vtx[j]) {
	    MSTK_Report("MESH_ImportFromGMV",
			"Cannot find vertex referenced by face",ERROR);
	    return 0;
	  }
	}

	vface[i] = mf = MF_New(mesh);
	MF_Set_Vertices(mf,nvtx,vtx);
	
	status = fscanf(fp,"%d %d",&ileft,&iright);
	if (status == EOF) {
	  MSTK_Report("MESH_ImportFromGMV",
		      "Premature end of file while reading faces",ERROR);
	  return 0;
	}

	if (ileft) {
	  nrf = vface3d_data[ileft-1][0];
	  vface3d_data[ileft-1][nrf+1] = i+1;
	  (vface3d_data[ileft-1][0])++;
	}

	if (iright) {
	  nrf = vface3d_data[iright-1][0];
	  vface3d_data[iright-1][nrf+1] = -(i+1);
	  (vface3d_data[iright-1][0])++;
	}
      }

      for (ic = 0; ic < ncells; ic++) {
	mr = MR_New(mesh);

	nrf = vface3d_data[ic][0];
	for (j = 0; j < nrf; j++) {
	  k = vface3d_data[ic][j+1];
	  rfdirs[j] = (k < 0) ? 0 : 1;
	  k = abs(k);
	  rfaces[j] = vface[k-1];
	}

	MR_Set_Faces(mr,nrf,rfaces,rfdirs);
      }
      
      free(vface);
      if (vface3d_data) {
	for (ic = 0; ic < ncells; ic++) 
	  free(vface3d_data[ic]);
	free(vface3d_data);
      }

    }
    else if (strcmp(temp_str,"material") == 0) {
      int nmats, matnum;
      char matname[10];

      matinfo = 1;

      status = fscanf(fp,"%d %d",&nmats,&onwhat);
      if (status == EOF) {
	MSTK_Report("MESH_ImportFromGMV",
		    "Premature end of file while reading material data",ERROR);
	return 0;
      }
      
      for (j = 0; j < nmats; j++) {
	status = fscanf(fp,"%s",matname);
	if (status == EOF) {
	  MSTK_Report("MESH_ImportFromGMV",
		      "Premature end of file while reading material data",
		      ERROR);
	  return 0;
	}
      }

      if (onwhat == 0) {
	if (MESH_Num_Regions(mesh)) {
	  for (j = 0; j < ncells; j++) {
	    status = fscanf(fp,"%d",&(matnum));
	    if (status == EOF) {
	      MSTK_Report("MESH_ImportFromGMV",
			  "Premature end of file while reading material data",
			  ERROR);
	      return 0;
	    }
	    
	    mr = MESH_Region(mesh,j);
	    MR_Set_GEntID(mr,matnum);
	  }
	}
	else {
	  for (j = 0; j < ncells; j++) {
	    status = fscanf(fp,"%d",&(matnum));
	    if (status == EOF) {
	      MSTK_Report("MESH_ImportFromGMV",
			  "Premature end of file while reading material data",
			  ERROR);
	      return 0;
	    }
	    
	    mf = MESH_Face(mesh,j);
	    MF_Set_GEntID(mf,matnum);
	    MF_Set_GEntDim(mf,2);
	  }
	}
      }
      else if (onwhat == 1) {
	MSTK_Report("MESH_ImportFromGMV",
		    "Must specify material IDs for elements, not nodes",ERROR);
	for (i = 0; i < nnodes; i++) {
	  status = fscanf(fp,"%d",&(matnum));
	  if (status == EOF) {
	    MSTK_Report("MESH_ImportFromGMV",
			"Premature end of file while reading material data",ERROR);
	    return 0;
	  }
	}
      }
      else {
	MSTK_Report("MESH_ImportFromGMV","Unrecognized entity for material IDs",ERROR);
      }
    }
    else if (strcmp(temp_str,"velocities") == 0) {
    }
    else if (strcmp(temp_str,"variable") == 0) {
      int varsdone, ival;
      MAttrib_ptr matt;
      double rval;
      

      varsdone = 0;
      while (!varsdone) {

	/* read variable name and on what type of entity it is */

	status = fscanf(fp,"%s",temp_str);
	if (status == EOF) {
	  MSTK_Report("MESH_ImportFromGMV",
		      "Premature end of file while reading variables",ERROR);
	  return 0;
	}

	if (strcmp(temp_str,"endvars") == 0)
	  varsdone = 1;
	else {
	  status = fscanf(fp,"%d",&onwhat);
	  if (status == EOF) {
	    MSTK_Report("MESH_ImportFromGMV",
			"Premature end of file while reading variables",ERROR);
	    return 0;
	  }

	  if (strncmp(temp_str,"itetclr",7) == 0) {
	    /* ITETCLR is LAGRIT's way of specifying classification
	       for the highest order entities in a mesh. It is
	       redundant if material properties are also specified */

	    if (onwhat == 1) { /* specified on nodes ?? Wrong ! */
	      MSTK_Report("MESH_ImportFromGMV",
			  "Cell color must be specified on cells, not nodes!!",
			  ERROR);
	      
	      matt = MAttrib_New(mesh,temp_str,INT,MVERTEX);

	      for (i = 0; i < nnodes; i++) {
		status = fscanf(fp,"%d",&ival);
		if (status == EOF) {
		  MSTK_Report("MESH_ImportFromGMV",
			      "Premature end of file while reading variable itetclr",
			      ERROR);
		  return 0;
		}

		mv = MESH_Vertex(mesh,i);
		MEnt_Set_AttVal(mv,matt,ival,0,NULL);
	      }
	    }
	    else {
	      if (MESH_Num_Regions(mesh)) {
		matt = MAttrib_New(mesh,temp_str,INT,MREGION);

		for (i = 0; i < ncells; i++) {
		  status = fscanf(fp,"%d",&ival);
		  if (status == EOF) {
		    MSTK_Report("MESH_ImportFromGMV",
				"Premature end of file while reading variable itetclr",
				ERROR);
		    return 0;
		  }

		  mr = MESH_Region(mesh,i);
		  MEnt_Set_AttVal(mr,matt,ival,0,NULL);

		  /* if material ids for elements is not given, then
		     assign them using this info */

		  if (!matinfo)
		    MR_Set_GEntID(mr,ival);
		}
	      }
	      else {
		matt = MAttrib_New(mesh,temp_str,INT,MFACE);

		for (i = 0; i < ncells; i++) {
		  status = fscanf(fp,"%d",&ival);
		  if (status == EOF) {
		    MSTK_Report("MESH_ImportFromGMV",
				"Premature end of file while reading variable itetclr",
				ERROR);
		    return 0;
		  }

		  mf = MESH_Face(mesh,i);
		  MEnt_Set_AttVal(mf,matt,ival,0,NULL);

		  /* if material ids for elements is not given, then
		     assign them using this info */

		  if (!matinfo) {
		    MF_Set_GEntID(mf,ival);
		    MF_Set_GEntDim(mf,2);
		  }
		}
	      }
	    }
	  }
	  else if (strncmp(temp_str,"icr",3) == 0) {
	    /* ICR is LAGRIT's way of specifying classification info
	       and it indicates the number of constraints on the
	       movement of a node. For some reason, LAGRIT chooses to
	       write these out as real numbers instead of integers */

	    if (onwhat == 0) { /* specified on cells ?? Wrong ! */
	      MSTK_Report("MESH_ImportFromGMV",
			  "Constraints must be specified on nodes!!",ERROR);
	      
	      if (MESH_Num_Regions(mesh)) {
		matt = MAttrib_New(mesh,temp_str,INT,MREGION);

		for (i = 0; i < ncells; i++) {
		  status = fscanf(fp,"%lf",&rval);
		  if (status == EOF) {
		    MSTK_Report("MESH_ImportFromGMV",
				"Premature end of file while reading variable itetclr",
				ERROR);
		    return 0;
		  }

		  mr = MESH_Region(mesh,i);
		  MEnt_Set_AttVal(mr,matt,(int) rval,0,NULL);
		}
	      }
	      else {
		matt = MAttrib_New(mesh,temp_str,INT,MFACE);

		for (i = 0; i < ncells; i++) {
		  status = fscanf(fp,"%lf",&rval);
		  if (status == EOF) {
		    MSTK_Report("MESH_ImportFromGMV",
				"Premature end of file while reading variable itetclr",
				ERROR);
		    return 0;
		  }

		  mf = MESH_Face(mesh,i);
		  MEnt_Set_AttVal(mf,matt,(int) rval,0,NULL);
		}
	      }
	    }
	    else {
	      matt = MAttrib_New(mesh,temp_str,INT,MVERTEX);

	      for (i = 0; i < nnodes; i++) {
		status = fscanf(fp,"%lf",&rval);
		if (status == EOF) {
		  MSTK_Report("MESH_ImportFromGMV",
			      "Premature end of file while reading variable itetclr",
			      ERROR);
		  return 0;
		}

		mv = MESH_Vertex(mesh,i);
		MEnt_Set_AttVal(mv,matt,(int) rval,0,NULL);

		MV_Set_GEntDim(mv,(int) 3-rval);
	      }
	    }
	  }
	  else { 
	    /* GENERAL ATTRIBUTE */

	    if (onwhat == 0) { 

	      if (MESH_Num_Regions(mesh)) {
		matt = MAttrib_New(mesh,temp_str,DOUBLE,MREGION);

		for (i = 0; i < ncells; i++) {
		  status = fscanf(fp,"%lf",&rval);
		  if (status == EOF) {
		    MSTK_Report("MESH_ImportFromGMV",
				"Premature end of file while reading variable itetclr",
				ERROR);
		    return 0;
		  }

		  mr = MESH_Region(mesh,i);
		  MEnt_Set_AttVal(mr,matt,0,rval,NULL);
		}
	      }
	      else {
		matt = MAttrib_New(mesh,temp_str,DOUBLE,MFACE);

		for (i = 0; i < ncells; i++) {
		  status = fscanf(fp,"%lf",&rval);
		  if (status == EOF) {
		    MSTK_Report("MESH_ImportFromGMV",
				"Premature end of file while reading variable itetclr",
				ERROR);
		    return 0;
		  }

		  mf = MESH_Face(mesh,i);
		  MEnt_Set_AttVal(mf,matt,0,rval,NULL);
		}
	      }
	    }
	    else {
	      matt = MAttrib_New(mesh,temp_str,DOUBLE,MVERTEX);

	      for (i = 0; i < nnodes; i++) {
		status = fscanf(fp,"%lf",&rval);
		if (status == EOF) {
		  MSTK_Report("MESH_ImportFromGMV",
			      "Premature end of file while reading variable itetclr",
			      ERROR);
		  return 0;
		}

		mv = MESH_Vertex(mesh,i);
		MEnt_Set_AttVal(mv,matt,0,rval,NULL);
	      }
	    }
	  }
	} 
      } /* while (!varsdone) */
    }
    else if (strcmp(temp_str,"endgmv") == 0) 
      done = 1;
    else if (strcmp(temp_str,"nodeids") == 0) {
    }
    else if (strcmp(temp_str,"cellids") == 0) {
    }
    else if (strcmp(temp_str,"faceids") == 0) {
    }
    else {
      MSTK_Report("MESH_ImportFromGMV","Unrecognized keyword",ERROR);
    }
  }

  fclose(fp);
  return 1;
}

#ifdef __cplusplus
}
#endif
