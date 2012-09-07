#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "MSTK.h"
#include "MSTK_private.h"

#include "exodusII.h"
#ifdef EXODUSII_4
#include "exodusII_ext.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif


  /* Read an Exodus II (or Nemesis) file into MSTK */
  /* Nemesis files are the distributed versions of Exodus II files
     with the global IDs of elements and nodes encoded as element maps
     and node maps. Nemesis files also contain some additional info
     that can be read by the Nemesis API but we are choosing to ignore
     that for now. While Nemesis tools that produce distributed meshes
     from a single Exodus II file use the extension .par.N.n where N
     is the total number of processors and n is the rank of the
     particular process we will also accept .exo.N.n */

  /* Right now we are creating an attribute for material sets, side
     sets and node sets. We are ALSO creating meshsets for each of
     these entity sets. We are also setting mesh geometric entity IDs
     - Should we pick one of the first two? */

  int MESH_ImportFromExodusII(Mesh_ptr mesh, const char *filename, MSTK_Comm comm) {

  char mesg[256], funcname[32]="MESH_ImportFromExodusII";
  char title[256], elem_type[256], sidesetname[256], nodesetname[256];
  char matsetname[256];
  char **elem_blknames;
  int i, j, k, k1;
  int comp_ws = sizeof(double), io_ws = 0;
  int exoid=0, status;
  int ndim, nnodes, nelems, nelblock, nnodesets, nsidesets;
  int nedges, nedge_blk, nfaces, nface_blk, nelemsets;
  int nedgesets, nfacesets, nnodemaps, nedgemaps, nfacemaps, nelemmaps;
  int *elem_blk_ids, *connect, *node_map, *elem_map, *nnpe;
  int nelnodes, neledges, nelfaces;
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
  MAttrib_ptr nmapatt=NULL, elblockatt=NULL, nodesetatt=NULL, sidesetatt=NULL;
  MSet_ptr faceset=NULL, nodeset=NULL, sideset=NULL, matset=NULL;
  int distributed=0;
  
  ex_init_params exopar;

  FILE *fp;
  char basename[256], modfilename[256], *ext;

#ifdef MSTK_HAVE_MPI

  int rank=0, numprocs=1;

  MPI_Comm_size(comm,&numprocs);
  MPI_Comm_rank(comm,&rank);

  if (numprocs > 1) {

    strcpy(basename,filename);
    ext = strstr(basename,".exo"); /* Search for the exodus extension */
    if (ext) {

      ext[0] = '\0'; /* Truncate the basename string at the extension */

      /* Try opening the file with .exo.N.n extension in the hope that
       we have a distributed set of files */

      sprintf(modfilename,"%s.exo.%-d.%-d",basename,numprocs,rank);
      
      if ((fp = fopen(modfilename,"r"))) {
        fclose(fp);
        
        distributed = 1;
      }
      else {

        sprintf(modfilename,"%s.par.%-d.%-d",basename,numprocs,rank);
      
        if ((fp = fopen(modfilename,"r"))) {
          fclose(fp);
        
          distributed = 1;
        }
        else {
        
          distributed = 0;
        
          if (rank == 0) {          
            if ((fp = fopen(filename,"r"))) {
              fclose(fp);
            }
            else {  
              sprintf(mesg,"Cannot open/read Exodus II file %s.exo, %s.exo.%-d.%-d or %s.par.%-d.%-d",basename,basename,numprocs,rank,basename,numprocs,rank);
              MSTK_Report(funcname,mesg,MSTK_FATAL);        
            }
          
          }
        }  
      }

    }
    else {
      ext = strstr(basename,".par"); /* Search for the Nemesis extension */
      if (!ext)
        MSTK_Report(funcname,
                 "Exodus II/Nemesis I files must have .exo or .par extension",
                    MSTK_FATAL);        
      else
        ext[0] = '\0'; /* Truncate the basename string at the extension */

      sprintf(modfilename,"%s.par.%-d.%-d",basename,numprocs,rank);
      
      if ((fp = fopen(modfilename,"r"))) {
        fclose(fp);
        
        distributed = 1;
      }
    }


    /* Now that we;ve identified the extension, read the meshes and
       weave them together or read the whole mesh on processor 0 and
       distribute it */

    int num_ghost_layers = 1;
    int topodim;
    int input_type = 1;
  
    if (distributed) {

      /* Now weave the distributed meshes together so that appropriate
         ghost links are created */

      int read_status = MESH_ReadExodusII_Serial(mesh,modfilename,rank);

      if (!read_status) {
        sprintf(mesg,"Could not read Exodus II file %s successfully\n",
                modfilename);
        MSTK_Report(funcname,mesg,MSTK_FATAL);
      }

      topodim = (MESH_Num_Regions(mesh) == 0) ? 2 : 3;

      int weavestatus = MSTK_Weave_DistributedMeshes(mesh, topodim,
                                                     num_ghost_layers, 
                                                     input_type, comm);
      
      if (!weavestatus)
        MSTK_Report(funcname,
                    "Could not weave distributed meshes correctly together",
                    MSTK_FATAL);
    }
    else {

      Mesh_ptr globalmesh;

      if (rank == 0) { /* Read only on processor 0 */

        globalmesh = MESH_New(MESH_RepType(mesh));
        int read_status = MESH_ReadExodusII_Serial(globalmesh,filename,rank);

        if (!read_status) {
          sprintf(mesg,"Could not read Exodus II file %s successfully\n",
                  filename);
          MSTK_Report(funcname,mesg,MSTK_FATAL);
        }

        topodim = (MESH_Num_Regions(globalmesh) == 0) ? 2 : 3;
      }
      else
        globalmesh = NULL;

      int with_attr = 1;      
      int method=0;

#if defined (_MSTK_HAVE_ZOLTAN)
      method = 1;
#elif defined (_MSTK_HAVE_METIS)
      method = 0;
#else
      MSTK_Report(funcname,"No partitioner defined",MSTK_FATAL);
#endif

      int dist_status = MSTK_Mesh_Distribute(globalmesh, &mesh, &topodim, 
                                             num_ghost_layers,
                                             with_attr, method, comm);
      if (!dist_status)
        MSTK_Report(funcname,
                    "Could not distribute meshes to other processors",
                    MSTK_FATAL);

      if (rank == 0)
        MESH_Delete(globalmesh);
    }

    int parallel_check = MESH_Parallel_Check(mesh,comm);        

    if (!parallel_check)
      MSTK_Report(funcname,
                  "Parallel mesh checks failed",
                  MSTK_FATAL);

  } /* if numprocs > 1 */
  else {
    return (MESH_ReadExodusII_Serial(mesh,filename,rank));
  }



#else /* Serial process */
  return (MESH_ReadExodusII_Serial(mesh,filename,0));
#endif 
  

  return 1;

}


#ifdef __cplusplus
}
#endif
