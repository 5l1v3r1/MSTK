#include <stdio.h>
#include <math.h>

#ifdef __cplusplus
extern "C" {
#endif

void VDiff3(double *a, double *b, double *c) {
  c[0] = a[0]-b[0]; c[1] = a[1]-b[1]; c[2] = a[2]-b[2];
}

double VDot3(double *a, double *b) {
  return (a[0]*b[0]+a[1]*b[1]+a[2]*b[2]);
}

void VCross3(double *a, double *b, double *x) {
  x[0] = a[1]*b[2]-a[2]*b[1];
  x[1] = a[2]*b[0]-a[0]*b[2];
  x[2] = a[0]*b[1]-a[1]*b[0];
}

void VCopy3(double *a, double *b) {
  a[0] = b[0]; a[1] = b[1]; a[2] = b[2];
}

double Tet_Volume(double (*rxyz)[3]) {
  double vec01[3], vec02[3], vec03[3], n0[3], vol;

  /* Assume that vertices of tet are ordered so that they obey the
     right-hand rule. In other words, the normal of the face formed by
     the first three vertices points towards the fourth vertex */


  VDiff3(rxyz[1],rxyz[0],vec01);
  VDiff3(rxyz[2],rxyz[0],vec02);
  VDiff3(rxyz[3],rxyz[0],vec03);
  VCross3(vec01,vec02,n0);
  
  vol = VDot3(n0,vec03);
  return vol;

}


/* Procedure to compute volume of general polyhedron */

/* If the polyhedron is of a specific type, say a tet, pyramid, prism
   or a hex, then an ordering of vertices is implied and the
   coordinates are to be provide according to that ordering.
   So one can call 

   vol = PR_Volume(rxyz, n, 0, 0, 0); 

*/

/* 

   For a general polyhedron, connectivity info has to be provided to
   the routine so as to determine the various faces. 'nf' is the
   number of faces of the polyhedron, the array 'nfv' has the number
   of vertices of each face and rfverts has the indices of the face
   vertices. 

   The assumption is that each face is described such that its normal
   points into the polyhedron.

*/

double PR_Volume(double (*rxyz)[3], int n, int **rfverts, int *nfv, 
			  int nf) {

  int i, j, k, ind, inverted = 0;
  int feverts[2], (*everts)[2];
  double vol=0.0, tvol;
  double fcen[3], rcen[3], txyz[4][3];

#ifdef DEBUG
  if (n < 4)
    fprintf(stderr,"PR_Volume: Input is not a polyhedron\n");
#endif

  switch (n) {
  case 4:
    vol = Tet_Volume(rxyz);
    break;
  default: /* General polyhedron */

    /* Geometric center of polyhedron */

    rcen[0] = rcen[1] = rcen[2] = 0.0;
    for (i = 0; i < n; i++)
      for (k = 0; k < 3; k++)
	rcen[k] += rxyz[i][k];
    for (k = 0; k < 3; k++) 
      rcen[k] /= n;
    VCopy3(txyz[3],rcen);


    /* Form a tet with each edge of the polyhedron, the center of an
       attached face and the center of the polyhedron AND check the
       tet's volume */

    for (i = 0; i < nf; i++) {
      /* Face center */

      fcen[0] = fcen[1] = fcen[2] = 0.0;
      for (j = 0; j < nfv[i]; j++) {
	ind = rfverts[i][j];
	for (k = 0; k < 3; k++)
	  fcen[k] += rxyz[ind][k];
      }
      for (k = 0; k < 3; k++) 
	fcen[k] /= nfv[i];	

      VCopy3(txyz[2],fcen);

      for (j = 0; j < nfv[i]; j++) {
	ind = rfverts[i][j];
	VCopy3(txyz[0],rxyz[ind]);

	ind = rfverts[i][(j+1)%nfv[i]];
	VCopy3(txyz[1],rxyz[ind]);
	
	tvol = Tet_Volume(txyz);
	vol += fabs(tvol);

	if (tvol < 0.0) inverted = 1;
      }

    }
    if (inverted) vol = -vol;
  }
   
  return vol;
}
