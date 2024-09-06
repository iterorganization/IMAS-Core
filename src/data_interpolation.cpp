#include "data_interpolation.h"

#include "al_const.h"
#include "al_exception.h"

#include <math.h>
#include <limits>
#include "assert.h"

DataInterpolation::DataInterpolation()
{
}

void DataInterpolation::getTimeRangeIndices(double tmin, double tmax, std::vector<double> dtime, const std::vector<double> &time_vector,
                                            int *time_min_index, int *time_max_index, int *range, int interp)
{
    //printf("getTimeRangeIndices::tmin=%f, tmax=%f, interp=%d\n", tmin, tmax, interp);
    if (dtime.size() == 0)
    {

        *time_min_index = -1;
        *time_max_index = -1;

        for (size_t i = 0; i < time_vector.size(); i++)
        {
            if (time_vector[i] >= tmin)
            {
                *time_min_index = i;
                break;
            }
        }
        //printf("time_vector.size()=%d\n", time_vector.size());

        if (*time_min_index == -1)
        {
            return;
        }

        const double default_value = -9.e40;

        for (size_t i = time_vector.size() - 1; i >= 0; i--)
        {

            if (time_vector[i] != default_value && time_vector[i] <= tmax)
            {
                //printf("found i = %d\n", i);
                *time_max_index = i;
                break;
            }
        }

        if (*time_max_index == -1)
        {
            return;
        }

        *range = *time_max_index - *time_min_index + 1;
    }
    else
    {
        std::map<std::string, int> times_indices;
        *time_max_index = getSlicesTimesIndices(tmax, time_vector, times_indices, interp);
        *time_min_index = getSlicesTimesIndices(tmin, time_vector, times_indices, interp);
        /*printf("*time_max_index=%d, *time_min_index=%d\n", *time_max_index, *time_min_index);
        printf("range=%d\n", *time_max_index - *time_min_index + 1);
        printf("tmax=%f, tmin=%f\n", tmax, tmin);*/
        if (dtime.size() == 1) {
            *range = round((tmax - tmin) / dtime[0] + 1);
        }
        else {
            size_t max_index = dtime.size() - 1;
            size_t min_index = 0;
            for (int i = 0; i < dtime.size(); i++) {
                if (dtime[i] <= tmin) 
                     min_index = i;
                if (dtime[i] >= tmax) 
                     max_index = i;
            }
            *range = max_index - min_index + 1;
        }
    }
}

int DataInterpolation::getSlicesTimesIndices(double requested_time, const std::vector<double> &time_vector, std::map<std::string, int> &times_indices, int interp)
{

    //printf("DataInterpolation::getSlicesTimesIndices::interp=%d\n", interp);
    if ((interp != alconst::undefined_interp) && (interp != CLOSEST_INTERP && interp != PREVIOUS_INTERP && interp != LINEAR_INTERP))
    {
        throw ALBackendException("Interpolation mode not supported", LOG);
    }

    int slice_sup = time_vector.size() - 1;
    int slice_inf = 0;
    if (slice_sup > 0)
        slice_inf = slice_sup - 1;

    int closest;

    for (size_t i = 0; i < time_vector.size(); i++)
    {
        if (time_vector[i] >= requested_time)
        {
            slice_sup = i;
            if (interp != CLOSEST_INTERP)
            {
                if (slice_sup == 0 && time_vector.size() > 1)
                    slice_sup = 1;
            }
            if (slice_sup > 0)
                slice_inf = slice_sup - 1;
            break;
        }
    }
    //printf("DataInterpolation::getSlicesTimesIndices::slice_inf=%d\n", slice_inf);
    //printf("DataInterpolation::getSlicesTimesIndices::slice_sup=%d\n", slice_sup);
    times_indices[SLICE_INF] = slice_inf;
    times_indices[SLICE_SUP] = slice_sup;

    /*printf("requested_time=%f\n", requested_time);
    for (int i = 0; i < time_vector.size(); i++) {
        printf("time_vector[%d]=%f\n", i, time_vector[i]);
    }*/

    if (interp == alconst::undefined_interp)
        interp = CLOSEST_INTERP;

    switch (interp)
    {

    case CLOSEST_INTERP:
    {
        if (fabs(requested_time - time_vector[slice_sup]) <= (fabs(requested_time - time_vector[slice_inf])))
        {
            closest = slice_sup;
        }
        else
        {
            closest = slice_inf;
        }
        //printf("DataInterpolation::CLOSEST_INTERP::getSlicesTimesIndices::slice_inf=%d\n", slice_inf);
        //printf("DataInterpolation::CLOSEST_INTERP::getSlicesTimesIndices::slice_sup=%d\n", slice_sup);
        return closest;
        break;
    }

    case PREVIOUS_INTERP:
    {
        closest = slice_inf;
        if (slice_sup > 0 && ((fabs(requested_time - time_vector[slice_sup]) < std::numeric_limits<double>::epsilon()) || (requested_time > time_vector[slice_sup])))
            closest = slice_sup;
        return closest;
        break;
    }

    case LINEAR_INTERP:
    {
        return slice_inf;
        break;
    }
    }
    return -1;
}

