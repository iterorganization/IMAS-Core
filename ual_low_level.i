/* -*- C -*-  (syntax highlighting trick) */
%module ual_low_level
%{
#define SWIG_FILE_WITH_INIT
#include "ual_low_level_swig.h"
#include <stdlib.h>
%}

%init %{
  import_array();
%}

 
%include numpy.i


%define %low_level_arrays(DATA_TYPE, DATA_TYPECODE, DIM_TYPE)

/****************** Typemap for arguments of the get methods *************/
  
%typemap(argout) (DATA_TYPE **UAL_OUT_ARRAY1, DIM_TYPE* DIM1) {
  PyArrayObject *array; 
  PyObject *o1, *o2;
  int size_tot;
  int nbdim = 1;
  npy_intp size[1];
  DATA_TYPE* tab_out, *tab_in;
		
  if(result==0)
    {
      tab_in = *$1;
      size[0] = *$2;
      array = (PyArrayObject*) PyArray_SimpleNew(nbdim, size, DATA_TYPECODE);
      tab_out = (DATA_TYPE *) array->data;
      size_tot=size[0];
			
      memcpy(tab_out, tab_in, size_tot*sizeof(DATA_TYPE));
			
      free(tab_in);
    }
  else
    {
      size[0] = 0;
      array = (PyArrayObject*) PyArray_SimpleNew(nbdim, size, DATA_TYPECODE);
    }	
		
  if ((!$result) || ($result == Py_None)) {
    $result = PyArray_Return(array);
  } else {
    if (!PyTuple_Check($result)) {
      o1 = $result;
      $result = PyTuple_New(1);
      PyTuple_SetItem($result,0,o1);
    }
    o1 = PyTuple_New(1);
    PyTuple_SetItem(o1,0,PyArray_Return(array));
    o2 = $result;
    $result = PySequence_Concat(o2,o1);
    Py_DECREF(o1);
    Py_DECREF(o2);
  }

}



%typemap(argout) (DATA_TYPE **UAL_OUT_ARRAY2, DIM_TYPE* DIM1, DIM_TYPE* DIM2) {
  PyArrayObject *array; 
  PyObject *o1, *o2;
  int size_tot,i,j;
  int nbdim = 2;
  npy_intp size[2];
  DATA_TYPE* tab_out, *tab_in;
		
  if(result==0)
    {
      tab_in = *$1;
      size[0] = *$2;
      size[1] = *$3;
      array = (PyArrayObject*) PyArray_SimpleNew(nbdim, size, DATA_TYPECODE);
      tab_out = (DATA_TYPE *) array->data;
      size_tot=1;
      for(i=0; i < nbdim; ++i) 
	size_tot *= size[i];
			
      for(i=0; i < size[0]; ++i) 
	for(j=0; j < size[1]; ++j) 
	  tab_out[j + size[1]*i] = tab_in[i + size[0]* j];
			
      free(tab_in);
    }
  else
    {
      size[0] = 0;size[1] = 0;
      array = (PyArrayObject*) PyArray_SimpleNew(nbdim, size, DATA_TYPECODE);
    }	

  if ((!$result) || ($result == Py_None)) {
    $result = PyArray_Return(array);
  } else {
    if (!PyTuple_Check($result)) {
      o1 = $result;
      $result = PyTuple_New(1);
      PyTuple_SetItem($result,0,o1);
    }
    o1 = PyTuple_New(1);
    PyTuple_SetItem(o1,0,PyArray_Return(array));
    o2 = $result;
    $result = PySequence_Concat(o2,o1);
    Py_DECREF(o1);
    Py_DECREF(o2);
  }

}



%typemap(argout) (DATA_TYPE **UAL_OUT_ARRAY3, DIM_TYPE* DIM1, DIM_TYPE* DIM2, DIM_TYPE* DIM3) {
  PyArrayObject *array; 
  PyObject *o1, *o2;
  int size_tot,i,j,k;
  int nbdim = 3;
  npy_intp size[3];
  DATA_TYPE* tab_out, *tab_in;
		
  if(result==0)
    {
      tab_in = *$1;
      size[0] = *$2;
      size[1] = *$3;
      size[2] = *$4;
      array = (PyArrayObject*) PyArray_SimpleNew(nbdim, size, DATA_TYPECODE);
      tab_out = (DATA_TYPE *) array->data;
      size_tot=1;
      for(i=0; i < nbdim; ++i) 
	size_tot *= size[i];
			
      for(i=0; i < size[0]; ++i) 
	for(j=0; j < size[1]; ++j) 
	  for(k=0; k < size[2]; ++k) 
	    tab_out[k + size[2]* (j + size[1]*i)] = tab_in[i + size[0]*(j + size[1]*k)];

      free(tab_in);
    }
  else
    {
      size[0] = 0;size[1] = 0;size[2] = 0;
      array = (PyArrayObject*) PyArray_SimpleNew(nbdim, size, DATA_TYPECODE);
    }	

  if ((!$result) || ($result == Py_None)) {
    $result = PyArray_Return(array);
  } else {
    if (!PyTuple_Check($result)) {
      o1 = $result;
      $result = PyTuple_New(1);
      PyTuple_SetItem($result,0,o1);
    }
    o1 = PyTuple_New(1);
    PyTuple_SetItem(o1,0,PyArray_Return(array));
    o2 = $result;
    $result = PySequence_Concat(o2,o1);
    Py_DECREF(o1);
    Py_DECREF(o2);
  }

}



