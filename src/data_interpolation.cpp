#include "data_interpolation.h"

#include "al_const.h"
#include "al_exception.h"

#include <math.h>
#include <limits>
#include "assert.h"

DataInterpolation::DataInterpolation()
{
}

void DataInterpolation::getTimeRangeIndices(double tmin, double tmax, double dtime, const std::vector<double> &time_vector,
                                            int *time_min_index, int *time_max_index, int *range, int interp)
{

    if (dtime == -1)
    {

        *time_min_index = -1;
        for (size_t i = 0; i < time_vector.size(); i++)
        {
            if (time_vector[i] >= tmin)
            {
                *time_min_index = i;
                break;
            }
        }

        if (*time_min_index == -1)
        {
            char message[200];
            double max_tb_value = time_vector[time_vector.size() - 1];
            sprintf(message, "Time range: specified time min value (%f) exceeds the largest value of the time basis vector: %f.", tmin, max_tb_value);
            throw ALBackendException(message, LOG);
        }

        *time_max_index = -1;
        for (size_t i = time_vector.size() - 1; i >= 0; i--)
        {
            if (time_vector[i] <= tmax)
            {
                *time_max_index = i;
                break;
            }
        }

        if (*time_max_index == -1)
        {
            char message[200];
            double min_tb_value = time_vector[0];
            sprintf(message, "Time range: specified time max value (%f) lower than the min value of the time basis vector: %f.", tmax, min_tb_value);
            throw ALBackendException(message, LOG);
        }

        *range = *time_max_index - *time_min_index + 1;
    }
    else
    {
        std::map<std::string, int> times_indices;
        *time_max_index = getSlicesTimesIndices(tmax, time_vector, times_indices, interp);
        *time_min_index = getSlicesTimesIndices(tmin, time_vector, times_indices, interp);
        *range = round((tmax - tmin) / dtime + 1);
    }
}

int DataInterpolation::getSlicesTimesIndices(double requested_time, const std::vector<double> &time_vector, std::map<std::string, int> &times_indices, int interp)
{

    //printf("DataInterpolation::interp=%d\n", interp);
    if (interp == alconst::undefined_interp || (interp != CLOSEST_INTERP && interp != PREVIOUS_INTERP && interp != LINEAR_INTERP))
    {
        throw ALBackendException("Interpolation mode not set or not supported", LOG);
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
        if (slice_sup == slice_inf)
        {
            return slice_inf;
        }
        else if (requested_time > time_vector[slice_sup])
        {
            return slice_sup;
        }
        else if (requested_time < time_vector[slice_inf])
        {
            return slice_inf;
        }

        bool same_slices = (slice_sup == slice_inf);
        if (!same_slices)
        {
            if (std::abs(requested_time - time_vector[slice_inf]) <= std::abs(requested_time - time_vector[slice_sup]))
                return slice_inf;
            else
                return slice_sup;
        }
        else
        {
            return slice_inf;
        }
        break;
    }
    }
    return -1;
}

void DataInterpolation::interpolate(int datatype, int shape, std::map<std::string, void *> &y_slices,
                                    std::map<std::string, double> &slices_times, double requested_time, void **result, int interp)
{

    printf("DataInterpolation::interpolate:: performing data interpolation at requested_time=%f\n", requested_time);

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
            *result = malloc(shape * sizeof(int));
            memcpy(*result, data_int, shape * sizeof(int));
            break;
        }

        case alconst::double_data:
        {
            double *data_double = (double *)y_slices[SLICE_INF];
            *result = malloc(shape * sizeof(double));
            memcpy(*result, data_double, shape * sizeof(double));
            break;
        }

        case alconst::char_data:
        {
            char *data_str = (char *)y_slices[SLICE_INF];
            *result = malloc(shape * sizeof(char));
            memcpy(*result, data_str, shape * sizeof(char));
            break;
        }
        }
        return;
    }

    if ((slices_times.size() != 2) || (y_slices.size() != 2))
        throw ALBackendException("Exactly 2 slices are required for LINEAR_INTERP interpolation mode.", LOG);

    double time_slice_inf = slices_times[SLICE_INF];
    double time_slice_sup = slices_times[SLICE_SUP];

    bool same_slices = fabs(time_slice_sup - time_slice_inf) < std::numeric_limits<double>::epsilon();

    double interpolation_factor = 0.;

    if ((requested_time >= time_slice_inf) && (requested_time <= time_slice_sup))
    {
        if (!same_slices)
            interpolation_factor = (requested_time - time_slice_inf) / (time_slice_sup - time_slice_inf);
    }

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
        /*if (!same_slices)
            free(next_slice_data_int);*/
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
            for (size_t i = 0; i < shape; i++)
                data_double[i] = data_double[i] + (next_slice_data_double[i] - data_double[i]) * interpolation_factor;
        }
        /*if (!same_slices)
            free(next_slice_data_double);*/
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
        /*if (!same_slices)
            free(next_data_str);*/

        *result = (void *)data_str;
        break;
    }
    }
}

