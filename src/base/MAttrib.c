#define _H_MAttrib_Private

#include <string.h>
#include "MAttrib.h"
#include "Mesh.h"
#include "MSTK_private.h"

#ifdef __cplusplus
extern "C" {
#endif


  MAttrib_ptr MAttrib_New(Mesh_ptr mesh, const char *att_name, MAttType att_type, MType entdim) {
    MAttrib_ptr attrib;
    int i, natt;

#ifdef DEBUG
    natt = MESH_Num_Attribs(mesh);
    for (i = 0; i < natt; i++) {
      attrib = MESH_Attrib(mesh,i);
      if (strcmp(att_name,attrib->name) == 0) {
	MSTK_Report("MAttrib_New","Attribute with given name exists",WARN);
	return attrib;
      }
    }
#endif

    attrib = (MAttrib_ptr) MSTK_malloc(sizeof(MAttrib));
    attrib->name = (char *) MSTK_malloc((strlen(att_name)+1)*sizeof(char));
    strcpy(attrib->name,att_name);

    attrib->type = att_type;
    
#ifdef DEBUG
    if (att_type != INT && att_type != DOUBLE && att_type != POINTER) {
      MSTK_Report("MAttDef_New","Unsupported attribute type",ERROR);
      return NULL;
    }
#endif

    attrib->entdim = entdim; /* what type of entity can this be attached to? */
    
    attrib->mesh = mesh;
    MESH_Add_Attrib(mesh,attrib);

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

  void MAttrib_Delete(MAttrib_ptr attrib) {
    MESH_Rem_Attrib(attrib->mesh,attrib);
    MSTK_free(attrib->name);
    MSTK_free(attrib);
  }




  MAttIns_ptr MAttIns_New(MAttrib_ptr attrib) {
    MAttIns_ptr attins;

    attins = (MAttIns_ptr) MSTK_malloc(sizeof(MAttIns));
    
    attins->attrib = attrib;
    attins->att_val.ival = 0; 

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
    case POINTER:
      *pval = attins->att_val.pval;
      break;
    default:
      status = 0;
      break;
    }

    return status;
  }

  
  void MAttIns_Delete(MAttIns_ptr attins) {
    attins->attrib = NULL;
    attins->att_val.ival = 0;
    
    MSTK_free(attins);
  }



#ifdef __cplusplus
}
#endif