%typemap(argout) (DATA_TYPE **UAL_OUT_ARRAY4, DIM_TYPE* DIM1, DIM_TYPE* DIM2, DIM_TYPE* DIM3, DIM_TYPE* DIM4) {
  PyArrayObject *array; 
  PyObject *o1, *o2;
  int size_tot,i,j,k,l;
  int nbdim = 4;
  npy_intp size[4];
  DATA_TYPE* tab_out, *tab_in;
		
  if(result==0)
    {
      tab_in = *$1;
      size[0] = *$2;
      size[1] = *$3;
      size[2] = *$4;
      size[3] = *$5;
      array = (PyArrayObject*) PyArray_SimpleNew(nbdim, size, DATA_TYPECODE);
      tab_out = (DATA_TYPE *) array->data;
      size_tot=1;
      for(i=0; i < nbdim; ++i) 
	size_tot *= size[i];
			
      for(i=0; i < size[0]; ++i) 
	for(j=0; j < size[1]; ++j) 
	  for(k=0; k < size[2]; ++k) 
	    for(l=0; l < size[3]; ++l) 
	      tab_out[l+ size[3]*(k + size[2]*(j + size[1]*i))] = tab_in[i + size[0]*(j + size[1]*(k + size[2]*l))];
						
      free(tab_in);
    }
  else
    {
      size[0] = 0;size[1] = 0;size[2] = 0;size[3] = 0;
      array = (PyArrayObject*) PyArray_SimpleNew(nbdim, size, DATA_TYPECODE);
    }	
		
  if ((!$result) || ($result == Py_None)) {
    $result = PyArray_Return(array);
  } else {
    if (!PyTuple_Check($result)) {
      o1 = $result;
      $result = PyTuple_New(1);
      PyTuple_SetItem($result,0,o1);
    }
    o1 = PyTuple_New(1);
    PyTuple_SetItem(o1,0,PyArray_Return(array));
    o2 = $result;
    $result = PySequence_Concat(o2,o1);
    Py_DECREF(o1);
    Py_DECREF(o2);
  }
}



%typemap(argout) (DATA_TYPE **UAL_OUT_ARRAY5, DIM_TYPE* DIM1, DIM_TYPE* DIM2, DIM_TYPE* DIM3, DIM_TYPE* DIM4, DIM_TYPE* DIM5) {
  PyArrayObject *array; 
  PyObject *o1, *o2;
  int size_tot,i,j,k,l,m;
  int nbdim = 5;
  npy_intp size[5];
  DATA_TYPE* tab_out, *tab_in;
		
  if(result==0)
    {
      tab_in = *$1;
      size[0] = *$2;
      size[1] = *$3;
      size[2] = *$4;
      size[3] = *$5;
      size[4] = *$6;
      array = (PyArrayObject*) PyArray_SimpleNew(nbdim, size, DATA_TYPECODE);
      tab_out = (DATA_TYPE *) array->data;
      size_tot=1;
      for(i=0; i < nbdim; ++i) 
	size_tot *= size[i];
			
      for(i=0; i < size[0]; ++i) 
	for(j=0; j < size[1]; ++j) 
	  for(k=0; k < size[2]; ++k) 
	    for(l=0; l < size[3]; ++l) 
	      for(m=0; m < size[4]; m++)
		tab_out[m + size[4]*(l + size[3]*(k + size[2]*(j + size[1]*i)))] = tab_in[i + size[0]*(j + size[1]*(k + size[2]*(l + size[3]*m)))];
						
      free(tab_in);
    }
  else
    {
      size[0] = 0;size[1] = 0;size[2] = 0;size[3] = 0;size[4] = 0;
      array = (PyArrayObject*) PyArray_SimpleNew(nbdim, size, DATA_TYPECODE);
    }	
		
  if ((!$result) || ($result == Py_None)) {
    $result = PyArray_Return(array);
  } else {
    if (!PyTuple_Check($result)) {
      o1 = $result;
      $result = PyTuple_New(1);
      PyTuple_SetItem($result,0,o1);
    }
    o1 = PyTuple_New(1);
    PyTuple_SetItem(o1,0,PyArray_Return(array));
    o2 = $result;
    $result = PySequence_Concat(o2,o1);
    Py_DECREF(o1);
    Py_DECREF(o2);
  }
}



%typemap(argout) (DATA_TYPE **UAL_OUT_ARRAY6, DIM_TYPE* DIM1, DIM_TYPE* DIM2, DIM_TYPE* DIM3, DIM_TYPE* DIM4, DIM_TYPE* DIM5, DIM_TYPE* DIM6) {
  PyArrayObject *array; 
  PyObject *o1, *o2;
  int size_tot,i,j,k,l,m,n;
  int nbdim = 6;
  npy_intp size[6];
  DATA_TYPE* tab_out, *tab_in;
		
  if(result==0)
    {
      tab_in = *$1;
      size[0] = *$2;
      size[1] = *$3;
      size[2] = *$4;
      size[3] = *$5;
      size[4] = *$6;
      size[5] = *$7;
      array = (PyArrayObject*) PyArray_SimpleNew(nbdim, size, DATA_TYPECODE);
      tab_out = (DATA_TYPE *) array->data;
      size_tot=1;
      for(i=0; i < nbdim; ++i) 
	size_tot *= size[i];
			
      for(i=0; i < size[0]; ++i) 
	for(j=0; j < size[1]; ++j) 
	  for(k=0; k < size[2]; ++k) 
	    for(l=0; l < size[3]; ++l) 
	      for(m=0; m < size[4]; m++)
		for(n=0; n < size[5]; n++)
		  tab_out[n + size[5]*(m + size[4]*(l + size[3]*(k + size[2]*(j + size[1]*i))))] = tab_in[i + size[0]*(j + size[1]*(k + size[2]*(l + size[3]*(m + size[4]*n))))];
						
      free(tab_in);
    }
  else
    {
      size[0] = 0;size[1] = 0;size[2] = 0;size[3] = 0;size[4] = 0;size[5] = 0;
      array = (PyArrayObject*) PyArray_SimpleNew(nbdim, size, DATA_TYPECODE);
    }	
		
  if ((!$result) || ($result == Py_None)) {
    $result = PyArray_Return(array);
  } else {
    if (!PyTuple_Check($result)) {
      o1 = $result;
      $result = PyTuple_New(1);
      PyTuple_SetItem($result,0,o1);
    }
    o1 = PyTuple_New(1);
    PyTuple_SetItem(o1,0,PyArray_Return(array));
    o2 = $result;
    $result = PySequence_Concat(o2,o1);
    Py_DECREF(o1);
    Py_DECREF(o2);
  }
}



