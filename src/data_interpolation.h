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
#  define LIBRARY_API __declspec(dllexport)
#else
#  define LIBRARY_API
#endif

#ifdef __cplusplus


/**
   DataInterpolation class.
*/
class LIBRARY_API DataInterpolation
{
private:
   //int interp;

public:
  
  DataInterpolation();

  ~DataInterpolation() {};

  void getTimeRangeIndices(double tmin, double tmax, double dtime, const std::vector<double> &time_vector, 
   int *time_min_index, int *time_max_index, int *range, int interp=-1);

  int getSlicesTimesIndices(double requested_time, const std::vector<double> &time_vector, 
   std::map<std::string, int> &times_indices, int interp);

  void interpolate(const std::string &field_path, int datatype, std::map<std::string, void*> &y_slices, 
  std::map<std::string, double> &slices_times, double requested_time, void **result, int interp, int shape = 0);

  void interpolate_with_resampling(OperationContext *opctx, const std::string &field_path, int datatype, void *data, 
    int *size, int dim, const std::vector<double> &time_vector, void **result, int interp);

void resample_timebasis(OperationContext *opctx, int timed_AOS_index, int datatype, void *data, int *size, int dim, void **result);


};

#endif

#endif // DATA_INTERPOLATION_H
