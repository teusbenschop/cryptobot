/*
 Copyright (©) 2013-2017 Teus Benschop.
 
 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 3 of the License, or
 (at your option) any later version.
 
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */


#include "libraries.h"


#ifndef INCLUDED_SIMULATORS_H
#define INCLUDED_SIMULATORS_H


enum class curve_type {
  begin,
  linear,
  linear_negative,
  squared,
  squared_negative,
  cubed,
  cubed_negative,
  binary_logarithm,
  binary_logarithm_negative,
  binary_logarithm_multiplied,
  binary_logarithm_multiplied_negative,
  end
};

class curve_description
{
public:
  int last_seen_x_value = 0;
  int minutes_measured = 0;
  int hours_measured = 0;
  curve_type curve = curve_type::end;
  float x_multiplier = 1;
  float x_addend = 0;
  float y_multiplier = 1;
  float y_addend = 0;
  float maximum_residual_percentage = 0;
  float average_residual_percentage = 0;
};

float curve_calculate (curve_description parameters, float x);
string curve_title (curve_type curve);
float curve_get_fitting_accuracy (vector <float> & observed_xs, vector <float> & observed_ys,
                                  curve_type curve,
                                  float x_multiplier, float x_addend, float y_multiplier, float y_addend,
                                  float & maximum_residual_percentage,
                                  float & average_residual_percentage);
float fitted_curve_get_optimized_residuals (vector <float> xs,
                                            vector <float> ys,
                                            curve_description & parameters);
void get_fitted_curve (vector <float> xs, vector <float> ys, curve_description & parameters);

class forecast_properties
{
public:
  int last_observed_hour = 0;
  float last_observed_average_hourly_rate = 0;
  float next_day_observed_average_hourly_rate = 0;
  float hourly_change_percentage = 0;
  float daily_change_percentage = 0;
  float weekly_change_percentage = 0;
  vector <curve_description> curves;
};

class forecast_weights
{
public:
  float hourly_change_weight = 1.0;
  float daily_change_weight = 1.0;
  float weekly_change_weight = 1.0;
  unordered_map <int, float> hours_curve_weights;
};

vector <int> deep_learning_hours ();
float rate_24h_forecast (void * output,
                         forecast_properties forecast_parameters,
                         forecast_weights weights);
void rate_forecast_validator (const string & exchange, const string & coin,
                              float total_balance, float starting_rate, float forecast_change_percentage,
                              bool update_database, int database_record_id,
                              bool plot_rates);

void learn_rates_process_observations (void * output,
                                       const string & exchange, const string & market, const string & coin,
                                       unordered_map <int, float> & minute_asks,
                                       unordered_map <int, float> & minute_bids,
                                       unordered_map <int, float> & hour_asks,
                                       unordered_map <int, float> & hour_bids,
                                       vector <forecast_properties> & all_forecast_properties,
                                       forecast_weights & weights);
void get_forecast_residuals (void * output,
                             vector <forecast_properties> & all_forecast_properties,
                             forecast_weights & weights,
                             float & sum_absolute_residuals,
                             float & maximum_residual_percentage,
                             float & average_residual_percentage);
float optimize_forecast_weights (void * output,
                                 vector <forecast_properties> & all_forecast_properties,
                                 forecast_weights & weights);


#endif