%typemap(argout) (DATA_TYPE **UAL_OUT_ARRAY7, DIM_TYPE* DIM1, DIM_TYPE* DIM2, DIM_TYPE* DIM3, DIM_TYPE* DIM4, DIM_TYPE* DIM5, DIM_TYPE* DIM6, DIM_TYPE* DIM7) {
  PyArrayObject *array; 
  PyObject *o1, *o2;
  int size_tot,i,j,k,l,m,n,o;
  int nbdim = 7;
  npy_intp size[7];
  DATA_TYPE* tab_out, *tab_in;
		
  if(result==0)
    {
      tab_in = *$1;
      size[0] = *$2;
      size[1] = *$3;
      size[2] = *$4;
      size[3] = *$5;
      size[4] = *$6;
      size[5] = *$7;
      size[6] = *$8;
      array = (PyArrayObject*) PyArray_SimpleNew(nbdim, size, DATA_TYPECODE);
      tab_out = (DATA_TYPE *) array->data;
      size_tot=1;
      for(i=0; i < nbdim; ++i) 
	size_tot *= size[i];
			
      for(i=0; i < size[0]; ++i) 
	for(j=0; j < size[1]; ++j) 
	  for(k=0; k < size[2]; ++k) 
	    for(l=0; l < size[3]; ++l) 
	      for(m=0; m < size[4]; m++)
		for(n=0; n < size[5]; n++)
		  for(o=0; o < size[6]; o++)
		    tab_out[o + size[6]*(n + size[5]*(m + size[4]*(l + size[3]*(k + size[2]*(j + size[1]*i)))))] = tab_in[i + size[0]* (j + size[1] * (k + size[2] * (l + size[3] * (m + size[4] * (n + size[5] * o)))))];	
      free(tab_in);
    }
  else
    {
      size[0] = 0;size[1] = 0;size[2] = 0;size[3] = 0;size[4] = 0;size[5] = 0;size[6] = 0;
      array = (PyArrayObject*) PyArray_SimpleNew(nbdim, size, DATA_TYPECODE);
    }	
		
  if ((!$result) || ($result == Py_None)) {
    $result = PyArray_Return(array);
  } else {
    if (!PyTuple_Check($result)) {
      o1 = $result;
      $result = PyTuple_New(1);
      PyTuple_SetItem($result,0,o1);
    }
    o1 = PyTuple_New(1);
    PyTuple_SetItem(o1,0,PyArray_Return(array));
    o2 = $result;
    $result = PySequence_Concat(o2,o1);
    Py_DECREF(o1);
    Py_DECREF(o2);
  }
}



%typemap(in,numinputs=0) DATA_TYPE **UAL_OUT_ARRAY(DATA_TYPE* temp) {
  $1 = &temp;
}




/****************** Typemap for arguments of the put methods *************/

// Copy/paste/modify from numpy.i to design arrays mapping => cannot be used directly because of transposition

%typemap(in)
  (DATA_TYPE* UAL_IN_ARRAY2, DIM_TYPE DIM1, DIM_TYPE DIM2)
{
  PyArrayObject* array=NULL;
  DIM_TYPE size[2] = {-1, -1};
  npy_intp size_py[2] = {-1, -1};
  int size_tot, i,j;
  DATA_TYPE* tab_in;
  int nbdim=2;
	
  array = obj_to_array_no_conversion($input, DATA_TYPECODE);
  if (!array || !require_dimensions(array, 2) || !require_size(array, size_py, 2)) SWIG_fail;
	
  size[0] = (DIM_TYPE) array_size(array,0);
  size[1] = (DIM_TYPE) array_size(array,1);
  
  tab_in = (DATA_TYPE*) array_data(array);
  size_tot=1;
  for(i=0; i < nbdim; ++i) 
    size_tot *= size[i];
	
  $1 = (DATA_TYPE*) malloc(size_tot*sizeof(DATA_TYPE));
	
  for(j=0; j < size[1]; ++j) 
    for(i=0; i < size[0]; ++i)
      $1[i + size[0]*j] = tab_in[j + size[1]*i];
	
  $2 = size[0];
  $3 = size[1];
}

%typemap(freearg) (DATA_TYPE* UAL_IN_ARRAY2, DIM_TYPE DIM1, DIM_TYPE DIM2)
{
  free($1);
}



%typemap(in)
  (DATA_TYPE* UAL_IN_ARRAY3, DIM_TYPE DIM1, DIM_TYPE DIM2, DIM_TYPE DIM3)
{
  PyArrayObject* array=NULL;
  DIM_TYPE size[3] = { -1, -1, -1};
  npy_intp size_py[3] = { -1, -1, -1};
  int size_tot, i,j,k;
  DATA_TYPE* tab_in;
  int nbdim=3;
	
  array = obj_to_array_no_conversion($input, DATA_TYPECODE);
  if (!array || !require_dimensions(array, 3) || !require_size(array, size_py, 3)) SWIG_fail;
	
  size[0] = (DIM_TYPE) array_size(array,0);
  size[1] = (DIM_TYPE) array_size(array,1);
  size[2] = (DIM_TYPE) array_size(array,2);
	
  tab_in = (DATA_TYPE*) array_data(array);
  size_tot=1;
  for(i=0; i < nbdim; ++i) 
    size_tot *= size[i];
	
  $1 = (DATA_TYPE*) malloc(size_tot*sizeof(DATA_TYPE));
	
  for(k=0; k < size[2]; ++k)
    for(j=0; j < size[1]; ++j) 
      for(i=0; i < size[0]; ++i)
	$1[i + size[0]*(j + size[1]*k)] = tab_in[k + size[2]*(j + size[1]*i)];
	
  $2 = size[0];
  $3 = size[1];
  $4 = size[2];
}

%typemap(freearg) (DATA_TYPE* UAL_IN_ARRAY3, DIM_TYPE DIM1, DIM_TYPE DIM2, DIM_TYPE DIM3)
{
  free($1);
}