void DataInterpolation::interpolate(int datatype, int shape, std::map<std::string, void *> &y_slices,
                                    std::map<std::string, double> &slices_times, double requested_time, void **result, int interp)
{

    //printf("DataInterpolation::interpolate:: performing data interpolation at requested_time=%f, interp=%d\n", requested_time, interp);

    if (shape == 0)
    {
        throw ALBackendException("Unable to perform interpolation with shape=0.", LOG);
    }

    if (interp != LINEAR_INTERP)
    {

        switch (datatype)
        {

        case alconst::integer_data:
        {
            int *data_int = (int *)y_slices[SLICE_INF];
            *result = data_int;
            break;
        }

        case alconst::double_data:
        {
            double *data_double = (double *)y_slices[SLICE_INF];
            *result = data_double;
            break;
        }

        case alconst::char_data:
        {
            char *data_str = (char *)y_slices[SLICE_INF];
            *result = data_str;
            break;
        }
        }
        return;
    }

    if ((slices_times.size() != 2) || (y_slices.size() != 2))
        throw ALBackendException("Exactly 2 slices are required for LINEAR_INTERP interpolation mode.", LOG);

    double time_slice_inf = slices_times[SLICE_INF];
    double time_slice_sup = slices_times[SLICE_SUP];

    double interpolation_factor = 0.;

    if (time_slice_sup != time_slice_inf && (requested_time >= time_slice_inf) && (requested_time <= time_slice_sup))
    {
        interpolation_factor = (requested_time - time_slice_inf) / (time_slice_sup - time_slice_inf);
    }

    if (requested_time < time_slice_inf)
        interpolation_factor = 0.;

    else if (requested_time > time_slice_sup)
        interpolation_factor = 1.;

    //printf("interpolation_factor=%f, time_slice_inf=%f, time_slice_sup=%f\n", interpolation_factor, time_slice_inf, time_slice_sup);

    //Making linear interpolation
    switch (datatype)
    {

    case alconst::integer_data:
    {
        int *data_int = (int *)y_slices[SLICE_INF];
        int *next_slice_data_int = (int *)y_slices[SLICE_SUP];
        if (interpolation_factor != 0)
        {
            for (size_t i = 0; i < shape; i++)
                data_int[i] = (int)round(data_int[i] + (next_slice_data_int[i] - data_int[i]) * interpolation_factor);
        }
        *result = (void *)data_int;
        break;
    }

    case alconst::double_data:
    {
        double *data_double = (double *)y_slices[SLICE_INF];
        double *next_slice_data_double = (double *)y_slices[SLICE_SUP];
        /*for (size_t i = 0; i < shape; i++) {
            printf("dd[%d] = %f\n", i, data_double[i]);
            printf("next dd[%d] = %f\n", i, next_slice_data_double[i]);
         }*/
        if (interpolation_factor != 0)
        {
            //printf("shape = %d\n", shape);
            for (size_t i = 0; i < shape; i++)
                data_double[i] = data_double[i] + (next_slice_data_double[i] - data_double[i]) * interpolation_factor;
        }
        /*for (size_t i = 0; i < shape; i++) {
            printf("result[%d] = %f\n", i, data_double[i]);
         }*/
        *result = (void *)data_double;
        break;
    }

    case alconst::char_data:
    {
        char *data_str = (char *)y_slices[SLICE_INF];
        char *next_data_str = (char *)y_slices[SLICE_SUP];
        if (strcmp(data_str, next_data_str) != 0)
        {
            strcpy(data_str, ""); //returning an empty string since values from neighboring slices are different
        }
        char* p = (char*) *result;
        p = data_str;
        break;
    }
    }
}

