#include <UnitTest++.h>

#include "../../include/MSTK.h"

// Test if we can correctly read a polygonal exodus mesh 
// Then test if we can correctly write it and read it back

TEST(Read_Write_ExodusII_Poly2) 
{
  int idx, ok, nfe;
  Mesh_ptr mesh, mesh2;
  MFace_ptr mf;
  MVertex_ptr mv;
  int one_tri, one_quad, one_penta;

  MSTK_Init();

  mesh = MESH_New(UNKNOWN_REP);
  ok = MESH_ImportFromFile(mesh,"serial/poly2.exo",NULL,NULL,NULL);
  CHECK_EQUAL(ok,1);

  CHECK(MESH_Num_Vertices(mesh) > 0);
  
  idx = 0; one_tri = 0; one_quad = 0; one_penta = 0;
  while ((mf = MESH_Next_Face(mesh,&idx))) {
    int ne = MF_Num_Edges(mf);
    if (ne == 3)
      one_tri++;
    else if (ne == 4)
      one_quad++;
    else if (ne == 5)
      one_penta++;
  }

  CHECK_EQUAL(one_tri,1);
  CHECK_EQUAL(one_quad,1);
  CHECK_EQUAL(one_penta,1);

  ok = MESH_ExportToFile(mesh,"./poly2-tmp.exo",NULL,0,NULL,NULL,NULL);

  CHECK_EQUAL(ok,1);

  mesh2 = MESH_New(UNKNOWN_REP);
  ok = MESH_ImportFromFile(mesh2,"./poly2-tmp.exo",NULL,NULL,NULL);

  idx = 0; one_tri = 0; one_quad = 0; one_penta = 0;
  while ((mf = MESH_Next_Face(mesh2,&idx))) {
    int ne = MF_Num_Edges(mf);
    if (ne == 3)
      one_tri++;
    else if (ne == 4)
      one_quad++;
    else if (ne == 5)
      one_penta++;
  }

  CHECK_EQUAL(one_tri,1);
  CHECK_EQUAL(one_quad,1);
  CHECK_EQUAL(one_penta,1);
}