%typemap(in)
  (DATA_TYPE* UAL_IN_ARRAY4, DIM_TYPE DIM1, DIM_TYPE DIM2, DIM_TYPE DIM3, DIM_TYPE DIM4)
{
  PyArrayObject* array=NULL;
  DIM_TYPE size[4] = {-1, -1, -1, -1};
  npy_intp size_py[4] = {-1, -1, -1, -1};
  int size_tot, i,j,k,l;
  DATA_TYPE* tab_in;
  int nbdim=4;
	
  array = obj_to_array_no_conversion($input, DATA_TYPECODE);
  if (!array || !require_dimensions(array, 4) || !require_size(array, size_py, 4)) SWIG_fail;
	
  size[0] = (DIM_TYPE) array_size(array,0);
  size[1] = (DIM_TYPE) array_size(array,1);
  size[2] = (DIM_TYPE) array_size(array,2);
  size[3] = (DIM_TYPE) array_size(array,3);
	
  tab_in = (DATA_TYPE*) array_data(array);
  size_tot=1;
  for(i=0; i < nbdim; ++i) 
    size_tot *= size[i];
	
  $1 = (DATA_TYPE*) malloc(size_tot*sizeof(DATA_TYPE));
	
  for(l=0; l < size[3]; ++l)
    for(k=0; k < size[2]; ++k)
      for(j=0; j < size[1]; ++j) 
	for(i=0; i < size[0]; ++i)
	  $1[i + size[0]*(j + size[1]*(k + size[2]*l))] = tab_in[l + size[3]*(k + size[2]*(j + size[1]*i))];
		
  $2 = size[0];
  $3 = size[1];
  $4 = size[2];
  $5 = size[3];
}

%typemap(freearg) (DATA_TYPE* UAL_IN_ARRAY4, DIM_TYPE DIM1, DIM_TYPE DIM2, DIM_TYPE DIM3, DIM_TYPE DIM4)
{
  free($1);
}



%typemap(in)
  (DATA_TYPE* UAL_IN_ARRAY5, DIM_TYPE DIM1, DIM_TYPE DIM2, DIM_TYPE DIM3, DIM_TYPE DIM4, DIM_TYPE DIM5)
{
  PyArrayObject* array=NULL;
  DIM_TYPE size[5] = {-1, -1, -1, -1, -1};
  npy_intp size_py[5] = {-1, -1, -1, -1, -1};
  int size_tot, i,j,k,l,m;
  DATA_TYPE* tab_in;
  int nbdim=5;
	
  array = obj_to_array_no_conversion($input, DATA_TYPECODE);
  if (!array || !require_dimensions(array, 5) || !require_size(array, size_py, 5)) SWIG_fail;
	
  size[0] = (DIM_TYPE) array_size(array,0);
  size[1] = (DIM_TYPE) array_size(array,1);
  size[2] = (DIM_TYPE) array_size(array,2);
  size[3] = (DIM_TYPE) array_size(array,3);
  size[4] = (DIM_TYPE) array_size(array,4);
	
  tab_in = (DATA_TYPE*) array_data(array);
  size_tot=1;
  for(i=0; i < nbdim; ++i) 
    size_tot *= size[i];
	
  $1 = (DATA_TYPE*) malloc(size_tot*sizeof(DATA_TYPE));
	
  for(m=0; m < size[4]; ++m)
    for(l=0; l < size[3]; ++l)
      for(k=0; k < size[2]; ++k)
	for(j=0; j < size[1]; ++j) 
	  for(i=0; i < size[0]; ++i)
	    $1[i + size[0]*(j + size[1]*(k + size[2]*(l + size[3]*m)))] = tab_in[m + size[4]*(l + size[3]*(k + size[2]*(j + size[1]*i)))];
		
  $2 = size[0];
  $3 = size[1];
  $4 = size[2];
  $5 = size[3];
  $6 = size[4];
}

%typemap(freearg) (DATA_TYPE* UAL_IN_ARRAY5, DIM_TYPE DIM1, DIM_TYPE DIM2, DIM_TYPE DIM3, DIM_TYPE DIM4, DIM_TYPE DIM5)
{
  free($1);
}



%typemap(in)
  (DATA_TYPE* UAL_IN_ARRAY6, DIM_TYPE DIM1, DIM_TYPE DIM2, DIM_TYPE DIM3, DIM_TYPE DIM4, DIM_TYPE DIM5, DIM_TYPE DIM6)
{
  PyArrayObject* array=NULL;
  DIM_TYPE size[6] = {-1, -1, -1, -1, -1, -1};
  npy_intp size_py[6] = {-1, -1, -1, -1, -1, -1};
  int size_tot, i,j,k,l,m,n;
  DATA_TYPE* tab_in;
  int nbdim=6;
	
  array = obj_to_array_no_conversion($input, DATA_TYPECODE);
  if (!array || !require_dimensions(array, 6) || !require_size(array, size_py, 6)) SWIG_fail;
	
  size[0] = (DIM_TYPE) array_size(array,0);
  size[1] = (DIM_TYPE) array_size(array,1);
  size[2] = (DIM_TYPE) array_size(array,2);
  size[3] = (DIM_TYPE) array_size(array,3);
  size[4] = (DIM_TYPE) array_size(array,4);
  size[5] = (DIM_TYPE) array_size(array,5);
	
  tab_in = (DATA_TYPE*) array_data(array);
  size_tot=1;
  for(i=0; i < nbdim; ++i) 
    size_tot *= size[i];
	
  $1 = (DATA_TYPE*) malloc(size_tot*sizeof(DATA_TYPE));

  for(n=0; n < size[5]; ++n)
    for(m=0; m < size[4]; ++m)
      for(l=0; l < size[3]; ++l)
	for(k=0; k < size[2]; ++k)
	  for(j=0; j < size[1]; ++j) 
	    for(i=0; i < size[0]; ++i)
	      $1[i + size[0]* (j + size[1]* (k + size[2]* (l + size[3]* (m + size[4]* n))))] = tab_in[n + size[5]*(m + size[4]*(l + size[3]*(k + size[2]* (j + size[1]*i))))];
		
  $2 = size[0];
  $3 = size[1];
  $4 = size[2];
  $5 = size[3];
  $6 = size[4];
  $7 = size[5];
}