int DataInterpolation::resample_timebasis(double tmin, double tmax, std::vector<double> dtime, int timed_AOS_index, std::vector<double> &time_basis, void **result)
{
    //printf("resample_timebasis::timed_AOS_index=%d\n", timed_AOS_index);
    assert(dtime.size() != 0); //this method is used only for resampling a time basis, so dtime should be defined
    //free(data);
    int nb_slices;
    if (timed_AOS_index == -1)
    {
        int time_min_index = -1;
        int time_max_index = -1;
        getTimeRangeIndices(tmin, tmax, dtime, time_basis, &time_min_index, &time_max_index, &nb_slices, alconst::undefined_interp);

        *result = (double *)malloc(nb_slices * sizeof(double));
        double *r = (double *)*result;

        if (dtime.size() == 1) {
            int i = 0;
            double requested_time = tmin;
            for (int i = 0; i < nb_slices; i++)
            {
                r[i] = requested_time;
                requested_time += dtime[0];
            }
        }
        else {
            for (int i = 0; i < nb_slices; i++)
                r[i] = dtime[i];
        }
    }
    else
    {
        nb_slices = 1;
        *result = (double *)malloc(sizeof(double));
        double *r = (double *)*result;
        double requested_time;
        if (dtime.size() == 1) 
            requested_time = tmin + timed_AOS_index * dtime[0];
        else
            requested_time = dtime[timed_AOS_index];
        
        r[0] = requested_time;
    }
    return nb_slices;
}

int DataInterpolation::interpolate_with_resampling(double tmin, double tmax, std::vector<double> dtime, int datatype, int *size, int dim, void *data,
                                                   const std::vector<double> &time_vector, void **result, int interp)
{

    int time_slice_shape = 1;         //shape of a time slice
    for (int i = 0; i < dim - 1; i++) //excluding last dimension (time)
        time_slice_shape *= size[i];  //NOTE: the shape along each AOS is 1
    return this->interpolate_with_resampling(tmin, tmax, dtime, datatype, time_slice_shape, data, time_vector, result, interp);
}

