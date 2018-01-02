/******************************************************************************
 * Copyright 2017 The Apollo Authors. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *****************************************************************************/

/*
 * warm_start_ipopt_interface.cc
 */

#include "modules/planning/planner/open_space/warm_start_ipopt_interface.h"

#include <math.h>
#include <utility>

#include "modules/common/log.h"
#include "modules/common/math/math_utils.h"
#include "modules/planning/common/planning_gflags.h"

namespace apollo {
namespace planning {

constexpr std::size_t N = 80;

WarmUpIPOPTInterface::WarmUpIPOPTInterface(int num_of_variables,
                                           int num_of_constraints, int horizon,
                                           float ts, float wheelbase_length,
                                           Eigen::MatrixXd x0,
                                           Eigen::MatrixXd xf,
                                           Eigen::MatrixXd XYbounds)
    : num_of_variables_(num_of_variables),
      num_of_constraints_(num_of_constraints),
      horizon_(horizon),
      ts_(ts),
      wheelbase_length_(wheelbase_length),
      x0_(x0),
      xf_(xf),
      XYbounds_(XYbounds) {}

void WarmUpIPOPTInterface::get_optimization_results() const {}

bool WarmUpIPOPTInterface::get_nlp_info(int& n, int& m, int& nnz_jac_g,
                                        int& nnz_h_lag,
                                        IndexStyleEnum& index_style) {
  // number of variables
  n = num_of_variables_;

  // number of constraints

  m = num_of_constraints_;

  // number of nonzero hessian and lagrangian.

  // TODO(QiL) : Update nnz_jac_g;
  nnz_jac_g = 0;

  // TOdo(QiL) : Update nnz_h_lag;
  nnz_h_lag = 0;

  index_style = IndexStyleEnum::C_STYLE;
  return true;
}

bool WarmUpIPOPTInterface::get_bounds_info(int n, double* x_l, double* x_u,
                                           int m, double* g_l, double* g_u) {
  // here, the n and m we gave IPOPT in get_nlp_info are passed back to us.
  // If desired, we could assert to make sure they are what we think they are.
  CHECK(n == num_of_variables_) << "num_of_variables_ mismatch, n: " << n
                                << ", num_of_variables_: " << num_of_variables_;
  CHECK(m == num_of_constraints_)
      << "num_of_constraints_ mismatch, n: " << n
      << ", num_of_constraints_: " << num_of_constraints_;

  // Variables: includes u and sample time

  std::size variable_index = 0;
  for (std::size_t i = 0; i < horizon_; ++i) {
    variable_index = i * 3;

    // steer command
    x_l[variable_index] = -0.6;
    x_u[variable_index] = 0.6;

    // acceleration
    x_l[variable_index + 1] = -1;
    x_u[variable_index + 1] = 1;

    // sampling time;
    x_l[variable_index + 2] = 0.5;
    x_u[variable_index + 2] = 2.5;
  }

  ADEBUG << "variable_index : " << variable_index;
  // Constraints

  // 1. state constraints
  // start point pose
  std::size_t constraint_index = 0;
  for (std::size_t i = 0; i < 4; ++i) {
    g_l[i] = g_u[i] = x0_[i];
  }
  constraint_index += 4;

  // During horizons
  for (std::size_t i = 1; i < horizon_ - 1; ++i) {
    // x
    g_l[constraint_index] = XYbounds_[0];
    g_u[constraint_index] = XYbounds_[1];

    // y
    g_l[constraint_index + 1] = XYbounds_[2];
    g_u[constraint_index + 1] = XYbounds_[3];

    // phi
    // TODO(QiL): Change this to configs
    g_l[constraint_index + 2] = -7;
    g_u[constraint_index + 2] = 7;

    // v
    // TODO(QiL) : Change this to configs
    g_l[constraint_index + 3] = -1;
    g_u[constraint_index + 3] = 2;

    constraint_index += 4;
  }

  // end point pose
  for (std::size_t i = 0; i < 4; ++i) {
    g_l[constraint_index + i] = g_u[constraint_index + i] = xf_[i];
  }
  constraint_index += 4;
  ADEBUG << "constraint_index after adding state constraints : "
         << constraint_index;

  // 2. input constraints
  for (std::size_t i = 1; i < horizon_; ++i) {
    // u1
    g_l[constraint_index] = -0.6;
    g_u[constraint_index] = 0.6;

    // u2
    g_l[constraint_index + 1] = -1;
    g_u[constraint_index + 1] = 1;

    constraint_index += 2;
  }
  ADEBUG << "constraint_index after adding input constraints : "
         << constraint_index;

  // 3. sampling time constraints
  for (std::size_t i = 1; i < horizon_; ++i) {
    g_l[constraint_index] = -0.6;
    g_u[constraint_index] = 0.6;

    ++constraint_index;
  }
  ADEBUG << "constraint_index after adding sampling time constraints : "
         << constraint_index;

  return true;
}

bool WarmUpIPOPTInterface::eval_g(int n, const double* x, bool new_x, int m,
                                  double* g) {
  return true;
}

bool WarmUpIPOPTInterface::eval_jac_g(int n, const double* x, bool new_x, int m,
                                      int nele_jac, int* iRow, int* jCol,
                                      double* values) {
  return true;
}

bool WarmUpIPOPTInterface::eval_h(int n, const double* x, bool new_x,
                                  double obj_factor, int m,
                                  const double* lambda, bool new_lambda,
                                  int nele_hess, int* iRow, int* jCol,
                                  double* values) {
  return true;
}

void WarmUpIPOPTInterface::set_start_point() {}

void WarmUpIPOPTInterface::set_end_point() {}

}  // namespace planning
}  // namespace apollo