%typemap(freearg) (DATA_TYPE* UAL_IN_ARRAY6, DIM_TYPE DIM1, DIM_TYPE DIM2, DIM_TYPE DIM3, DIM_TYPE DIM4, DIM_TYPE DIM5, DIM_TYPE DIM6)
{
  free($1);
}




%typemap(in)
  (DATA_TYPE* UAL_IN_ARRAY7, DIM_TYPE DIM1, DIM_TYPE DIM2, DIM_TYPE DIM3, DIM_TYPE DIM4, DIM_TYPE DIM5, DIM_TYPE DIM6, DIM_TYPE DIM7)
{
  PyArrayObject* array=NULL;
  DIM_TYPE size[7] = {-1, -1, -1, -1, -1, -1, -1};
  npy_intp size_py[7] = {-1, -1, -1, -1, -1, -1, -1};
  int size_tot, i,j,k,l,m,n,o;
  DATA_TYPE* tab_in;
  int nbdim=7;
	
  array = obj_to_array_no_conversion($input, DATA_TYPECODE);
  if (!array || !require_dimensions(array, 7) || !require_size(array, size_py, 7)) SWIG_fail;
	
  size[0] = (DIM_TYPE) array_size(array,0);
  size[1] = (DIM_TYPE) array_size(array,1);
  size[2] = (DIM_TYPE) array_size(array,2);
  size[3] = (DIM_TYPE) array_size(array,3);
  size[4] = (DIM_TYPE) array_size(array,4);
  size[5] = (DIM_TYPE) array_size(array,5);
  size[6] = (DIM_TYPE) array_size(array,6);
	
  tab_in = (DATA_TYPE*) array_data(array);
  size_tot=1;
  for(i=0; i < nbdim; ++i) 
    size_tot *= size[i];
	
  $1 = (DATA_TYPE*) malloc(size_tot*sizeof(DATA_TYPE));
	
  for(o=0; o < size[6]; ++o)
    for(n=0; n < size[5]; ++n)
      for(m=0; m < size[4]; ++m)
	for(l=0; l < size[3]; ++l)
	  for(k=0; k < size[2]; ++k)
	    for(j=0; j < size[1]; ++j) 
	      for(i=0; i < size[0]; ++i)
		$1[i + size[0]* (j + size[1]* (k + size[2]* (l + size[3]* (m + size[4]* (n + size[5]* o)))))] = tab_in[o + size[6]*(n + size[5]*(m + size[4]*(l + size[3]*(k + size[2]* (j + size[1]*i)))))];
		
  $2 = size[0];
  $3 = size[1];
  $4 = size[2];
  $5 = size[3];
  $6 = size[4];
  $7 = size[5];
  $8 = size[6];
}

%typemap(freearg) (DATA_TYPE* UAL_IN_ARRAY6, DIM_TYPE DIM1, DIM_TYPE DIM2, DIM_TYPE DIM3, DIM_TYPE DIM4, DIM_TYPE DIM5, DIM_TYPE DIM6)
{
  free($1);
}


%enddef    /* %low_level_arrays() macro */


 //Apply all these templates to the threee following types
%low_level_arrays(int               , NPY_INT      , int)
%low_level_arrays(float             , NPY_FLOAT    , int)
%low_level_arrays(double            , NPY_DOUBLE   , int)



//Specific typemap to manage dimout arguments
%typemap(in,numinputs=0) int *DIM_OUT (int temp) {
  $1 = &temp;
}



%define %low_level_scalar(DATA_TYPE, PYOBJECT_FUNC, CAST_TYPE)

// Typemap to handle scalar output
%typemap(argout) DATA_TYPE *OUT_SCALAR {
  PyObject* o1, *o2;
  PyObject* my_py_int = PYOBJECT_FUNC((CAST_TYPE)*$1);
  if ((!$result) || ($result == Py_None)) {
    $result = my_py_int;
  } else {
    if (!PyTuple_Check($result)) {
      o1 = $result;
      $result = PyTuple_New(1);
      PyTuple_SetItem($result,0,o1);
    }
    o2 = PyTuple_New(1);
    PyTuple_SetItem(o2,0,my_py_int);
    o1 = $result;
    $result = PySequence_Concat(o1,o2);
    Py_DECREF(o1);
    Py_DECREF(o2);
  }
}

%typemap(in,numinputs=0) DATA_TYPE *OUT_SCALAR (DATA_TYPE temp) {
  $1 = &temp;
}

%enddef    /* %low_level_scalar() macro */

//Apply all these templates to the threee following types
%low_level_scalar(int               , PyInt_FromLong, long)
%low_level_scalar(float             , PyFloat_FromDouble, double)
%low_level_scalar(double            , PyFloat_FromDouble, double)




 // Typemap to handle getDim...-like fonctions