int DataInterpolation::interpolate_with_resampling(double tmin, double tmax, std::vector<double> dtime, int datatype, int time_slice_shape, void *data,
                                                   const std::vector<double> &time_vector, void **result, int interp)
{
    //printf("calling interpolate_with_resampling using interp=%d, time_slice_shape=%d, tmin=%f, tmax=%f, dtime=%f\n", interp, time_slice_shape, tmin, tmax, dtime);
    //data is a pointer to data limited to a time range
    //for (int i = 0; i < time_vector.size(); i++)
    //    printf("DataInterpolation::interpolate_with_resampling::time_vector[%d] = %f\n", i, time_vector[i]);
    assert(dtime.size() != 0);
    int start_index;
    int stop_index;
    int range;
    getTimeRangeIndices(tmin, tmax, dtime, time_vector, &start_index, &stop_index, &range, interp);

    std::map<std::string, int> times_indices;
    int start = getSlicesTimesIndices(tmin, time_vector, times_indices, interp);
    std::vector<void *> interpolation_results;
    std::map<std::string, double> slices_times;

    void *slice1;
    void *slice2;

    switch (datatype)
    {

    case alconst::integer_data:
    {
        slice1 = (int *)malloc(time_slice_shape * sizeof(int));
        if (interp == LINEAR_INTERP)
            slice2 = (int *)malloc(time_slice_shape * sizeof(int));
        break;
    }

    case alconst::double_data:
    {
        slice1 = (double *)malloc(time_slice_shape * sizeof(double));
        if (interp == LINEAR_INTERP)
            slice2 = (double *)malloc(time_slice_shape * sizeof(double));
        break;
    }

    case alconst::char_data:
    {
        slice1 = (char *)malloc(time_slice_shape);
        if (interp == LINEAR_INTERP)
            slice2 = (char *)malloc(time_slice_shape);
        break;
    }
    }

    int nb_slices = 0;
    double requested_time;

    if (dtime.size() == 1)
        requested_time = tmin;
    else 
        requested_time = dtime[0];
    //printf("tmax=%f\n", tmax);
    //printf("--> DataInterpolation::interpolate_with_resampling::starting with requested_time=%f\n", requested_time);

    while (requested_time <= tmax)
    {
        //printf("--> DataInterpolation::interpolate_with_resampling::requested_time=%f\n", requested_time);
        if (tmax > time_vector[time_vector.size() -1]) {
            //printf("time_vector[time_vector.size() -1]=%f\n", time_vector[time_vector.size() -1]);
            break;
        }

        void *interpolation_result;

        int requested_index = getSlicesTimesIndices(requested_time, time_vector, times_indices, interp);

        //printf("--> DataInterpolation::interpolate_with_resampling::requested_index=%d\n", requested_index);

        slices_times[SLICE_INF] = time_vector[requested_index];

        if (requested_index == times_indices[SLICE_SUP])
        {
            slices_times[SLICE_SUP] = slices_times[SLICE_INF];
        }
        else
        {
            slices_times[SLICE_SUP] = time_vector[times_indices[SLICE_SUP]];
        }

        //printf("slices_times[SLICE_INF]=%f, slices_times[SLICE_SUP]=%f\n", slices_times[SLICE_INF], slices_times[SLICE_SUP]);

        std::map<std::string, void *> y_slices;

        switch (datatype)
        {

        case alconst::integer_data:
        {
            int offset = requested_index - start;
            assert(offset >= 0);
            int n = time_slice_shape;
            int *data_int = (int *)data;
            memcpy(slice1, data_int + offset * n, n * sizeof(int));
            y_slices[SLICE_INF] = slice1;

            if (interp == LINEAR_INTERP)
            {
                if (offset < time_vector.size()) {
                    memcpy(slice2, data_int + (offset + 1) * n, n * sizeof(int));
                    y_slices[SLICE_SUP] = slice2;
                }
                else {
                    y_slices[SLICE_SUP] = slice1;
                }
            }
            interpolation_result = (int*) malloc(sizeof(int)*time_slice_shape);
            break;
        }

        case alconst::double_data:
        {
            int offset = requested_index - start;
            assert(offset >= 0);
            int n = time_slice_shape;
            double *data_double = (double *)data;
            memcpy(slice1, data_double + offset * n, n * sizeof(double));
            y_slices[SLICE_INF] = slice1;

            if (interp == LINEAR_INTERP)
            {
                //printf("offset=%d, start_index=%d, stop_index=%d, stop_index - start_index=%d\n", offset, start_index, stop_index, stop_index - start_index);
                if (offset < time_vector.size()) {
                    memcpy(slice2, data_double + (offset + 1)* n, n * sizeof(double));
                    y_slices[SLICE_SUP] = slice2;
                }
                else {
                    y_slices[SLICE_SUP] = slice1;
                }
            }

            interpolation_result = (double*) malloc(sizeof(double)*time_slice_shape);
            break;
        }

        case alconst::char_data:
        {
            int offset = requested_index - start;
            assert(offset >= 0);
            int n = time_slice_shape;
            char *data_str = (char *)data;
            memcpy(slice1, data_str + offset * n, n);

            y_slices[SLICE_INF] = slice1;

            if (interp == LINEAR_INTERP)
            {
                if (offset < time_vector.size()) {
                    memcpy(slice2, data_str + (offset + 1) * n, n);
                    y_slices[SLICE_SUP] = slice2;
                }
                else {
                    y_slices[SLICE_SUP] = slice1;
                }
            }

            interpolation_result = (char*) malloc(sizeof(char)*time_slice_shape);
            break;
        }
        }

        void *interpolation_result_tmp;
        interpolate(datatype, time_slice_shape, y_slices, slices_times, requested_time, &interpolation_result_tmp, interp);

        /*for (size_t j = 0; j < time_slice_shape; j++) {
            double *p = (double*)interpolation_result_tmp;
            printf("interpolation_result[%d] = %f\n", j, p[j]);
         }*/
        memcpy(interpolation_result, interpolation_result_tmp, time_slice_shape*sizeof(double));
        interpolation_results.push_back(interpolation_result);
        nb_slices++;

        if (dtime.size() == 1) {
            requested_time += dtime[0];
        }
        else {
            if (nb_slices < dtime.size())
                requested_time = dtime[nb_slices];
            else
                break;
        }
        
    } //end of while
    free(slice1);
    if (interp == LINEAR_INTERP)
        free(slice2);
    free(data);
    if (datatype == alconst::double_data)
    {

        *result = (void *)malloc(sizeof(double) * interpolation_results.size() * time_slice_shape);
        for (int i = 0; i < interpolation_results.size(); i++)
        {
            double *q = (double *)interpolation_results[i];
            double *r = (double *)*result;
            memcpy(r + i * time_slice_shape, q, time_slice_shape * sizeof(double));
            free(q);
        }
    }
    else if (datatype == alconst::integer_data)
    {

        *result = (void *)malloc(sizeof(int) * interpolation_results.size() * time_slice_shape);
        for (int i = 0; i < interpolation_results.size(); i++)
        {
            int *q = (int *)interpolation_results[i];
            int *r = (int *)*result;
            memcpy(r + i * time_slice_shape, q, time_slice_shape * sizeof(int));
            free(q);
        }
    }
    else if (datatype == alconst::char_data)
    {

        *result = (void *)malloc(sizeof(char) * interpolation_results.size() * time_slice_shape);
        for (int i = 0; i < interpolation_results.size(); i++)
        {
            char *q = (char *)interpolation_results[i];
            char *r = (char *)*result;
            memcpy(r + i * time_slice_shape, q, time_slice_shape * sizeof(char));
            free(q);
        }
    }
    //printf("nb_slices = %d\n", nb_slices);
    return nb_slices;
}
