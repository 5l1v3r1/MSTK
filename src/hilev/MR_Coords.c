#include <stdio.h>
#include <stdlib.h>
#include "MSTK.h"

#ifdef __cplusplus
extern "C" {
#endif

void MR_Coords(MRegion_ptr r, int *n, double (*xyz)[3]) {
  int i;
  MVertex_ptr rv;
  List_ptr rverts;

  rverts = MR_Vertices(r);
  *n = List_Num_Entries(rverts);

  for (i = 0; i < *n; i++) {
    rv = List_Entry(rverts,i);
    MV_Coords(rv,xyz[i]);
  }
  List_Delete(rverts);
}

#ifdef __cplusplus
}
#endif