%typemap(argout) (int *NDIMS, int *DIM1, int *DIM2, int *DIM3, int *DIM4, int *DIM5, int *DIM6, int *DIM7) {
  PyObject *o1, *o2;
  PyObject *my_py_int = PyInt_FromLong((long)*$1);
  int i = 0;
  int nbd = *$1;

  if ((!$result) || ($result == Py_None)) {
    $result = my_py_int;
  } else {
    if (!PyTuple_Check($result)) {
      o1 = $result;
      $result = PyTuple_New(1);
      PyTuple_SetItem($result,0,o1);
    }
    o2 = PyTuple_New(1);
    PyTuple_SetItem(o2,0,my_py_int);
    o1 = $result;
    $result = PySequence_Concat(o1,o2);
    Py_DECREF(o1);
    Py_DECREF(o2);
    Py_DECREF(my_py_int);

    for (i=1; i<=nbd; i++) {

      switch (i) {

      case 1:
	my_py_int = PyInt_FromLong((long)*$2);
	o2 = PyTuple_New(1);
	PyTuple_SetItem(o2,0,my_py_int);
	o1 = $result;
	$result = PySequence_Concat(o1,o2);
	Py_DECREF(o1);
	Py_DECREF(o2);
	Py_DECREF(my_py_int);
	break;

      case 2:
	my_py_int = PyInt_FromLong((long)*$3);
	o2 = PyTuple_New(1);
	PyTuple_SetItem(o2,0,my_py_int);
	o1 = $result;
	$result = PySequence_Concat(o1,o2);
	Py_DECREF(o1);
	Py_DECREF(o2);
	Py_DECREF(my_py_int);
	break;
      
      case 3:
	my_py_int = PyInt_FromLong((long)*$4);
	o2 = PyTuple_New(1);
	PyTuple_SetItem(o2,0,my_py_int);
	o1 = $result;
	$result = PySequence_Concat(o1,o2);
	Py_DECREF(o1);
	Py_DECREF(o2);
	Py_DECREF(my_py_int);
	break;

      case 4:
	my_py_int = PyInt_FromLong((long)*$5);
	o2 = PyTuple_New(1);
	PyTuple_SetItem(o2,0,my_py_int);
	o1 = $result;
	$result = PySequence_Concat(o1,o2);
	Py_DECREF(o1);
	Py_DECREF(o2);
	Py_DECREF(my_py_int);
	break;

      case 5:
	my_py_int = PyInt_FromLong((long)*$6);
	o2 = PyTuple_New(1);
	PyTuple_SetItem(o2,0,my_py_int);
	o1 = $result;
	$result = PySequence_Concat(o1,o2);
	Py_DECREF(o1);
	Py_DECREF(o2);
	Py_DECREF(my_py_int);
	break;

      case 6:
	my_py_int = PyInt_FromLong((long)*$7);
	o2 = PyTuple_New(1);
	PyTuple_SetItem(o2,0,my_py_int);
	o1 = $result;
	$result = PySequence_Concat(o1,o2);
	Py_DECREF(o1);
	Py_DECREF(o2);
	Py_DECREF(my_py_int);
	break;

      case 7:
	my_py_int = PyInt_FromLong((long)*$8);
	o2 = PyTuple_New(1);
	PyTuple_SetItem(o2,0,my_py_int);
	o1 = $result;
	$result = PySequence_Concat(o1,o2);
	Py_DECREF(o1);
	Py_DECREF(o2);
	break;
	
      default:
	SWIG_exception_fail(SWIG_ValueError, "in method get...Dimension wrong value of numDims");

      }
    }
  }
}



// Typemap to handle pointer output
%typemap(argout) void **OUT_PTR {
  PyObject* o1, *o2;
  PyObject* my_py_obj = PyCObject_FromVoidPtr(*$1,NULL); 
  if ((!$result) || ($result == Py_None)) {
    $result = my_py_obj;
  } else {
    if (!PyTuple_Check($result)) {
      o1 = $result;
      $result = PyTuple_New(1);
      PyTuple_SetItem($result,0,o1);
    }
    o2 = PyTuple_New(1);
    PyTuple_SetItem(o2,0,my_py_obj);
    o1 = $result;
    $result = PySequence_Concat(o1,o2);
    Py_DECREF(o1);
    Py_DECREF(o2);
  }
}

//Specific typemap to manage obj_out arguments
%typemap(in,numinputs=0) void **OUT_PTR (void *temp) {
  $1 = &temp;
}



// wrap char * inputs
%typemap(in) char * {
  if (PyString_Check($input)) {
    $1 = (char*) PyString_AsString($input);
  } else {
    PyErr_SetString(PyExc_ValueError,"input not a String!");
  }
}
%typemap(freearg) char * {
  $1 = NULL;
}

// same in case char * are const char *
%typemap(in) const char * {
  if (PyString_Check($input)) {
    $1 = (char*) PyString_AsString($input);
  } else {
    PyErr_SetString(PyExc_ValueError,"input not a String!");
  }
}
%typemap(freearg) const char * {
  $1 = NULL;
}



// wrap void * inputs
%typemap(in) void * {
  if (PyCObject_Check($input)) {
    if ($input == Py_None)
      $1 = NULL;
    else
      $1 = PyCObject_AsVoidPtr($input);
  }
}

// convert for void * return of functions
%typemap(out) void * {
  $result = PyCObject_FromVoidPtr($1, NULL);
}


//Apply the typemap on output struct objects pointers
%apply void **OUT_PTR {void **obj_out, void **dataObj_out};


%apply (int *NDIMS, int *DIM1, int *DIM2, int *DIM3, int *DIM4, int *DIM5, int *DIM6, int *DIM7) {(int *numDims, int *dim_out1, int *dim_out2, int *dim_out3, int *dim_out4, int *dim_out5, int *dim_out6, int *dim_out7)};


//Apply the typemap on arguments (dimensions size) of the get method and some other create, open and begin* methods
%apply int* DIM_OUT { int* dim_out, int* dim_out1, int* dim_out2, int* dim_out3, int* dim_out4, int* dim_out5, int* dim_out6, int* dim_out7};


//Apply the typemap on output arguments create, open and begin* methods
%apply int* OUT_SCALAR {int *retIdx, int* retSamples, int* data_out }; 
%apply float* OUT_SCALAR {float* data_out }; 
%apply double* OUT_SCALAR {double* data_out, double* retTime }; 


//Apply the typemap on arguments (arrays) of the get method 
%apply int **UAL_OUT_ARRAY { int ** data_out};
%apply float **UAL_OUT_ARRAY { float ** data_out};
%apply double **UAL_OUT_ARRAY { double ** data_out};


