//-*-c++-*-

#ifndef DATA_INTERPOLATION_H
#define DATA_INTERPOLATION_H 1

#include <vector>
#include <map>
#include <string>
#include "al_context.h"

#define SLICE_INF "slice_inf"
#define SLICE_SUP "slice_sup"

#if defined(_WIN32)
#  define IMAS_CORE_LIBRARY_API __declspec(dllexport)
#else
#  define IMAS_CORE_LIBRARY_API
#endif

#ifdef __cplusplus


/**
   DataInterpolation class.
*/
class IMAS_CORE_LIBRARY_API DataInterpolation
{
private:
   //int interp;

public:
  
  DataInterpolation();

  ~DataInterpolation() {};

  /* This function returns the time vector samples indices 'time_min_index' and 'time_max_index' according to 
  the time range input values tmin, tmax and the specified 'time_vector'. If dtime != -1, the interpolation method ('interp' argument) must be specified. The 
  number of time samples 'range' between 'time_min_index' and 'time_max_index' is also returned. 
  */
  void getTimeRangeIndices(double tmin, double tmax, std::vector<double> dtime, const std::vector<double> &time_vector, 
   int *time_min_index, int *time_max_index, int *range, int interp=-1);

  /* This function returns the index of the time vector sample corresponding to the requested time ('requested_time') using the interpolation method 'interp'.
  The 'time_indices' map returns the indices time_indices['SLICE_INF'] and time_indices['SLICE_SUP'] corresponding respectively to the min and
  max values of the time range which contains the time vector sample index returned by this function. In case of linear interpolation, the index returned is the 
  smallest one (time_indices['SLICE_INF']).
  */
  int getSlicesTimesIndices(double requested_time, const std::vector<double> &time_vector, std::map<std::string, int> &times_indices, int interp);

  /* This function interpolates (at time 'requested_time') the data of the 2 time slices provided in the map 'y_slices' at the keys 'SLICE_INF' and 'SLICE_SUP' 
  using the interpolation method 'interp'. The times of the 2 slices are provided by slices_times['SLICE_INF'] and slices_times['SLICE_SUP']. The shape of the time slices is
  passed using the 'shape' argument (number of data points to be interpolated). The interpolation result 
  is returned in the *result pointer. No resampling is performed, so this function must not be used with dtime != -1. */
  void interpolate(int datatype, int shape, std::map<std::string, void*> &y_slices, 
  std::map<std::string, double> &slices_times, double requested_time, void **result, int interp);

  /* This function interpolates (at time 'requested_time') and resamples the data of the 2 time slices provided in the map 'y_slices' at the keys 'SLICE_INF' and 'SLICE_SUP' 
  using the interpolation method 'interp'. The times of the 2 slices are provided by slices_times['SLICE_INF'] and slices_times['SLICE_SUP']. The interpolation result 
  is returned in the *result pointer. If resampling is not required (dtime=-1), the interpolation method should be used instead.
  The new number of time slices is returned.*/
  int interpolate_with_resampling(double tmin, double tmax, std::vector<double> dtime, int datatype, int *size, int dim, void *data, 
    const std::vector<double> &time_vector, void **result, int interp);

  /* Same functionality as the previous function; however the (full) shape of a time slice is passed directly.*/
  int interpolate_with_resampling(double tmin, double tmax, std::vector<double> dtime, int datatype, int shape, void *data, 
    const std::vector<double> &time_vector, void **result, int interp);

/* This function resamples the data of a time basis vector according to the time range parameters.
The parameter dtime must be > 0 (otherwise an assertion occurs). 
If the time basis is located in a dynamic AOS, the value of its current index must be passed using the 'timed_AOS_index' argument. 
If the time basis is located in a static AOS, the 'timed_AOS_index' argument must be set to -1. 
The resampling data are returned in the *result pointer.
The new number of time slices is returned.*/
int resample_timebasis(double tmin, double tmax, std::vector<double> dtime, int timed_AOS_index, std::vector<double> &time_basis, void **result);

};

#endif

#endif // DATA_INTERPOLATION_H
