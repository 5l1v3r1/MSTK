#define _H_Mesh_Private

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "Mesh.h"
#include "MSTK.h"
#include "List.h"
#include "MSTK_malloc.h"

#include "MSTK_private.h"

#ifdef __cplusplus
extern "C" {
#endif

  RepType MESH_rtype[5] = {F1, F4, R1, R2, R4};
  char MESH_rtype_str[5][3] = {"F1\0","F4\0","R1\0","R2\0","R4\0"};

Mesh_ptr MESH_New(RepType type) {
  Mesh_ptr newmesh;

  newmesh = (Mesh_ptr) MSTK_malloc(sizeof(Mesh));

  newmesh->reptype = type;
  newmesh->nv = newmesh->ne = newmesh->nf = newmesh->nr = 0;
  newmesh->mvertex = (List_ptr) NULL;
  newmesh->medge = (List_ptr) NULL;
  newmesh->mface = (List_ptr) NULL;
  newmesh->mregion = (List_ptr) NULL;
  newmesh->geom = (GModel_ptr) NULL;

  newmesh->max_vid = newmesh->max_eid = newmesh->max_fid = newmesh->max_rid = 0;

  return newmesh;
}


void MESH_Delete(Mesh_ptr mesh) {
  int i, nv, ne, nf, nr;
  MVertex_ptr mv;
  MEdge_ptr me;
  MFace_ptr mf;
  MRegion_ptr mr;

  if (mesh->mregion) {
    nr = mesh->nr;
    i = 0;
    while ((mr = List_Next_Entry(mesh->mregion,&i))) {
      MR_Delete(mr);
    }
    List_Delete(mesh->mregion);
  }
  if (mesh->mface) {
    nf = mesh->nf;
    i = 0;
    while ((mf = List_Next_Entry(mesh->mface,&i))) {
      MF_Delete(mf);
    }
    List_Delete(mesh->mface);
  }
  if (mesh->medge) {
    ne = mesh->ne;
    i = 0;
    while ((me = List_Next_Entry(mesh->medge,&i))) {
      ME_Delete(me);
    }
    List_Delete(mesh->medge);
  }
  if (mesh->mvertex) {
    nv = mesh->nv;
    i = 0;
    while ((mv = List_Next_Entry(mesh->mvertex,&i))) {
      MV_Delete(mv);
    }
    List_Delete(mesh->mvertex);
  }
  free(mesh);
}

int MESH_SetRepType(Mesh_ptr mesh, RepType type) {
  mesh->reptype = type;
  return 1;
}

void MESH_SetGModel(Mesh_ptr mesh, GModel_ptr gm) {
  mesh->geom = gm;
}

GModel_ptr MESH_GModel(Mesh_ptr mesh) {
  return mesh->geom;
}

RepType    MESH_RepType(Mesh_ptr mesh) {
  return mesh->reptype;
}

int        MESH_Num_Vertices(Mesh_ptr mesh) {
  return mesh->nv;
}

int        MESH_Num_Edges(Mesh_ptr mesh) {
  return mesh->ne;
}

int        MESH_Num_Faces(Mesh_ptr mesh) {
  return mesh->nf;
}

int        MESH_Num_Regions(Mesh_ptr mesh) {
  return mesh->nr;
}

/*
int        MESH_Num_Elements(Mesh_ptr mesh) {
  return mesh->nel;
}
*/

MVertex_ptr  MESH_Vertex(Mesh_ptr mesh, int i) {
  if (i >= mesh->nv) {
#ifdef DEBUG
    MSTK_Report("Mesh_Vertex","Non-existent vertex requested\n",ERROR);
#endif
    return (MVertex_ptr) NULL;
  }
  else
    return (MVertex_ptr) List_Entry(mesh->mvertex, i);
}

MEdge_ptr MESH_Edge(Mesh_ptr mesh, int i) {
  if (i >= mesh->ne) {
#ifdef DEBUG
    MSTK_Report("Mesh_Edge","Non-existent edge requested\n",ERROR);
#endif
    return (MEdge_ptr) NULL;
  }
  else 
    return (MEdge_ptr) List_Entry(mesh->medge, i);
}

MFace_ptr MESH_Face(Mesh_ptr mesh, int i) {
  if (i >= mesh->nf) {
#ifdef DEBUG
    MSTK_Report("Mesh_Face","Non-existent face requested\n",ERROR);
#endif
    return (MFace_ptr) NULL;
  }
  else
    return (MFace_ptr) List_Entry(mesh->mface, i);
}

MRegion_ptr MESH_Region(Mesh_ptr mesh, int i) {

  if (i >= mesh->nr) {
#ifdef DEBUG
    MSTK_Report("Mesh_Region","Non-existent region requested\n",ERROR);
#endif
    return (MRegion_ptr) NULL;
  }
  else
    return (MRegion_ptr) List_Entry(mesh->mregion, i);
}

MVertex_ptr  MESH_Next_Vertex(Mesh_ptr mesh, int *index) {
  return (MVertex_ptr) List_Next_Entry(mesh->mvertex, index);
}

MEdge_ptr MESH_Next_Edge(Mesh_ptr mesh, int *index) {
  return (MEdge_ptr) List_Next_Entry(mesh->medge, index);
}

MFace_ptr MESH_Next_Face(Mesh_ptr mesh, int *index) {
  return (MFace_ptr) List_Next_Entry(mesh->mface, index);
}

MRegion_ptr MESH_Next_Region(Mesh_ptr mesh, int *index) {
  return (MRegion_ptr) List_Next_Entry(mesh->mregion, index);
}

void MESH_Add_Vertex(Mesh_ptr mesh, MVertex_ptr v) {
  if (mesh->mvertex == (List_ptr) NULL)
    mesh->mvertex = List_New(10);

  mesh->mvertex = List_Add(mesh->mvertex, (void *) v);
  mesh->nv = List_Num_Entries(mesh->mvertex);

  (mesh->max_vid)++;
  MV_Set_ID(v,mesh->max_vid);
}

void MESH_Add_Edge(Mesh_ptr mesh, MEdge_ptr e){
  /* Have to check if edges exist in this type of representation */
  if (mesh->reptype >= R1 && mesh->reptype <= MSTK_MAXREP)
    return;

  if (mesh->medge == (List_ptr) NULL)
    mesh->medge = List_New(10);

  mesh->medge = List_Add(mesh->medge, (void *) e);
  mesh->ne = List_Num_Entries(mesh->medge);

  (mesh->max_eid)++;
  ME_Set_ID(e,mesh->max_eid);
  
}    
     
void MESH_Add_Face(Mesh_ptr mesh, MFace_ptr f){
  /* Have to check if faces exist in this type of representation */
  if (mesh->reptype == R1 || mesh->reptype == R2)
    return;

  if ((mesh->reptype == R4) && (MF_Region(f,0) || MF_Region(f,1))) {
#ifdef DEBUG
    MSTK_Report("MESH_Add_Face","Can add disconnected faces only",ERROR);
#endif
    return;
  }

  if (mesh->mface == (List_ptr) NULL)
    mesh->mface = List_New(10);
  
  mesh->mface = List_Add(mesh->mface, (void *) f);
  mesh->nf = List_Num_Entries(mesh->mface);

  (mesh->max_fid)++;
  MF_Set_ID(f,mesh->max_fid);
}    
     
void MESH_Add_Region(Mesh_ptr mesh, MRegion_ptr r){
  if (mesh->mregion == (List_ptr) NULL)
    mesh->mregion = List_New(10);

  mesh->mregion = List_Add(mesh->mregion, (void *) r);
  mesh->nr = List_Num_Entries(mesh->mregion);

  (mesh->max_rid)++;
  MR_Set_ID(r,mesh->max_rid);
}    
     
void MESH_Rem_Vertex(Mesh_ptr mesh, MVertex_ptr v) {
  if (mesh->mvertex == (List_ptr) NULL) {
#ifdef DEBUG
    MSTK_Report("Mesh_Rem_Vertex","No vertices in mesh to remove", ERROR);
#endif
    return;
  }

  List_Rem(mesh->mvertex, (void *) v);
  mesh->nv = List_Num_Entries(mesh->mvertex);
}    
     
void MESH_Rem_Edge(Mesh_ptr mesh, MEdge_ptr e) {
  if (mesh->medge == (List_ptr) NULL) {
#ifdef DEBUG
    MSTK_Report("Mesh_Rem_Edge","No Edges in mesh to remove",ERROR);
#endif
    return;
  }

  List_Rem(mesh->medge, (void *) e);
  mesh->ne = List_Num_Entries(mesh->medge);
}    
     
void MESH_Rem_Face(Mesh_ptr mesh, MFace_ptr f){
  if (mesh->mface == (List_ptr) NULL) {
#ifdef DEBUG
    MSTK_Report("Mesh_Rem_Face","No Faces in mesh to remove",ERROR);
#endif
    return;
  }

  List_Rem(mesh->mface, (void *) f);
  mesh->nf = List_Num_Entries(mesh->mface);
}    
     
void MESH_Rem_Region(Mesh_ptr mesh, MRegion_ptr r){
  if (mesh->mregion == (List_ptr) NULL) {
    MSTK_Report("Mesh_Rem_Region","No regions in mesh to remove",ERROR);
    return;
  }

  List_Rem(mesh->mregion, (void *) r);
  mesh->nr = List_Num_Entries(mesh->mregion);
}    
     
void MESH_Set_GModel(Mesh_ptr mesh, GModel_ptr geom){
  mesh->geom = geom;
}    
     
void MESH_SetRepTypeIni(Mesh_ptr mesh, RepType reptype){
  mesh->reptype = reptype;
}    
     
int  MESH_ChangeRepType(Mesh_ptr mesh, RepType nureptype){
  /* Only certain types are allowed */
  return 1;
}

/* File name should have the .msk extension - will not check here */
int MESH_InitFromFile(Mesh_ptr mesh, const char *filename) {
  FILE *fp;
  char inp_rtype[16], temp_str[256], fltype_str[16], rltype_str[16];
  int i, j, found, NV, NE, NF, NR, nav, nar, gdim, gid;
  int vid1, vid2, eid, fid, rid, adjvid, adjrid, adjv_flag;
  int nfv, max_nfv, nfe, max_nfe, nrv, max_nrv, nrf, max_nrf;
  int *fedirs, *rfdirs;
  double ver, xyz[3];
  MVertex_ptr mv, ev1, ev2, adjv, *fverts, *rverts;
  MEdge_ptr me, *fedges;
  MFace_ptr mf, *rfaces;
  MRegion_ptr mr, adjr;


  if (!(fp = fopen(filename,"r"))) {
    MSTK_Report("MESH_InitFromFile","Cannot open file",ERROR);
    return 0;
  }

  fscanf(fp,"%s %lf",temp_str,&ver);
  if (strcmp(temp_str,"MSTK") != 0) {
    MSTK_Report("MESH_InitFromFile","Not a MSTK file",ERROR);
    fclose(fp);
    return 0;
  }

  if (ver != MSTK_VER) {
    MSTK_Report("MESH_InitFromFile","Version mismatch",WARN);
  }

  fscanf(fp,"%s %d %d %d %d\n",inp_rtype,&(mesh->nv),&(mesh->ne),&(mesh->nf),&(mesh->nr));

  found = 0;
  for (i = 0; i < MSTK_MAXREP; i++) {
    if (strncmp(inp_rtype,MESH_rtype_str[i],2) == 0) {
      mesh->reptype = MESH_rtype[i];
      found = 1;
      break;
    }
  }

  if (!found) {
    MSTK_Report("MESH_InitFromFile","Unrecognized representation type",ERROR);
    fclose(fp);
    return 0;
  }


  /* For now, the reduced representations only allow region and
     face elements - no edge elements are allowed */

  if (mesh->reptype >= R1 && mesh->reptype <= R4) {
    if (mesh->ne)
      MSTK_Report("Mesh_InitFromFile","Representation does not allow edges",WARN);
  }


  fscanf(fp,"%s",temp_str);
  if (strncasecmp(temp_str,"vertices",8) == 0) {

    NV = mesh->nv;
    mesh->mvertex = List_New(NV);

    for (i = 0; i < NV; i++) {
      fscanf(fp,"%lf %lf %lf %d %d",&xyz[0],&xyz[1],&xyz[2],&gdim,&gid);
      
      mv = MV_New(mesh);
      MV_Set_Coords(mv,xyz);
      
      MV_Set_GEntID(mv,gid);
      MV_Set_GEntDim(mv,gdim);
      
      MV_Set_ID(mv,(i+1));
    }
  }
  else {
    MSTK_Report("MESH_InitFromFile","Vertex information should be listed first",ERROR);
    fclose(fp);
    return 0;
  }


  fscanf(fp,"%s",temp_str);
  if (strncasecmp(temp_str,"adjvertices",11) == 0) {
    adjv_flag = 1;
    if (mesh->reptype == R2 || mesh->reptype == R4) {
      for (i = 0; i < NV; i++) {
	fscanf(fp,"%d",&nav);
	for (j = 0; j < nav; j++) {
	  fscanf(fp,"%d",&adjvid);
	  adjv = List_Entry(mesh->mvertex,adjvid-1);
#ifdef DEBUG
	  if (MV_ID(adjv) != adjvid)
	    MSTK_Report("MESH_InitFromFile","Adjacent vertex ID mismatch",ERROR);
#endif
	  
	  MV_Add_AdjVertex(mv,adjv);
	}
      }
    }
    else {
      for (i = 0; i < NV; i++) {
	fscanf(fp,"%d",&nav);
	for (j = 0; j < nav; j++)
	  fscanf(fp,"%d",&adjvid);
      }
    }
  }
  else {
    adjv_flag = 0;
    if (mesh->reptype == R2 || mesh->reptype == R4) {
      MSTK_Report("MESH_InitFromFile","Expected adjacent vertex information",ERROR);
    }
  }
    
  if (adjv_flag)
    fscanf(fp,"%s",temp_str);
  if (strncasecmp(temp_str,"edges",5) == 0) {

    NE = mesh->ne;
    mesh->medge = List_New(NE);

    if (mesh->reptype >= F1 || mesh->reptype <= F4) {
      for (i = 0; i < NE; i++) {
	fscanf(fp,"%d %d %d %d",&vid1,&vid2,&gdim,&gid);
	ev1 = List_Entry(mesh->mvertex,vid1-1);
	ev2 = List_Entry(mesh->mvertex,vid2-1);
#ifdef DEBUG
	if (MV_ID(ev1) != vid1 || MV_ID(ev2) != vid2)
	  MSTK_Report("MESH_InitFromFile","Mesh vertex ID mismatch",ERROR);
#endif

	me = ME_New(mesh);

	ME_Set_Vertex(me,0,ev1);
	ME_Set_Vertex(me,1,ev2);

	ME_Set_GEntID(me,gid);
	ME_Set_GEntDim(me,gdim);

	ME_Set_ID(me, i+1);
      }
    }
    else {
      for (i = 0; i < NE; i++)
	fscanf(fp,"%d %d %d %d",&vid1,&vid2,&gdim,&gid);
    }
  }
  else {
    if (mesh->reptype >= F1 || mesh->reptype <= F4) {
      MSTK_Report("MESH_InitFromFile","Expected edge information",ERROR);
      return 0;
    }
  }
  
  fscanf(fp,"%s",temp_str);
  if (strncasecmp(temp_str,"face",4) == 0) {
    if (strncasecmp(temp_str,"faces",5) != 0) 
      MSTK_Report("MESH_InitFromFile","Expected keyword \"faces\"",ERROR);
    
    NF = mesh->nf;
    mesh->mface = List_New(NF);

    fscanf(fp,"%s",fltype_str);

    if (strncasecmp(fltype_str,"vertex",6) == 0) {      
      if (mesh->reptype >= R1 && mesh->reptype <= R4) {

	fverts = NULL;
	for (i = 0; i < NF; i++) {
	  mf = MF_New(mesh);

	  fscanf(fp,"%d",&nfv);
	  if (fverts) {
	    if (nfv > max_nfv) {
	      max_nfv = nfv;
	      fverts = MSTK_realloc(fverts,max_nfv*sizeof(MVertex_ptr));
	    }
	  }
	  else {
	    max_nfv = nfv;
	    fverts = MSTK_malloc(nfv*sizeof(MVertex_ptr));
	  }
	  
	  for (j = 0; j < nfv; j++) {
	    fscanf(fp,"%d",&vid1);
	    fverts[j] = List_Entry(mesh->mvertex,vid1-1);
#ifdef DEBUG
	    if (MV_ID(fverts[j]) != vid1)
	      MSTK_Report("MESH_InitFromFile","Mesh vertex ID mismatch",ERROR);
#endif
	  }
	  MF_Set_Vertices(mf,nfv,fverts);  

	  fscanf(fp,"%d %d",&gdim,&gid);
	  
	  MF_Set_GEntID(mf,gid);
	  MF_Set_GEntDim(mf,gdim);
	  
	  MF_Set_ID(mf,i+1);
	}
	if (fverts)
	  MSTK_free(fverts);
      }
      else {
	MSTK_Report("MESH_InitFromFile","Expected face description in terms of vertices",ERROR);
        fclose(fp);
	return 0;
      }
    }
    else if (strncasecmp(fltype_str,"edge",4) == 0) {
      if (mesh->reptype >= F1 && mesh->reptype <= F4) { 

	fedges = NULL;	fedirs = NULL;
	for (i = 0; i < NF; i++) {
	  mf = MF_New(mesh);

	  fscanf(fp,"%d",&nfe);
	  if (fedges) {
	    if (nfe > max_nfe) {
	      max_nfe = nfe;
	      fedges = MSTK_realloc(fedges,max_nfe*sizeof(MVertex_ptr));
	      fedirs = MSTK_realloc(fedirs,max_nfe*sizeof(int));
	    }
	  }
	  else {
	    max_nfe = nfe;
	    fedges = MSTK_malloc(max_nfe*sizeof(MVertex_ptr));
	    fedirs = MSTK_malloc(max_nfe*sizeof(MVertex_ptr));
	  }
	  
	  for (j = 0; j < nfe; j++) {
	    fscanf(fp,"%d",&eid);
	    fedirs[j] = eid > 0 ? 1 : 0;
	    fedges[j] = List_Entry(mesh->medge,abs(eid)-1);
#ifdef DEBUG
	    if (ME_ID(fedges[j]) != abs(eid))
	      MSTK_Report("MESH_InitFromFile","Mesh edge ID mismatch",ERROR);
#endif
	  }
	  
	  MF_Set_Edges(mf,nfe,fedges,fedirs);
	  
	  fscanf(fp,"%d %d",&gdim,&gid);
	  
	  MF_Set_GEntDim(mf,gdim);
	  MF_Set_GEntID(mf,gid);
	  
	  MF_Set_ID(mf,i+1);
	}
	if (fedges) {
	  MSTK_free(fedges);
	  MSTK_free(fedirs);
	}
      }
      else {
	MSTK_Report("MESH_InitFromFile","Expect face description in terms of vertices",ERROR);
	return 0;
      }
    }
  }
  else {
    if (mesh->reptype >= F1 || mesh->reptype <= F4) {
      MSTK_Report("MESH_InitFromFile","Expected face information",ERROR);
      fclose(fp);
      return 0;
    }
  }
  
  
  fscanf(fp,"%s",temp_str);
  if (strncasecmp(temp_str,"region",6) == 0) {
    if (strncasecmp(temp_str,"regions",7) != 0) 
      MSTK_Report("MESH_InitFromFile","Expected keyword \"regions\"",ERROR);

    NR = mesh->nr;
    mesh->mregion = List_New(NR);

    fscanf(fp,"%s",rltype_str);

    if (strncasecmp(rltype_str,"vertex",6) == 0) {
      if (mesh->reptype == R1 || mesh->reptype == R2) {
	rverts = NULL;
	for (i = 0; i < NR; i++) {
	  mr = MR_New(mesh);
	  
	  fscanf(fp,"%d",&nrv);
	  if (rverts) {
	    if (nrv > max_nrv) {
	      max_nrv = nrv;
	      rverts = MSTK_realloc(rverts,max_nrv*sizeof(MVertex_ptr));
	    }
	  }
	  else {
	    max_nrv = nrv;
	    rverts = MSTK_malloc(nrv*sizeof(MVertex_ptr));
	  }
	  
	  for (j = 0; j < nrv; j++) {
	    fscanf(fp,"%d",&vid1);
	    rverts[j] = List_Entry(mesh->mvertex,vid1-1);
#ifdef DEBUG
	    if (MV_ID(rverts[j]) != vid1)
	      MSTK_Report("MESH_InitFromFile","Mesh vertex ID mismatch",ERROR);
#endif
	  }
	  MR_Set_Vertices(mr,nrv,rverts);  
	  
	  fscanf(fp,"%d %d",&gdim,&gid);
	  
	  MR_Set_GEntDim(mr,gdim);

	  MR_Set_GEntID(mr,gid);
	  
	  MR_Set_ID(mr,i+1);
	}
	if (rverts)
	  MSTK_free(rverts);
      }
      else {
	MSTK_Report("MESH_InitFromFile","Expected region description in terms of faces",ERROR);
        fclose(fp);
	return 0;
      }
    }
    else if (strncasecmp(rltype_str,"face",4) == 0) {
      if (mesh->reptype != R1 && mesh->reptype != R2) {
	rfaces = NULL;
	rfdirs = NULL;
	for (i = 0; i < NR; i++) {
	  mr = MR_New(mesh);
	  
	  fscanf(fp,"%d",&nrf);
	  if (rfaces) {
	    if (nrf > max_nrf) {
	      max_nrf = nrf;
	      rfaces = MSTK_realloc(rfaces,max_nrf*sizeof(MFace_ptr));
	      rfdirs = MSTK_realloc(rfdirs,max_nrf*sizeof(int));
	    }
	  }
	  else {
	    max_nrf = nrf;
	    rfaces = MSTK_malloc(nrf*sizeof(MFace_ptr));
	    rfdirs = MSTK_malloc(nrf*sizeof(int));
	  }
	
	  for (j = 0; j < nrf; j++) {
	    fscanf(fp,"%d",&fid);
	    rfdirs[j] = fid > 0 ? 1 : 0;
	    rfaces[j] = List_Entry(mesh->mface,abs(fid)-1);
#ifdef DEBUG
	    if (MF_ID(rfaces[j]) != abs(fid))
	      MSTK_Report("MESH_InitFromFile","Mesh face ID mismatch",ERROR);
#endif
	  }
	  
	  MR_Set_Faces(mr,nrf,rfaces,rfdirs);
	  
	  fscanf(fp,"%d %d",&gdim,&gid);
	  
	  MR_Set_GEntDim(mr,gdim);

	  MR_Set_GEntID(mr,gid);
	  
	  MR_Set_ID(mr,i+1);
	}
	if (rfaces) {
	  MSTK_free(rfaces);
	  MSTK_free(rfdirs);
	}
      }
    }
    else {
      MSTK_Report("MESH_InitFromFile","Expected region description in terms of vertices",ERROR);
      fclose(fp);
      return 0;
    }
  }

  fscanf(fp,"%s",temp_str);
  if (strncasecmp(temp_str,"adjregions",10) == 0) {
    if (mesh->reptype == R2) {
      for (i = 0; i < NR; i++) {
	mr = List_Entry(mesh->mregion,i);

	fscanf(fp,"%d",&nar);
	for (j = 0; j < nar; j++) {
	  fscanf(fp,"%d",&adjrid);
	  if (adjrid == 0)
	    continue;
	  adjr = List_Entry(mesh->mregion,adjrid-1);
#ifdef DEBUG
	  if (MR_ID(adjr) != rid)
	    MSTK_Report("MESH_InitFromFile","Adjacent region ID mismatch",ERROR);
#endif

	  MR_Add_AdjRegion(mr,j,adjr);
	}
      }
    }
    else {
      for (i = 0; i < NR; i++) {
	fscanf(fp,"%d",&nar);
	for (j = 0; j < nar; j++)
	  fscanf(fp,"%d",&rid);
      }
    }
  }

  fclose(fp);

  return 1;
}

void MESH_WriteToFile(Mesh_ptr mesh, const char *filename) {
  FILE *fp;
  char mesg[80];
  int i, j;
  int gdim, gid;
  int mvid, mvid0, mvid1, mvid2, mrid2, meid, mfid;
  int nav, nar, nfe, nfv, nrf, nrv, dir;
  double xyz[3];
  MVertex_ptr mv, mv0, mv1, mv2;
  MEdge_ptr me;
  MFace_ptr mf;
  MRegion_ptr mr, mr2;
  GEntity_ptr gent;
  List_ptr adjverts, mfedges, mfverts, mrfaces, mrverts, adjregs;
  

  if (!(fp = fopen(filename,"w"))) {
    sprintf(mesg,"Cannot open file %-s for writing",filename);
    MSTK_Report("MESH_WriteToFile",mesg,ERROR);
    return;
  }

  fprintf(fp,"MSTK %-2.1lf\n",MSTK_VER);
  fprintf(fp,"%s %d %d %d %d\n",MESH_rtype_str[mesh->reptype],mesh->nv,mesh->ne,mesh->nf,mesh->nr);


  for (i = 0; i < mesh->nv; i++) {
    mv = MESH_Vertex(mesh,i);
    MV_Set_ID(mv,i+1);
  }

  for (i = 0; i < mesh->ne; i++) {
    me = MESH_Edge(mesh,i);
    ME_Set_ID(me,i+1);
  }

  for (i = 0; i < mesh->nf; i++) {
    mf = MESH_Face(mesh,i);
    MF_Set_ID(mf,i+1);
  }

  for (i = 0; i < mesh->nr; i++) {
    mr = MESH_Region(mesh,i);
    MR_Set_ID(mr,i+1);
  }
  
  
  fprintf(fp,"vertices\n");
  for (i = 0; i < mesh->nv; i++) {
    mv = MESH_Vertex(mesh,i);

    MV_Coords(mv,xyz);

    gdim = MV_GEntDim(mv);
    gent = MV_GEntity(mv);
    gid = gent ? -99 : 0;

    fprintf(fp,"%30.16lf %30.16lf %30.16lf \t%d %d\n",xyz[0],xyz[1],xyz[2],gdim,gid);
    
#ifdef DEBUG
    if (gent)
      MSTK_Report("MESH_WriteToFile","GEnt_ID not implemented",WARN);
#endif
  }

  if (mesh->reptype == R2 || mesh->reptype == R4) {
    fprintf(fp,"adjvertices\n");

    for (i = 0; i < mesh->nv; i++) {
      mv = MESH_Vertex(mesh,i);

      nav = MV_Num_AdjVertices(mv);
      fprintf(fp,"%d ",nav);
      
      adjverts = MV_AdjVertices(mv);
      for (j = 0; j < nav; j++) {
	mv2 = List_Entry(adjverts,j);
	mvid2 = MV_ID(mv2);
	fprintf(fp,"%d ",mvid2);
      }
      fprintf(fp,"\n");
      List_Delete(adjverts);
    }
  }



  if (mesh->reptype <= F4 && mesh->ne) {
    fprintf(fp,"edges\n");

    for (i = 0; i < mesh->ne; i++) {
      me = MESH_Edge(mesh,i);

      mv0 = ME_Vertex(me,0);
      mvid0 = MV_ID(mv0);
      mv1 = ME_Vertex(me,1);
      mvid1 = MV_ID(mv1);

      gdim = ME_GEntDim(me);
      gent = ME_GEntity(me);
      gid = gent ? -99 : 0;

      fprintf(fp,"%d %d \t%d %d\n",mvid0,mvid1,gdim,gid);
    }
  }



  if (((mesh->reptype <= F4) || (mesh->reptype >= R2)) && mesh->nf) {
    if (mesh->reptype <= F4) {
      fprintf(fp,"faces edge\n");

      for (i = 0; i < mesh->nf; i++) {
	mf = MESH_Face(mesh,i);
	
	nfe = MF_Num_Edges(mf);
	fprintf(fp,"%d ",nfe);
	
	mfedges = MF_Edges(mf,1,0);
	for (j = 0; j < nfe; j++) {
	  me = List_Entry(mfedges,j);
	  dir = MF_EdgeDir_i(mf,j);
	  meid = dir ? ME_ID(me) : -ME_ID(me);
	  fprintf(fp,"%d ",meid);
	}
	List_Delete(mfedges);

	gdim = MF_GEntDim(mf);
	gent = MF_GEntity(mf);
	gid = gent ? -99 : 0;
    
	fprintf(fp,"\t%d %d\n",gdim,gid);
      }
    }
    else {
      fprintf(fp,"faces vertex\n");

      for (i = 0; i < mesh->nf; i++) {
	mf = MESH_Face(mesh,i);
	
	nfv = MF_Num_Edges(mf);
	fprintf(fp,"%d ",nfv);
	
	mfverts = MF_Vertices(mf,1);
	for (j = 0; j < nfv; j++) {
	  mv = List_Entry(mfverts,j);
	  mvid = MV_ID(mv);
	  fprintf(fp,"%d ",mvid);
	}
	List_Delete(mfverts);

	gdim = MF_GEntDim(mf);
	gent = MF_GEntity(mf);
	gid = gent ? -99 : 0;
	
	fprintf(fp,"\t%d %d\n",gdim,gid);
      }
    }
	
  }


  if (mesh->nr) {
    if (mesh->reptype <= F4 || mesh->reptype >= R2) {
      fprintf(fp,"regions face\n");

      for (i = 0; i < mesh->nr; i++) {
	mr = MESH_Region(mesh,i);

	nrf = MR_Num_Faces(mr);
	fprintf(fp,"%d ",nrf);

	mrfaces = MR_Faces(mr);
	for (j = 0; j < nrf; j++) {
	  mf = List_Entry(mrfaces,j);
	  dir = MR_FaceDir_i(mr,j);
	  mfid = dir ? MF_ID(mf) : -MF_ID(mf);
	  fprintf(fp,"%d ",mfid);
	}
	List_Delete(mrfaces);
	
	gdim = MF_GEntDim(mr);
	gent = MF_GEntity(mr);
	gid = gent ? -99 : 0;

	fprintf(fp,"\t%d %d\n",gdim,gid);
      }
    }
    else {
      fprintf(fp,"regions vertex\n");

      for (i = 0; i < mesh->nr; i++) {
	mr = MESH_Region(mesh,i);

	nrv = MR_Num_Vertices(mr);
	fprintf(fp,"%d ",nrv);

	mrverts = MR_Vertices(mr);
	for (j = 0; j < nrv; j++) {
	  mv = List_Entry(mrverts,j);
	  mvid = MV_ID(mf);
	  fprintf(fp,"%d ",mvid);
	}
	List_Delete(mrverts);
	
	gdim = MF_GEntDim(mr);
	gent = MF_GEntity(mr);
	gid = gent ? -99 : 0;

	fprintf(fp,"\t%d %d\n",gdim,gid);
      }
    }

    if (mesh->reptype == R2 || mesh->reptype == R4) {
      fprintf(fp,"adjregions\n");
      
      for (i = 0; i < mesh->nr; i++) {
	mr = MESH_Region(mesh,i);

	nar = MR_Num_Faces(mr);
	fprintf(fp,"%d ",nar);

	adjregs = MR_AdjRegions(mr);

	for (j = 0; j < nar; j++) {
	  mr2 = List_Entry(adjregs,j);
	  if ((int) mr2 == -1) 
	    fprintf(fp,"%d ",0);
	  else {
	    mrid2 = MR_ID(mr2);
	    fprintf(fp,"%d ",mrid2);
	  }
	}
	fprintf(fp,"\n");
	List_Delete(adjregs);
      }
    }
  }

  fclose(fp);
}



#ifdef __cplusplus
}
#endif