//Apply ual typemaps for 1D, 2D, 3D and 4D arrays for the get methods bindings
%apply (int** UAL_OUT_ARRAY1, int *DIM1) {(int** data_out, int *dim_out)};
%apply (int** UAL_OUT_ARRAY2, int *DIM1, int *DIM2) {(int** data_out, int *dim_out1, int *dim_out2 )};
%apply (int** UAL_OUT_ARRAY3, int *DIM1, int *DIM2, int *DIM3) {(int** data_out, int *dim_out1, int *dim_out2, int *dim_out3)};
%apply (int** UAL_OUT_ARRAY4, int *DIM1, int *DIM2, int *DIM3, int *DIM4) {(int** data_out, int *dim_out1, int *dim_out2, int *dim_out3, int *dim_out4)};
%apply (int** UAL_OUT_ARRAY5, int *DIM1, int *DIM2, int *DIM3, int *DIM4, int *DIM5) {(int** data_out, int *dim_out1, int *dim_out2, int *dim_out3, int *dim_out4, int *dim_out5)};
%apply (int** UAL_OUT_ARRAY6, int *DIM1, int *DIM2, int *DIM3, int *DIM4, int *DIM5, int *DIM6) {(int** data_out, int *dim_out1, int *dim_out2, int *dim_out3, int *dim_out4, int *dim_out5, int *dim_out6)};
%apply (int** UAL_OUT_ARRAY7, int *DIM1, int *DIM2, int *DIM3, int *DIM4, int *DIM5, int *DIM6, int *DIM7) {(int** data_out, int *dim_out1, int *dim_out2, int *dim_out3, int *dim_out4, int *dim_out5, int *dim_out6, int *dim_out7)};
%apply (float** UAL_OUT_ARRAY1, int *DIM1) {(float** data_out, int *dim_out)};
%apply (float** UAL_OUT_ARRAY2, int *DIM1, int *DIM2) {(float** data_out, int *dim_out1, int *dim_out2 )};
%apply (float** UAL_OUT_ARRAY3, int *DIM1, int *DIM2, int *DIM3) {(float** data_out, int *dim_out1, int *dim_out2, int *dim_out3)};
%apply (float** UAL_OUT_ARRAY4, int *DIM1, int *DIM2, int *DIM3, int *DIM4) {(float** data_out, int *dim_out1, int *dim_out2, int *dim_out3, int *dim_out4)};
%apply (float** UAL_OUT_ARRAY5, int *DIM1, int *DIM2, int *DIM3, int *DIM4, int *DIM5) {(float** data_out, int *dim_out1, int *dim_out2, int *dim_out3, int *dim_out4, int *dim_out5)};
%apply (float** UAL_OUT_ARRAY6, int *DIM1, int *DIM2, int *DIM3, int *DIM4, int *DIM5, int *DIM6) {(float** data_out, int *dim_out1, int *dim_out2, int *dim_out3, int *dim_out4, int *dim_out5, int *dim_out6)};
%apply (float** UAL_OUT_ARRAY7, int *DIM1, int *DIM2, int *DIM3, int *DIM4, int *DIM5, int *DIM6, int *DIM7) {(float** data_out, int *dim_out1, int *dim_out2, int *dim_out3, int *dim_out4, int *dim_out5, int *dim_out6, int *dim_out7)};
%apply (double** UAL_OUT_ARRAY1, int *DIM1) {(double** data_out, int *dim_out)};
%apply (double** UAL_OUT_ARRAY2, int *DIM1, int *DIM2) {(double** data_out, int *dim_out1, int *dim_out2 )};
%apply (double** UAL_OUT_ARRAY3, int *DIM1, int *DIM2, int *DIM3) {(double** data_out, int *dim_out1, int *dim_out2, int *dim_out3)};
%apply (double** UAL_OUT_ARRAY4, int *DIM1, int *DIM2, int *DIM3, int *DIM4) {(double** data_out, int *dim_out1, int *dim_out2, int *dim_out3, int *dim_out4)};
%apply (double** UAL_OUT_ARRAY5, int *DIM1, int *DIM2, int *DIM3, int *DIM4, int *DIM5) {(double** data_out, int *dim_out1, int *dim_out2, int *dim_out3, int *dim_out4, int *dim_out5)};
%apply (double** UAL_OUT_ARRAY6, int *DIM1, int *DIM2, int *DIM3, int *DIM4, int *DIM5, int *DIM6) {(double** data_out, int *dim_out1, int *dim_out2, int *dim_out3, int *dim_out4, int *dim_out5, int *dim_out6)};
%apply (double** UAL_OUT_ARRAY7, int *DIM1, int *DIM2, int *DIM3, int *DIM4, int *DIM5, int *DIM6, int *DIM7) {(double** data_out, int *dim_out1, int *dim_out2, int *dim_out3, int *dim_out4, int *dim_out5, int *dim_out6, int *dim_out7)};


//Just apply standard numpy typemaps for 1D arrays for the put methods bindings
%apply (int* IN_ARRAY1, int DIM1) {(int* data_in, int dim_in)};
%apply (float* IN_ARRAY1, int DIM1) {(float* data_in, int dim_in)};
%apply (double* IN_ARRAY1, int DIM1) {(double* data_in, int dim_in)};
%apply (int DIM1, double* IN_ARRAY1) {(int samples, double *inTimes)};