TEST(Write_Read_ExodusII_HexMesh) {
  MFace_ptr mf;
  MVertex_ptr mv;
  MSet_ptr mset;

  MSTK_Init();

  /* Read a 2x2x2 mesh of hexes with full classification in MSTK format */ 

  Mesh_ptr mesh = MESH_New(UNKNOWN_REP);
  int ok = MESH_InitFromFile(mesh,"serial/reghex3D.mstk",NULL);

  /* Reclassify one layer of hexes as belonging to a different ID region */

  int elblock_numregs[2] = {6,2};
  int elblock_regids[2][7] = {{3,4,5,6,7,8},
                               {1,2,0,0,0,0,0}};
  
  for (int i = 0; i < 2; i++) {
    MRegion_ptr mr = MESH_Region(mesh,i);
    MR_Set_GEntID(mr,2);
  }

  /* Reclassify the faces between region 1 and region 2 as being on an
     internal surface */

  int idx = 0;
  while ((mf = MESH_Next_Face(mesh,&idx))) {

    if (MF_GEntDim(mf) == 3) {
      List_ptr fregs = MF_Regions(mf);
      if (MR_GEntID(List_Entry(fregs,0)) != MR_GEntID(List_Entry(fregs,1))) {
        MF_Set_GEntDim(mf,2);
        MF_Set_GEntID(mf,7);
      }
    }

  }


  /* Create two element sets */
  int elementset_regids[2][4];

  for (int i = 0; i < 2; i++) {
    char elementsetname[256];
    sprintf(elementsetname,"elementset_%-d",i+1);

    mset = MSet_New(mesh,elementsetname,MREGION);

    for (int j = 0; j < 4; j++) {
      MRegion_ptr mr = MESH_Region(mesh,i*4+j);
      MSet_Add(mset,mr);
      elementset_regids[i][j] = MR_ID(mr);
    }
  }

  /* Create 2 sidesets - 1 external (top surface) and one internal */

  idx = 0;
  double maxz = -1e8;
  while ((mv = MESH_Next_Vertex(mesh,&idx))) {
    double vxyz[3];
    MV_Coords(mv,vxyz);
    if (vxyz[2] > maxz)
      maxz = vxyz[2];
  }

  mset = MSet_New(mesh,"sideset_1",MFACE);
  idx = 0;
  while ((mf = MESH_Next_Face(mesh,&idx))) {
    double fxyz[MAXPV2][3];
    int nfv;
    int zmatch = 1;
    MF_Coords(mf,&nfv,fxyz);
    for (int i = 0; i < nfv; i++) {
      if (fxyz[i][2] != maxz) {
        zmatch=0;
        break;
      }
    }
    if (zmatch) MSet_Add(mset,mf);
  }

  mset = MSet_New(mesh,"sideset_2",MFACE);
    
  idx = 0;
  while ((mf = MESH_Next_Face(mesh,&idx))) {
    if (MF_GEntDim(mf) == 2 && MF_GEntID(mf) == 7) {
      MSet_Add(mset,mf);
    }
  }

  /* Create 2 nodesets - one composed of nodes on faces and the other 
   composed of nodes on edges */

  int nodeset_numverts[2] = {12,6};
  int nodeset_vertids[2][12] = {{2,4,6,8,10,12,16,18,20,22,24,26},
                                {5,11,13,15,17,23,0,0,0,0,0,0}};
  
  for (int i = 0; i < 2; i++) {
    char nodesetname[256];
    sprintf(nodesetname,"nodeset_%-d",11*(i+1));
    
    mset = MSet_New(mesh,nodesetname,MVERTEX);

    idx = 0;
    int j = 0;
    while ((mv = MESH_Next_Vertex(mesh,&idx))) {
      if (MV_GEntDim(mv) == i+1) {
        MSet_Add(mset,mv);
        nodeset_vertids[i][j] = MV_ID(mv);
        j++;
      }
    }
  }

  /* Now export to an Exodus II file */

  ok = MESH_ExportToFile(mesh,"temp.exo","exodusii",0,NULL,NULL,NULL);


  /* Now create another mesh and import this file back */

  Mesh_ptr mesh2 = MESH_New(F1);

  ok = MESH_ImportFromFile(mesh2,"temp.exo","exodusii",NULL,NULL);


  /* Now verify that we retrieved all the model regions (element
     blocks) as expected */

  int nr = MESH_Num_Regions(mesh2);
  CHECK_EQUAL(nr,MESH_Num_Regions(mesh));

  for (int i = 0; i < 2; i++) {
    for (int j = 0; j < elblock_numregs[i]; j++) {
      int rid = elblock_regids[i][j];
      MRegion_ptr mr = MESH_RegionFromID(mesh2,rid);
      CHECK(mr);
      CHECK_EQUAL(MR_GEntID(mr),i+1);
    }
  }

  /* Verify that we retrieved element sets as expected */

  for (int i = 0; i < 2; i++) {
    char elementsetname[256];
    sprintf(elementsetname,"elemset_%-d",i+1);

    mset = MESH_MSetByName(mesh2,elementsetname);
    CHECK(mset);

    for (int j = 0; j < 4; j++) {
      MRegion_ptr mr = (MRegion_ptr) MSet_Entry(mset,j);
      int rid = MR_ID(mr);
      int found = 0;
      for (int k = 0; k < 4; k++) {
        if (elementset_regids[i][k] == rid) {
          found = 1;
          break;
        }
      }
      CHECK_EQUAL(found,1);
    }
  }


  /* Verify that we retrieved side sets as expected */

  // Verify that sideset 1 has faces on the top surface

  mset = MESH_MSetByName(mesh2,"sideset_1");
  CHECK(mset);
  CHECK_EQUAL(4,MSet_Num_Entries(mset));
  for (int i = 0; i < 4; i++) {
    double fxyz[MAXPV2][3];
    int nfv, zmatch=1;
 
    mf = MSet_Entry(mset,i);
    MF_Coords(mf,&nfv,fxyz);
    
    for (int j = 0; j < nfv; j++) {
      if (fxyz[i][2] != maxz) {
        zmatch = 0;
        break;
      }
    }
    CHECK(zmatch);
  }

  // Verify that sideset 2 has only interior faces

  mset = MESH_MSetByName(mesh2,"sideset_2");
  CHECK(mset);
  CHECK_EQUAL(4,MSet_Num_Entries(mset));
  for (int i = 0; i < 4; i++) {
    List_ptr fregs = MF_Regions(MSet_Entry(mset,i));
    CHECK_EQUAL(2,List_Num_Entries(fregs));
    List_Delete(fregs);
  }  


  /* Verify that we retrieved node sets as expected */
  /* We cannot ensure that the node IDs in the original 
     MSTK mesh are the same as those in the mesh imported
     Exodus II files - so we will only match coordinates */

  for (int i = 0; i < 2; i++) {
    char nodesetname[256];
    sprintf(nodesetname,"nodeset_%-d",11*(i+1));

    mset = MESH_MSetByName(mesh2,nodesetname);
    CHECK(mset);

    for (int j = 0; j < nodeset_numverts[i]; j++) {
      mv = (MVertex_ptr) MSet_Entry(mset,j);
      double xyz[3];
      MV_Coords(mv,xyz);
      int found = 0;
      for (int k = 0; k < nodeset_numverts[i]; k++) {
        MVertex_ptr mv2 = MESH_VertexFromID(mesh,nodeset_vertids[i][k]);
        double xyz2[3];
        MV_Coords(mv2,xyz2);
        double dist2 = ((xyz[0]-xyz2[0])*(xyz[0]-xyz2[0]) +
                        (xyz[1]-xyz2[1])*(xyz[1]-xyz2[1]) +
                        (xyz[2]-xyz2[2])*(xyz[2]-xyz2[2]));
        if (dist2 < 1.0e-16) {
          found = 1;
          break;
        }
      }
      CHECK_EQUAL(found,1);
    }
  }

}
      
