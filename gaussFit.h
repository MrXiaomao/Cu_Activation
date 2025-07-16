/*
 * @Author: MaoXiaoqing
 * @Date: 2025-04-22 17:20:42
 * @LastEditors: Maoxiaoqing
 * @LastEditTime: 2025-07-16 17:11:22
 * @Description: 请填写简介
 */
#include <stdlib.h>
#include <stdio.h>
#include <gsl/gsl_vector.h>
#include <gsl/gsl_matrix.h>
#include <gsl/gsl_blas.h>
#include <gsl/gsl_multifit_nlinear.h>
#include <gsl/gsl_rng.h>
#include <gsl/gsl_randist.h>

#include <vector>

#define gauss_arr_count_min 10 //高斯拟合数据点数最小值

double gaussian(const double a, const double b, const double c, const double t);

int func_f(const gsl_vector* x, void* params, gsl_vector* f);

int func_df(const gsl_vector* x, void* params, gsl_matrix* J);

int func_fvv(const gsl_vector* x, const gsl_vector* v, void* params, gsl_vector* fvv);

void callback(const size_t iter, void* params, const gsl_multifit_nlinear_workspace* w);

int solve_system(gsl_vector* x, gsl_multifit_nlinear_fdf* fdf, gsl_multifit_nlinear_parameters* params);

int testFun(void);

bool GaussFit(std::vector<double> sx, std::vector<double> sy, int n, double *result);

struct Fitdata_Cheng
{
    double* x;
	double* y;
	size_t n;
};
