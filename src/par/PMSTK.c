#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "MSTK.h"
#include "MSTK_private.h"

#ifdef __cplusplus
extern "C" {
#endif

  /* 
     MSTK high level parallel functions for user/application

     Author(s): Duo Wang, Rao Garimella

  */

  /* Partition a given mesh into 'num' submeshes, adding a 'ring'
     layers of ghost elements around each partition. If 'with_attr' is
     1, attributes from the mesh are copied onto the submeshes. This
     routine does not send the meshes to other partitions */

  int MSTK_Mesh_Partition(Mesh_ptr mesh, Mesh_ptr *submeshes, int num, int ring, 
			  int with_attr) {
    int i, j, natt, nset;
    char attr_name[256];
    MAttrib_ptr attrib;
    MSet_ptr mset;

    /* Split the mesh into 'num' submeshes */

    MESH_Partition(mesh, submeshes, num);

    for(i = 0; i < num; i++) {

      /* */

      MESH_BuildPBoundary(mesh,submeshes[i]);

      /* Add ghost layers */

      MESH_AddGhost(mesh,submeshes[i],i,ring);

    }

    if(!with_attr) 
      return 1;

    /* Also transmit attributes to the partitions */

    natt = MESH_Num_Attribs(mesh);
    for(j = 0; j < natt; j++) {
      attrib = MESH_Attrib(mesh,j);
      MAttrib_Get_Name(attrib,attr_name);
      for(i = 0; i < num; i++)
	MESH_CopyAttr(mesh,submeshes[i],attr_name);
    }

    /* Split the sets into subsets and send them to the partitions */
    nset = MESH_Num_MSets(mesh);
    for (j = 0; j < nset; j++) {
      mset = MESH_MSet(mesh,j);
      
      for (i = 0; i < num; i++)
	MESH_CopySet(mesh,submeshes[i],mset);
    }
    

    printf("global mesh has been partitioned into %d parts\n",num);
    return 1;
  }


  /* Send a mesh to a processor 'rank' with or without attributes */

  int MSTK_SendMesh(Mesh_ptr mesh, int rank, int with_attr, MPI_Comm comm) {
    int i, natt, nset;
    char attr_name[256], mset_name[256];
    MAttrib_ptr attrib;
    MSet_ptr mset;

    /* Send mesh only */

    MESH_SendMesh(mesh,rank,comm);

    if (!with_attr) 
      return 1;

    /* Send attributes as well */

    natt = MESH_Num_Attribs(mesh);
    for(i = 0; i < natt; i++) {
      attrib = MESH_Attrib(mesh,i);
      MAttrib_Get_Name(attrib,attr_name);
      MESH_SendAttr(mesh,attr_name,rank,comm);
    }

    /* Distribute entity sets */

    nset = MESH_Num_MSets(mesh);
    for(i = 0; i < nset; i++) {
      mset = MESH_MSet(mesh,i);
      MSet_Name(mset,mset_name);
      MESH_SendMSet(mesh,mset_name,rank,comm);
    }


    return 1;
  }

  /* Receive a mesh of dimension 'dim' from processor 'send_rank' with
     or without attributes onto processor 'rank' */

  int MSTK_RecvMesh(Mesh_ptr mesh, int dim, int send_rank, int rank, 
		    int with_attr, MPI_Comm comm) {
    int i, natt, nset;
    char attr_name[256], mset_name[256];
    MAttrib_ptr attrib;
    MSet_ptr mset;

    /* Receive the mesh only */

    MESH_RecvMesh(mesh,dim,send_rank,rank,comm);

    /* Build the sorted lists of ghost and overlap entities */

    MESH_Build_GhostLists(mesh);

    if(!with_attr)
      return 1;

    /* Receive attributes as well */

    natt = MESH_Num_Attribs(mesh);
    for(i = 0; i < natt; i++) {
      attrib = MESH_Attrib(mesh,i);
      MAttrib_Get_Name(attrib,attr_name);
      MESH_RecvAttr(mesh,attr_name,send_rank,rank,comm);
    }


    /* Receive attributes as well */

    nset = MESH_Num_MSets(mesh);
    for(i = 0; i < nset; i++) {
      mset = MESH_MSet(mesh,i);
      MSet_Name(mset,mset_name);
      MESH_RecvMSet(mesh,mset_name,send_rank,rank,comm);
    }

    return 1;
  }
    

  /* Read a mesh in, partition it and distribute it to 'num' processors */

  int MSTK_Mesh_Read_Distribute(Mesh_ptr *recv_mesh, 
				    const char* global_mesh_name, 
				    int dim, int ring, int with_attr, 
				    int rank, int num, 
				    MPI_Comm comm) {
    int i, ok;

    if(rank == 0) {
      Mesh_ptr mesh = MESH_New(UNKNOWN_REP);
      ok = MESH_InitFromFile(mesh,global_mesh_name);
      if (!ok) {
	fprintf(stderr,"Cannot open input file %s\n\n\n",global_mesh_name);
	exit(-1);
      }
      fprintf(stdout,"mstk mesh %s read in successfully\n",global_mesh_name);

      *recv_mesh = mesh;
    }

    MSTK_Mesh_Distribute(recv_mesh, dim, ring, with_attr, rank, num, comm);

    return 1;
  }


  /* Partition a given mesh and distribute it to 'num' processors */

  int MSTK_Mesh_Distribute(Mesh_ptr *mesh, int dim, 
			   int ring, int with_attr, 
			   int rank, int num, 
			   MPI_Comm comm) {
    int i;

    if(rank == 0) {    
      /* Partition the mesh*/
      Mesh_ptr *submeshes = (Mesh_ptr *) MSTK_malloc((num)*sizeof(Mesh_ptr));
      MSTK_Mesh_Partition(*mesh, submeshes, num, ring, with_attr);

      *mesh = submeshes[0];
      for(i = 1; i < num; i++) {
	MSTK_SendMesh(submeshes[i],i,with_attr,comm);
      }

    }
    if( rank > 0) {
      *mesh = MESH_New(UNKNOWN_REP);
      MSTK_RecvMesh(*mesh,dim,0,rank,with_attr,comm);
    }
    return 1;
  }



  /* Update attributes on partitions */

  int MSTK_UpdateAttr(Mesh_ptr mesh, int rank, int num,  MPI_Comm comm) {  
    int i, natt;
    char attr_name[256];
    MAttrib_ptr attrib;

    MESH_UpdateGlobalInfo(mesh, rank, num,  MPI_COMM_WORLD);

    natt = MESH_Num_Attribs(mesh);
    for(i = 0; i < natt; i++) {
      attrib = MESH_Attrib(mesh,i);
      MAttrib_Get_Name(attrib,attr_name);
      MPI_Barrier(comm);
      MESH_UpdateAttr(mesh,attr_name,rank,num,comm);
    }

    return 1;
  }


#ifdef __cplusplus
}
#endif