int DataInterpolation::resample_timebasis(double tmin, double tmax, double dtime, int timed_AOS_index, void *data, void **result)
{

    //printf("resample_timebasis::timed_AOS_index=%d\n", timed_AOS_index);
    assert(dtime != -1); //this method is used only for resampling a time basis, so dtime should be defined
    free(data);
    int nb_slices;
    if (timed_AOS_index == -1)
    {
        nb_slices = round((tmax - tmin) / dtime + 1);
        *result = (double *)malloc(nb_slices * sizeof(double));
        double *r = (double *)*result;
        int i = 0;
        double requested_time = tmin;
        for (int i = 0; i < nb_slices; i++)
        {
            r[i] = requested_time;
            requested_time += dtime;
        }
    }
    else
    {
        nb_slices = 1;
        *result = (double *)malloc(sizeof(double));
        double *r = (double *)*result;
        double requested_time = tmin + timed_AOS_index * dtime;
        r[0] = requested_time;
    }
    return nb_slices;
}

int DataInterpolation::interpolate_with_resampling(double tmin, double tmax, double dtime, int datatype, int *size, int dim, void *data,
                                                   const std::vector<double> &time_vector, void **result, int interp)
{

    int time_slice_shape = 1;         //shape of a time slice
    for (int i = 0; i < dim - 1; i++) //excluding last dimension (time)
        time_slice_shape *= size[i];  //NOTE: the shape along each AOS is 1
    return this->interpolate_with_resampling(tmin, tmax, dtime, datatype, time_slice_shape, data, time_vector, result, interp);
}

int DataInterpolation::interpolate_with_resampling(double tmin, double tmax, double dtime, int datatype, int time_slice_shape, void *data,
                                                   const std::vector<double> &time_vector, void **result, int interp)
{
    //printf("calling interpolate_with_resampling...\n");
    //data is a pointer to data limited to a time range

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
    double requested_time = tmin;
    //printf("tmax=%f\n", tmax);


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
        bool same_slices = (times_indices[SLICE_INF] == times_indices[SLICE_SUP]);

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
                if (!same_slices && offset < (stop_index - start_index) ) {
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
                if (!same_slices && offset < (stop_index - start_index) ) {
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
                if (!same_slices && offset < (stop_index - start_index) ) {
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
        requested_time += dtime;
        nb_slices++;
    }
    /*free(slice1);
    if (interp == LINEAR_INTERP)
        free(slice2);*/
    free(data);
    if (datatype == alconst::double_data)
    {

        *result = (void *)malloc(sizeof(double) * interpolation_results.size() * time_slice_shape);
        for (int i = 0; i < interpolation_results.size(); i++)
        {
            double *q = (double *)interpolation_results[i];
            double *r = (double *)*result;
            /*for (size_t j = 0; j < time_slice_shape; j++) {
            printf("q[%d] = %f\n", j, q[j]);
            //printf("next dd[%d] = %f\n", i, next_slice_data_double[i]);
            }*/
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
    return nb_slices;
}
