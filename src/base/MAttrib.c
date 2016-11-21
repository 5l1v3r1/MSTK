#define _H_MAttrib_Private

#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "MAttrib.h"
#include "Mesh.h"
#include "MSTK_private.h"

#ifdef __cplusplus
extern "C" {
#endif


  MAttrib_ptr MAttrib_New(Mesh_ptr mesh, const char *att_name, MAttType att_type, MType entdim, ...) {
    MAttrib_ptr attrib;
    int ncomponents=1;
    va_list ap;

#ifdef DEBUG
    if(mesh) {
      int natt = MESH_Num_Attribs(mesh);
      int i;
      for (i = 0; i < natt; i++) {
	attrib = MESH_Attrib(mesh,i);
	if (strcmp(att_name,attrib->name) == 0) {
	  MSTK_Report("MAttrib_New","Attribute with given name exists",MSTK_WARN);
	  return attrib;
	}
      }      
    }
#endif

    attrib = (MAttrib_ptr) malloc(sizeof(MAttrib));
    attrib->name = (char *) malloc((strlen(att_name)+1)*sizeof(char));
    strcpy(attrib->name,att_name);

    attrib->type = att_type;
    
#ifdef DEBUG
    if (att_type != INT && att_type != DOUBLE && 
	att_type != VECTOR && att_type != TENSOR && att_type != POINTER) {
      MSTK_Report("MAttDef_New","Unsupported attribute type",MSTK_ERROR);
      return NULL;
    }
#endif

    attrib->entdim = entdim; /* what type of entity can this be attached to? */

    if (att_type == VECTOR || att_type == TENSOR) {
      va_start(ap, entdim);

      ncomponents = va_arg(ap, int);

#ifdef DEBUG
      if (ncomponents == 0)      
	MSTK_Report("MAttrib_New","Number of components for attribute type VECTOR or TENSOR should be non-zero",MSTK_FATAL);
#endif
      va_end(ap);
    }

    if (att_type == INT || att_type == DOUBLE || att_type == POINTER)
      attrib->ncomp = 1;
    else
      attrib->ncomp = ncomponents;
    if(mesh) {
      attrib->mesh = mesh;
      MESH_Add_Attrib(mesh,attrib);
    }
    va_end(ap);

    return attrib;
  }


  char *MAttrib_Get_Name(MAttrib_ptr attrib, char *att_name) {
    strcpy(att_name,attrib->name);
    return att_name;
  }

  MAttType MAttrib_Get_Type(MAttrib_ptr attrib) {
    return attrib->type;
  }

  MType MAttrib_Get_EntDim(MAttrib_ptr attrib) {
    return attrib->entdim;
  }

  int MAttrib_Get_NumComps(MAttrib_ptr attrib) {
    return attrib->ncomp;
  }

  void MAttrib_Delete(MAttrib_ptr attrib) {
    MESH_Rem_Attrib(attrib->mesh,attrib);
    free(attrib->name);
    free(attrib);
  }

  void MAttrib_Destroy_For_MESH_Delete(MAttrib_ptr attrib) {
    /*    
	  Don't need to remove the attribute from the mesh as the 
	  atttribute list will get destroyed and all attributes
	  will get removed from the entities during their destruction 
	  
	  MESH_Rem_Attrib(attrib->mesh,attrib); 
    */

    free(attrib->name);
    free(attrib);
  }


  void MAttrib_Clear(MAttrib_ptr attrib) {
    MESH_Clear_Attrib(attrib->mesh,attrib);
  }



  MAttIns_ptr MAttIns_New(MAttrib_ptr attrib) {
    MAttIns_ptr attins;

    attins = (MAttIns_ptr) malloc(sizeof(MAttIns));
    
    attins->attrib = attrib;
    attins->att_val.pval = NULL;

    return attins;
  }

  MAttrib_ptr MAttIns_Attrib(MAttIns_ptr attins) {
    return attins->attrib;
  }

  void MAttIns_Set_Value(MAttIns_ptr attins, int ival, double lval, void *pval) {
    MAttrib_ptr attrib = attins->attrib;

    switch (MAttrib_Get_Type(attrib)) {
      case INT:
        attins->att_val.ival = ival;
        break;
      case DOUBLE:
        attins->att_val.lval = lval;
        break;
      case VECTOR: {
        /* vectors are stored as pointers to data which needs to be
         * persistent - so copy the data into newly allocated space
         * before storing */
        
        int ncomp = MAttrib_Get_NumComps(attrib);
        double *vval_copy = (double *) malloc(ncomp*sizeof(double));
        memcpy(vval_copy, pval, ncomp*sizeof(double));
        attins->att_val.pval = vval_copy;
        break;
      }
      case TENSOR: {
        /* tensors are stored as pointers to data which needs to be
         * persistent - so copy the data into newly allocated space before
         * storing */
        
        int ncomp = MAttrib_Get_NumComps(attrib);
        double *tval_copy = (double *) malloc(ncomp*sizeof(double));
        memcpy(tval_copy, pval, ncomp*sizeof(double));
        attins->att_val.pval = tval_copy;
        break;
      }
      case POINTER:
        attins->att_val.pval = pval;
        break;
      default:
        break;
    }
  }
    
  int MAttIns_Get_Value(MAttIns_ptr attins, int *ival, double *lval, void **pval) {
    MAttrib_ptr attrib = attins->attrib;
    int status=1;

    switch (MAttrib_Get_Type(attrib)) {
    case INT:
      *ival = attins->att_val.ival;
      break;
    case DOUBLE:
      *lval = attins->att_val.lval;
      break;
    case VECTOR: case TENSOR: case POINTER:
      *pval = attins->att_val.pval;
    default:
      status = 0;
      break;
    }

    return status;
  }

  
  void MAttIns_Delete(MAttIns_ptr attins) {
    MAttType atttype = MAttrib_Get_Type(attins->attrib);

    if (atttype == VECTOR || atttype == TENSOR)
      if (attins->att_val.pval)
        free(attins->att_val.pval);

    free(attins);
  }



#ifdef __cplusplus
}
#endif