//Just custom typemaps for 2D, 3D, 4D, 5D and 6D arrays for the put methods bindings because of transposition
%apply (int* UAL_IN_ARRAY2, int DIM1, int DIM2) {(int* data_in, int dim_in1, int dim_in2 )};
%apply (int* UAL_IN_ARRAY3, int DIM1, int DIM2, int DIM3) {(int* data_in, int dim_in1, int dim_in2, int dim_in3)};
%apply (int* UAL_IN_ARRAY4, int DIM1, int DIM2, int DIM3, int DIM4) {(int* data_in, int dim_in1, int dim_in2, int dim_in3, int dim_in4)};
%apply (int* UAL_IN_ARRAY5, int DIM1, int DIM2, int DIM3, int DIM4, int DIM5) {(int* data_in, int dim_in1, int dim_in2, int dim_in3, int dim_in4, int dim_in5)};
%apply (int* UAL_IN_ARRAY6, int DIM1, int DIM2, int DIM3, int DIM4, int DIM5, int DIM6) {(int* data_in, int dim_in1, int dim_in2, int dim_in3, int dim_in4, int dim_in5, int dim_in6)};
%apply (int* UAL_IN_ARRAY7, int DIM1, int DIM2, int DIM3, int DIM4, int DIM5, int DIM6, int DIM7) {(int* data_in, int dim_in1, int dim_in2, int dim_in3, int dim_in4, int dim_in5, int dim_in6, int dim_in7)};
%apply (float* UAL_IN_ARRAY2, int DIM1, int DIM2) {(float* data_in, int dim_in1, int dim_in2 )};
%apply (float* UAL_IN_ARRAY3, int DIM1, int DIM2, int DIM3) {(float* data_in, int dim_in1, int dim_in2, int dim_in3)};
%apply (float* UAL_IN_ARRAY4, int DIM1, int DIM2, int DIM3, int DIM4) {(float* data_in, int dim_in1, int dim_in2, int dim_in3, int dim_in4)};
%apply (float* UAL_IN_ARRAY5, int DIM1, int DIM2, int DIM3, int DIM4, int DIM5) {(float* data_in, int dim_in1, int dim_in2, int dim_in3, int dim_in4, int dim_in5)};
%apply (float* UAL_IN_ARRAY6, int DIM1, int DIM2, int DIM3, int DIM4, int DIM5, int DIM6) {(float* data_in, int dim_in1, int dim_in2, int dim_in3, int dim_in4, int dim_in5, int dim_in6)};
%apply (float* UAL_IN_ARRAY7, int DIM1, int DIM2, int DIM3, int DIM4, int DIM5, int DIM6, int DIM7) {(float* data_in, int dim_in1, int dim_in2, int dim_in3, int dim_in4, int dim_in5, int dim_in6, int dim_in7)};
%apply (double* UAL_IN_ARRAY2, int DIM1, int DIM2) {(double* data_in, int dim_in1, int dim_in2 )};
%apply (double* UAL_IN_ARRAY3, int DIM1, int DIM2, int DIM3) {(double* data_in, int dim_in1, int dim_in2, int dim_in3)};
%apply (double* UAL_IN_ARRAY4, int DIM1, int DIM2, int DIM3, int DIM4) {(double* data_in, int dim_in1, int dim_in2, int dim_in3, int dim_in4)};
%apply (double* UAL_IN_ARRAY5, int DIM1, int DIM2, int DIM3, int DIM4, int DIM5) {(double* data_in, int dim_in1, int dim_in2, int dim_in3, int dim_in4, int dim_in5)};
%apply (double* UAL_IN_ARRAY6, int DIM1, int DIM2, int DIM3, int DIM4, int DIM5, int DIM6) {(double* data_in, int dim_in1, int dim_in2, int dim_in3, int dim_in4, int dim_in5, int dim_in6)};
%apply (double* UAL_IN_ARRAY7, int DIM1, int DIM2, int DIM3, int DIM4, int DIM5, int DIM6, int DIM7) {(double* data_in, int dim_in1, int dim_in2, int dim_in3, int dim_in4, int dim_in5, int dim_in6, int dim_in7)};





/*********************** Typemaps for string argument of the get method **************/

%typemap(argout) (char **data_out) {
  PyObject* o1, *o2;
  PyObject* my_py_string;
		
  if(result == 0)
    {
      my_py_string = PyString_FromStringAndSize(*$1, strlen(*$1));
      free(*$1);
    }
  else
    my_py_string = PyString_FromString("");
		
  if ((!$result) || ($result == Py_None)) {
    $result = my_py_string;
  } else {
    if (!PyTuple_Check($result)) {
      o1 = $result;
      $result = PyTuple_New(1);
      PyTuple_SetItem($result,0,o1);
    }
    o2 = PyTuple_New(1);
    PyTuple_SetItem(o2,0,my_py_string);
    o1 = $result;
    $result = PySequence_Concat(o1,o2);
    Py_DECREF(o1);
    Py_DECREF(o2);
  }
}

%typemap(in,numinputs=0) char ** data_out (char* temp) {
  $1 = &temp;
}

%typemap(argout) (char *** data_out, int* dim_out) {
  int i;
  char ** str_tab;
  PyObject *str_obj;
  PyObject* o1, *o2;
  PyObject* my_py_string;
		
  if(result == 0)
    {
      str_tab = *$1;
      my_py_string = PyList_New(*$2);
      for(i=0; i < *$2; ++i)
	{
	  str_obj = PyString_FromStringAndSize(str_tab[i], strlen(str_tab[i]));
	  PyList_SET_ITEM(my_py_string, i, str_obj );
	  free(str_tab[i]);
	}
      free(str_tab);
    }
  else
    my_py_string = PyList_New(0);
		
  if ((!$result) || ($result == Py_None)) {
    $result = my_py_string;
  } else {
    if (!PyTuple_Check($result)) {
      o1 = $result;
      $result = PyTuple_New(1);
      PyTuple_SetItem($result,0,o1);
    }
    o2 = PyTuple_New(1);
    PyTuple_SetItem(o2,0,my_py_string);
    o1 = $result;
    $result = PySequence_Concat(o1,o2);
    Py_DECREF(o1);
    Py_DECREF(o2);
  }
}

%typemap(in,numinputs=0) char *** data_out (char** temp) {
  $1 = &temp;
}


/*********************** Typemaps for string argument of the put method **************/
%typemap(in) (char** data_in, int dim_in) {
  int i;
  if (!PySequence_Check($input)) {
    PyErr_SetString(PyExc_ValueError,"Expected a sequence");
    return NULL;
  }
  
  $2 = PySequence_Length($input);
  $1 = malloc($2 * sizeof(char*));
    
  for (i = 0; i < $2; i++) {
    PyObject *o = PySequence_GetItem($input,i);
    if (PyString_Check(o)) {
      $1[i] = (char*) PyString_AsString(o);
    } else {
      PyErr_SetString(PyExc_ValueError,"Sequence elements must be strings");      
      return NULL;
    }
  }

  if ($2==1 && strlen($1[0])==0)
    $2 = 0;
}

%typemap(freearg) (char** data_in, int dim_in)
{
  free($1);
}



%include "ual_low_level_swig.h"

