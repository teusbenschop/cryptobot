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


#include "simulators.h"
#include "controllers.h"
#include "sql.h"
#include "shared.h"
#include "models.h"
#include "exchanges.h"
#include "cryptopia.h"
#include "io.h"
#include "proxy.h"
#include "bitfinex.h"
#include "kraken.h"
#include "bl3p.h"


float curve_calculate (curve_description parameters, float x)
{
  float y = 0;
  
  x *= parameters.x_multiplier;
  x += parameters.x_addend;
  
  switch (parameters.curve)
  {
    case curve_type::begin: break;
    case curve_type::linear: y = x; break;
    case curve_type::linear_negative: y = 0 - x; break;
    case curve_type::squared: y = x * x; break;
    case curve_type::squared_negative: y = 0 - (x * x); break;
    case curve_type::cubed: y = x * x * x; break;
    case curve_type::cubed_negative: y = 0 - (x * x * x); break;
    case curve_type::binary_logarithm: y = log2 (x); break;
    case curve_type::binary_logarithm_negative: y = 0 - log2 (x); break;
    case curve_type::binary_logarithm_multiplied: y = log2 (x) * x; break;
    case curve_type::binary_logarithm_multiplied_negative: y = 0 - (log2 (x) * x); break;
    case curve_type::end: break;
  }
  
  y *= parameters.y_multiplier;
  y += parameters.y_addend;
  
  return y;
}


string curve_title (curve_type curve)
{
  switch (curve)
  {
    case curve_type::begin: return "";
    case curve_type::linear: return "curve-linear"; break;
    case curve_type::linear_negative: return "curve-linear-negative"; break;
    case curve_type::squared: return "curve-squared"; break;
    case curve_type::squared_negative: return "curve-squared-negative"; break;
    case curve_type::cubed: return "curve-cubed"; break;
    case curve_type::cubed_negative: return "curve-cubed-negative"; break;
    case curve_type::binary_logarithm: return "curve-binary-logarithm"; break;
    case curve_type::binary_logarithm_negative: return "curve-binary-logarithm-negative"; break;
    case curve_type::binary_logarithm_multiplied: return "curve-binary-logarithm-multiplied"; break;
    case curve_type::binary_logarithm_multiplied_negative: return "curve-binary-logarithm-multiplied-negative"; break;
    case curve_type::end: return "";
  }
  return "";
}


float curve_get_fitting_accuracy (vector <float> & observed_xs, vector <float> & observed_ys,
                                  curve_type curve,
                                  float x_multiplier, float x_addend, float y_multiplier, float y_addend,
                                  float & maximum_residual_percentage,
                                  float & average_residual_percentage)
{
  float sum_absolute_residuals = 0;
  maximum_residual_percentage = 0;
  average_residual_percentage = 0;
  
  curve_description parameters;
  parameters.curve = curve;
  parameters.x_multiplier = x_multiplier;
  parameters.x_addend = x_addend;
  parameters.y_multiplier = y_multiplier;
  parameters.y_addend = y_addend;
  
  // At the end of the rates, that is, around the current rates, now,
  // the curve should fit better than at the beginning of the rates.
  // Because if the time progresses to be nearer the current time,
  // the accuracy is important for determining the forecast of the coin rate.
  // The weight given to a residual.
  // A higher weight leads to a better fit.
  float starting_weight = 0.5;
  float weight_multiplier = 4.5 / observed_xs.size ();
  
  for (unsigned int i = 0; i < observed_xs.size(); i++) {
    float observed_x = observed_xs [i];
    float observed_y = observed_ys [i];
    float curve_y = curve_calculate (parameters, observed_x);
    // Conventional wisdom says that the summed square residuals is a good indicator for curve fitting.
    // But on crypto coins, this does not work so well.
    // So here it does not take the square, but just the absolute value.
    float residual = abs (curve_y - observed_y);
    // Apply the weight at this position of the curve.
    float weight = starting_weight + (i * weight_multiplier);
    residual *= weight;
    //to_tty ({to_string (i), "x", float2string (observed_x), "yin", float2string (observed_y), "yout", float2string (curve_y), "residual", float2string (squared_residual)});
    sum_absolute_residuals += residual;
    // The absolute percentage change.
    float absolute_change_percentage = abs (get_percentage_change (observed_y, curve_y));
    if (absolute_change_percentage > maximum_residual_percentage) {
      maximum_residual_percentage = absolute_change_percentage;
    }
    average_residual_percentage += absolute_change_percentage;
  }
  
  average_residual_percentage /= observed_ys.size();
  
  return sum_absolute_residuals;
}


float fitted_curve_get_optimized_residuals (vector <float> xs,
                                            vector <float> ys,
                                            curve_description & parameters)
{
  // Requires at least three observed data points.
  if (xs.size() < 3) return numeric_limits<float>::infinity();
  if (ys.size() < 3) return numeric_limits<float>::infinity();
  
  // Number of points on X-axis and on Y-axis should be the same.
  if (xs.size() != ys.size()) return numeric_limits<float>::infinity();
  
  // Initialize the fitting parameters.
  parameters.x_multiplier = 1;
  parameters.x_addend = xs[0];
  float diff_x = xs.back () - xs[0];
  parameters.y_multiplier = 1;
  parameters.y_addend = ys[0];
  float diff_y = ys.back () - ys[0];
  int iterations = 0;
  int no_convergence_counter = 0;
  
  float dummy;
  
  // Result of the initial curve fitting.
  float sum_residuals = curve_get_fitting_accuracy (xs, ys, parameters.curve, parameters.x_multiplier, parameters.x_addend, parameters.y_multiplier, parameters.y_addend, dummy, dummy);
  
  // Limit the number of iterations for convergence.
  int maximum_iterations = 5000;
  while ((iterations < maximum_iterations) && (no_convergence_counter < 10)) {
    
    bool convergence = false;
    float residue_up, residue_down;
    
    float x_multiplier_up = parameters.x_multiplier * 1.01;
    float x_multiplier_down = parameters.x_multiplier * 0.99;
    residue_up = curve_get_fitting_accuracy (xs, ys, parameters.curve, x_multiplier_up, parameters.x_addend, parameters.y_multiplier, parameters.y_addend, dummy, dummy);
    residue_down = curve_get_fitting_accuracy (xs, ys, parameters.curve, x_multiplier_down, parameters.x_addend, parameters.y_multiplier, parameters.y_addend, dummy, dummy);
    if (residue_up < sum_residuals) {
      parameters.x_multiplier = x_multiplier_up;
      sum_residuals = residue_up;
      convergence = true;
    }
    if (residue_down < sum_residuals) {
      parameters.x_multiplier = x_multiplier_down;
      sum_residuals = residue_down;
      convergence = true;
    }
    
    float x_addend_up = parameters.x_addend + (0.01 * diff_x);
    float x_addend_down = parameters.x_addend - (0.01 * diff_x);
    residue_up = curve_get_fitting_accuracy (xs, ys, parameters.curve, parameters.x_multiplier, x_addend_up, parameters.y_multiplier, parameters.y_addend, dummy, dummy);
    residue_down = curve_get_fitting_accuracy (xs, ys, parameters.curve, parameters.x_multiplier, x_addend_down, parameters.y_multiplier, parameters.y_addend, dummy, dummy);
    if (residue_up < sum_residuals) {
      parameters.x_addend = x_addend_up;
      sum_residuals = residue_up;
      convergence = true;
    }
    if (residue_down < sum_residuals) {
      parameters.x_addend = x_addend_down;
      sum_residuals = residue_down;
      convergence = true;
    }
    
    float y_multiplier_up = parameters.y_multiplier * 1.01;
    float y_multiplier_down = parameters.y_multiplier * 0.99;
    residue_up = curve_get_fitting_accuracy (xs, ys, parameters.curve, parameters.x_multiplier, parameters.x_addend, y_multiplier_up, parameters.y_addend, dummy, dummy);
    residue_down = curve_get_fitting_accuracy (xs, ys, parameters.curve, parameters.x_multiplier, parameters.x_addend, y_multiplier_down, parameters.y_addend, dummy, dummy);
    if (residue_up < sum_residuals) {
      parameters.y_multiplier = y_multiplier_up;
      sum_residuals = residue_up;
      convergence = true;
    }
    if (residue_down < sum_residuals) {
      parameters.y_multiplier = y_multiplier_down;
      sum_residuals = residue_down;
      convergence = true;
    }
    
    float y_addend_up = parameters.y_addend + (0.01 * diff_y);
    float y_addend_down = parameters.y_addend - (0.01 * diff_y);
    residue_up = curve_get_fitting_accuracy (xs, ys, parameters.curve, parameters.x_multiplier, parameters.x_addend, parameters.y_multiplier, y_addend_up, dummy, dummy);
    residue_down = curve_get_fitting_accuracy (xs, ys, parameters.curve, parameters.x_multiplier, parameters.x_addend, parameters.y_multiplier, y_addend_down, dummy, dummy);
    if (residue_up < sum_residuals) {
      parameters.y_addend = y_addend_up;
      sum_residuals = residue_up;
      convergence = true;
    }
    if (residue_down < sum_residuals) {
      parameters.y_addend = y_addend_down;
      sum_residuals = residue_down;
      convergence = true;
    }
    
    iterations++;
    if (convergence) {
      no_convergence_counter = 0;
    } else {
      no_convergence_counter++;
    }
  }
  
  //to_tty ({"iterations", to_string (iterations), "no convergence counter", to_string (no_convergence_counter)});
  
  // If there were too many iterations,
  // it means that a fitting curve was not found.
  if (iterations >= maximum_iterations) {
    return numeric_limits<float>::infinity();
  }
  
  // Final calculation of other fitting quality indicators.
  curve_get_fitting_accuracy (xs, ys, parameters.curve, parameters.x_multiplier, parameters.x_addend, parameters.y_multiplier, parameters.y_addend, parameters.maximum_residual_percentage, parameters.average_residual_percentage);
  
  // Done.
  return sum_residuals;
}


void get_fitted_curve (vector <float> xs, vector <float> ys, curve_description & parameters)
{
  // Initialize default to no fitting curve found.
  parameters.curve = curve_type::end;
  float best_summed_residuals = numeric_limits<double>::infinity();
  parameters.maximum_residual_percentage = numeric_limits<double>::infinity();
  parameters.average_residual_percentage = numeric_limits<double>::infinity();
  
  for (int iter = static_cast <int> (curve_type::begin) + 1; iter < static_cast <int> (curve_type::end); iter++)
  {
    curve_description local_parameters;
    local_parameters.curve = static_cast <curve_type> (iter);
    local_parameters.maximum_residual_percentage = 0;
    local_parameters.average_residual_percentage = 0;
    float residual = fitted_curve_get_optimized_residuals (xs, ys, local_parameters);
    if (residual < best_summed_residuals) {
      parameters.curve = local_parameters.curve;
      parameters.x_multiplier = local_parameters.x_multiplier;
      parameters.x_addend = local_parameters.x_addend;
      parameters.y_multiplier = local_parameters.y_multiplier;
      parameters.y_addend = local_parameters.y_addend;
      best_summed_residuals = residual;
      parameters.maximum_residual_percentage = local_parameters.maximum_residual_percentage;
      parameters.average_residual_percentage = local_parameters.average_residual_percentage;
    }
  }
}


// The number of hours for the rates deep learner.
vector <int> deep_learning_hours ()
{
  return { 2, 5, 12, 24, 2 * 24, 7 * 24, 30 * 24 };
}


// Looks at the curves that capture the rate pattern.
// Takes in account the object with the parameters for the forecast.
// Does a forecast about the new rate of the coin after 24 hours.
// It uses a weights assigned to all the factors.
float rate_24h_forecast (void * output,
                         forecast_properties forecast_parameters,
                         forecast_weights weights)
{
  to_output * out = (to_output *) output;
  
  float overall_24h_forecast = 0;
  
  // Iterate over all the fitting curve parameters.
  vector <curve_description> all_curve_parameters = forecast_parameters.curves;
  for (auto & parameters : all_curve_parameters) {
    
    // The name of the curve. For developer checks.
    string curve_name = curve_title (parameters.curve);
    
    // The last coin's rate that follows from the detected curve.
    float most_recent_smoothed_coin_rate = curve_calculate (parameters, parameters.last_seen_x_value);
    
    // Get the next period, moving forward, for extrapolating the coin's rate.
    // This period will be the same period the curve was fitted to,
    // with a maximum of 24 hours.
    float weight = 0;
    int next_period_x_value = parameters.last_seen_x_value;
    if (parameters.minutes_measured) {
      int minutes = parameters.minutes_measured;
      if (minutes > 60 * 24) minutes = 60 * 24;
      next_period_x_value += minutes;
      weight = weights.hours_curve_weights [minutes / 60];
    }
    if (parameters.hours_measured) {
      int hours = parameters.hours_measured;
      if (hours > 24) hours = 24;
      next_period_x_value += hours;
      weight = weights.hours_curve_weights [hours];
    }
    
    // Extrapolate the coin rate that it is going to be based on the statistical information.
    float next_period_expected_coin_rate = curve_calculate (parameters, next_period_x_value);
    float weighed_rate = weight * next_period_expected_coin_rate;
    
    // The number of minutes or hours.
    int units_count = parameters.minutes_measured + parameters.hours_measured;
    string period_units = parameters.minutes_measured ? "minutes" : "hours";
    
    vector <string> msg = {
      curve_name,
      "most recent smoothed rate", float2string (most_recent_smoothed_coin_rate),
      to_string (units_count), period_units,
      "expected next rate", float2string (next_period_expected_coin_rate),
      "weighed", float2string (weighed_rate),
    };
    //out->add (msg);
    //to_tty (msg);

    // Add the partial new forecast, weighed.
    overall_24h_forecast += weighed_rate;
  }

  // Add the average hourly/daily/weekly changes, weighed.
  {
    float absolute;
    float weighed;
    absolute = forecast_parameters.last_observed_average_hourly_rate + (forecast_parameters.last_observed_average_hourly_rate * forecast_parameters.hourly_change_percentage * 24 / 100);
    weighed = absolute * weights.hourly_change_weight;
    overall_24h_forecast += weighed;
    absolute = forecast_parameters.last_observed_average_hourly_rate + (forecast_parameters.last_observed_average_hourly_rate * forecast_parameters.daily_change_percentage / 100);
    weighed = absolute * weights.daily_change_weight;
    overall_24h_forecast += weighed;
    absolute = forecast_parameters.last_observed_average_hourly_rate + (forecast_parameters.last_observed_average_hourly_rate * forecast_parameters.weekly_change_percentage / 7 / 100);
    weighed = absolute * weights.weekly_change_weight;
    overall_24h_forecast += weighed;
  }
  
  // Divide the total by the number of factors taken in account.
  // This is important because when a fitting curve could not be found,
  // for some reasons, then the factors taken in account is one less than usual.
  // The factors now in use are as follows:
  // * The number of fitted curves.
  // * Three averages: The hourly / daily / weekly changes
  overall_24h_forecast /= (all_curve_parameters.size() + 3);
  
  // Done.
  return overall_24h_forecast;
}


// This does the following:
// 1. Validates last day's 24 hour rate forcast.
// 2. Makes a rate forcast for the next day.
void rate_forecast_validator (const string & exchange, const string & coin,
                              float total_balance, float starting_rate, float forecast_change_percentage,
                              bool update_database, int database_record_id,
                              bool plot_rates)
{
  to_output output (exchange + " " + coin);
  output.to_stderr (true);
  
  SQL sql (front_bot_ip_address ());
  
  // Output the total balance available for this coin.
  float total = total_balance;
  output.add ({"Exchange", exchange, "has a balance of", float2string (total), coin});

  // Storage for the prepared rates.
  unordered_map <int, float> minute_asks, minute_bids, hour_asks, hour_bids;
  // The change in a coin's value is measured against the Euro.
  // Measuring change against the Bitcoin is not always a good indicator of
  // how much the value of the coin changes.
  // The reason is that the rate of the Bitcoin itself fluctuates a lot too.
  // Thus the Bitcoin is no longer considered the standard to measure everything against.
  // Rather the Euro is now taken instead.
  // This is more realistic.
  // It is expected to lead to better and more stable profitability.
  // The number of hours passed to the preparer is more than enough to cover all curves below.
  string market = euro_id ();
  // Prepare the rates for looking at the patterns.
  pattern_prepare (&output, exchange, market, coin, 10 * 24,
                   minute_asks, minute_bids, hour_asks, hour_bids);
  
  // Get the current rate.
  // This is the average over the most recent hour.
  int last_hour = 0;
  float current_bid = hour_bids[last_hour];
  float current_ask = hour_asks[last_hour];
  float current_rate = (current_bid + current_ask) / 2;
  
  // Output and store information about the accuracy of the previous forecast.
  {
    float previous_rate = starting_rate;
    float realized_change = get_percentage_change (previous_rate, current_rate);
    output.add ({
      "Previous rate was", float2string (previous_rate), euro_id (),
      "forecast change was", float2visual (forecast_change_percentage), "%",
      "current rate is", float2string (current_rate), market,
      "realized change is", float2string (realized_change), "%"
    });
    if (update_database) optimizer_update (sql, database_record_id, current_rate);
  }

  // Read the data that the deep learner has stored:
  // * The weights assigned to the factors for forecasting next day's rate.
  // * The forecast accuracy indicators.
  forecast_weights weights;
  float sum_absolute_residuals, maximum_residual_percentage, average_residual_percentage;
  string weights_json;
  weights_get (sql, exchange, market, coin, weights_json,
               sum_absolute_residuals, maximum_residual_percentage, average_residual_percentage);
  
  // If no JSON is given, it means that the deep learner has not yet processed this coin.
  if (weights_json.empty ()) {
    output.add ({"No forecast could be made because the deep learner has not yet processed this coin"});
    return;
  }

  // Decode the JSON for getting the weights of the forecast factors.
  weights_decode (weights_json,
                  weights.hourly_change_weight,
                  weights.daily_change_weight,
                  weights.weekly_change_weight,
                  weights.hours_curve_weights);

  // Container for the properties for making a rate forcast.
  forecast_properties forecast_parameters;
  // Add the last observed hour and its average rate to the properties.
  forecast_parameters.last_observed_hour = last_hour;
  forecast_parameters.last_observed_average_hourly_rate = current_rate;
  {
    // Add the change percentages.
    float percents;
    percents = pattern_change (minute_asks, minute_bids, 0, 60);
    if (isnan (percents)) percents = 0;
    if (isinf (percents)) percents = 0;
    forecast_parameters.hourly_change_percentage = percents;
    percents = pattern_change (hour_asks, hour_bids, 0, 24);
    if (isnan (percents)) percents = 0;
    if (isinf (percents)) percents = 0;
    forecast_parameters.daily_change_percentage = percents;
    percents = pattern_change (hour_asks, hour_bids, 0, 24 * 7);
    if (isnan (percents)) percents = 0;
    if (isinf (percents)) percents = 0;
    forecast_parameters.weekly_change_percentage = percents;
    // Output the change percentages.
    output.add ({
      "Current rate changes:",
      "hourly", float2visual (forecast_parameters.hourly_change_percentage), "%",
      "daily", float2visual (forecast_parameters.daily_change_percentage), "%",
      "weekly", float2visual (forecast_parameters.weekly_change_percentage), "%",
    });
  }
  
  output.add ({
    "Deep learner forecast residuals:",
    "maximum", float2visual (maximum_residual_percentage), "%",
    "average", float2visual (average_residual_percentage), "%",
  });

  // Go over various amounts of hours to study the changes in the rates.
  for (int hours : deep_learning_hours () ) {
    curve_description parameters;
    vector <float> xs;
    vector <float> ys;
    if (hours > 15) {
      for (int hour = 0 - hours; hour <= 0; hour++) {
        float ask = hour_asks [hour];
        float bid = hour_bids [hour];
        float rate = (ask + bid) / 2;
        if (rate > 0) {
          xs.push_back (hour);
          ys.push_back (rate);
          // Set the curve parameter.
          parameters.last_seen_x_value = hour;
        }
      }
      // Set the curve parameter.
      parameters.hours_measured = hours;
    } else {
      for (int minute = 0 - hours * 60; minute <= 0; minute++) {
        float ask = minute_asks [minute];
        float bid = minute_bids [minute];
        float rate = (ask + bid) / 2;
        if (rate > 0) {
          xs.push_back (minute);
          ys.push_back (rate);
          // Set the curve parameter.
          parameters.last_seen_x_value = minute;
        }
      }
      // Set the curve parameter.
      parameters.minutes_measured = hours * 60;
    }
    
    // Capture the rates curve in a mathematical formula.
    get_fitted_curve (xs, ys, parameters);
    
    // Plot the observed rates and the fitted curve in one graph.
    // Having one graph is easier for visual inspection of the curve fitting accuracy.
    if (plot_rates) {
      vector <pair <float, float> > plotdata1;
      vector <pair <float, float> > plotdata2;
      for (unsigned int i = 0; i < xs.size(); i++) {
        float x = xs[i];
        float observed_y = ys[i];
        float statistical_y = curve_calculate (parameters, x);
        plotdata1.push_back (make_pair (x, observed_y));
        plotdata2.push_back (make_pair (x, statistical_y));
        
      }
      string title1 = "rates-" + exchange + "-" + euro_id () + "-" + coin + "-" + to_string (hours);
      string title2 = "statistical";
      string picturepath;
      plot (plotdata1, plotdata2, title1, title2, picturepath);
    }
    
    // No formula found: Skip this coin.
    if (parameters.curve == curve_type::end) continue;
    
    // Store the parameters for this curve.
    forecast_parameters.curves.push_back (parameters);
    
    // Output and feedback.
    //output.add ({to_string (hours), "hours", "residual change max", float2visual (parameters.maximum_residual_percentage), "%", "avg", float2visual (parameters.average_residual_percentage), "%"});
  }
  
  // Run the change probability analyzer based on all input gathered.
  float forecast = rate_24h_forecast (&output, forecast_parameters, weights);
  output.add ({"Expected 24-hour forecast is", float2visual (forecast), market});
  
  // Record the new forecast in the database.
  if (update_database) {
    optimizer_add (sql, exchange, coin, current_rate, forecast);
  }
}


void learn_rates_process_observations (void * output,
                                       const string & exchange, const string & market, const string & coin,
                                       unordered_map <int, float> & minute_asks,
                                       unordered_map <int, float> & minute_bids,
                                       unordered_map <int, float> & hour_asks,
                                       unordered_map <int, float> & hour_bids,
                                       vector <forecast_properties> & all_forecast_properties,
                                       forecast_weights & weights)
{
  to_output * out = (to_output *) output;

  // Initialize the overall weights for all factors taken in account for making a forecast.
  for (int hours : deep_learning_hours () ) {
    weights.hours_curve_weights [hours] = 1.0;
  }

  // It will run simulations for the last 30 days.
  // One simulation covers a period of 30 days too.
  // It needs the realized average rate one day after the last day in the simulation.
  // So it won't run the simulation on the most recent 24 hours,
  // because the realized rate is not yet known for those hours.
  for (int last_simulation_hour = 0 - (30 * 24); last_simulation_hour <= 0 - 24; last_simulation_hour++) {

    // Simulation over a span of 30 days.
    int first_simulation_hour = last_simulation_hour - 30 * 24;
    vector <string> msg = {"Simulate from", to_string (first_simulation_hour), "to", to_string (last_simulation_hour), "hours"};
    //out->add (msg);
    to_tty (msg);
    
    // Get the average rate over the last simulation hour.
    float last_simulation_hour_rate = 0;
    {
      float bid = hour_bids [last_simulation_hour];
      float ask = hour_asks [last_simulation_hour];
      last_simulation_hour_rate = (bid + ask) / 2;
    }
    // Get the avarage rate for 24 hours later.
    float next_day_observed_rate = 0;
    {
      float bid = hour_bids [last_simulation_hour + 24];
      float ask = hour_asks [last_simulation_hour + 24];
      next_day_observed_rate = (bid + ask) / 2;
    }
    // Feedback.
    msg = {"Rate at last hour is", float2string (last_simulation_hour_rate), market, "next day is", float2string (next_day_observed_rate)};
    //out->add (msg);
    //to_tty (msg);

    // Container for the properties for making a rate forcast.
    forecast_properties forecast_parameters;
    forecast_parameters.last_observed_hour = last_simulation_hour;
    forecast_parameters.last_observed_average_hourly_rate = last_simulation_hour_rate;
    forecast_parameters.next_day_observed_average_hourly_rate = next_day_observed_rate;
    {
      // Get the change percentages.
      forecast_parameters.hourly_change_percentage = pattern_change (minute_asks, minute_bids, last_simulation_hour * 60, 60);
      if (isnan (forecast_parameters.hourly_change_percentage)) forecast_parameters.hourly_change_percentage = 0;
      if (isinf (forecast_parameters.hourly_change_percentage)) forecast_parameters.hourly_change_percentage = 0;
      forecast_parameters.daily_change_percentage = pattern_change (hour_asks, hour_bids, last_simulation_hour, 24);
      if (isnan (forecast_parameters.daily_change_percentage)) forecast_parameters.daily_change_percentage = 0;
      if (isinf (forecast_parameters.daily_change_percentage)) forecast_parameters.daily_change_percentage = 0;
      forecast_parameters.weekly_change_percentage = pattern_change (hour_asks, hour_bids, last_simulation_hour, 24 * 7);
      if (isnan (forecast_parameters.weekly_change_percentage)) forecast_parameters.weekly_change_percentage = 0;
      if (isinf (forecast_parameters.weekly_change_percentage)) forecast_parameters.weekly_change_percentage = 0;
      // Output the change percentages.
      /*
      out->add ({
        "Changes:",
        "hourly", float2visual (forecast_parameters.hourly_change_percentage), "%",
        "daily", float2visual (forecast_parameters.daily_change_percentage), "%",
        "weekly", float2visual (forecast_parameters.weekly_change_percentage), "%",
      });
       */
    }
    
    // Create a chart to check that the change percentages are correct.
    if (false) {
      vector <pair <float, float> > plotdata;
      for (int hour = first_simulation_hour; hour < last_simulation_hour; hour++) {
        float x = hour;
        float bid = hour_bids [hour];
        float ask = hour_asks [hour];
        float rate = (bid + ask) / 2;
        plotdata.push_back (make_pair (x, rate));
      }
      string title = "rates-" + exchange + "-" + market + "-" + coin + "-" + to_string (first_simulation_hour) + "-" + to_string (last_simulation_hour);
      string picturepath;
      plot (plotdata, {}, title, "", picturepath);
    }
    
    // Go over various amounts of hours to study the changes in the rates.
    for (int hours : deep_learning_hours () ) {
      
      curve_description parameters;
      vector <float> xs;
      vector <float> ys;
      
      if (hours > 15) {
        for (int hour = last_simulation_hour - hours; hour <= last_simulation_hour; hour++) {
          float ask = hour_asks [hour];
          float bid = hour_bids [hour];
          float rate = (ask + bid) / 2;
          if (rate > 0) {
            xs.push_back (hour);
            ys.push_back (rate);
            // Set the curve parameter.
            parameters.last_seen_x_value = hour;
          }
        }
        // Set the curve parameter.
        parameters.hours_measured = hours;
      } else {
        for (int minute = (last_simulation_hour - hours) * 60; minute <= last_simulation_hour * 60; minute++) {
          float ask = minute_asks [minute];
          float bid = minute_bids [minute];
          float rate = (ask + bid) / 2;
          if (rate > 0) {
            xs.push_back (minute);
            ys.push_back (rate);
            // Set the curve parameter.
            parameters.last_seen_x_value = minute;
          }
        }
        // Set the curve parameter.
        parameters.minutes_measured = hours * 60;
      }
      
      // Capture the rates curve in a mathematical formula.
      get_fitted_curve (xs, ys, parameters);
      
      // Plot the observed rates and the fitted curve in one graph.
      // Having one graph is easier for visual inspection of the curve fitting accuracy.
      if (false) {
        vector <pair <float, float> > plotdata1;
        vector <pair <float, float> > plotdata2;
        for (unsigned int i = 0; i < xs.size(); i++) {
          float x = xs[i];
          float observed_y = ys[i];
          float statistical_y = curve_calculate (parameters, x);
          plotdata1.push_back (make_pair (x, observed_y));
          plotdata2.push_back (make_pair (x, statistical_y));
        }
        string title1 = "rates-" + exchange + "-" + market + "-" + coin + "-" + to_string (first_simulation_hour) + "-" + to_string (last_simulation_hour) + "-" + to_string (hours);
        string title2 = "statistical";
        string picturepath;
        plot (plotdata1, plotdata2, title1, title2, picturepath);
      }
      
      // No formula found: Skip this coin.
      if (parameters.curve == curve_type::end) continue;
      
      // Store the parameters for this curve.
      forecast_parameters.curves.push_back (parameters);
      
      // Output and feedback.
      msg = {
        to_string (hours), "hours",
        "residual change max", float2visual (parameters.maximum_residual_percentage), "%",
        "avg", float2visual (parameters.average_residual_percentage), "%"
      };
      //out->add (msg);
      //to_tty (msg);
    }
    
    // Run the rate forecaster based on all input gathered.
    /*
    float forecast = rate_24h_forecast (output, forecast_parameters, weights);
    msg = {"Expected 24-hour forecast is", float2string (forecast)};
    out->add (msg);
    to_tty (msg);
    */
    
    // Add the forecast properties of this simulated hour to the general container.
    all_forecast_properties.push_back (forecast_parameters);
  }
}


void get_forecast_residuals (void * output,
                             vector <forecast_properties> & all_forecast_properties,
                             forecast_weights & weights,
                             float & sum_absolute_residuals,
                             float & maximum_residual_percentage,
                             float & average_residual_percentage)
{
  to_output * out = (to_output *) output;

  sum_absolute_residuals = 0;
  maximum_residual_percentage = 0;
  average_residual_percentage = 0;

  // At the end of the rates, that is, around the current rates, now,
  // the accuracy should be better than at the beginning of the rates.
  // Because if the time progresses to be nearer the current time,
  // the accuracy is important for determining the forecast of the coin rate.
  // The weight given to a residual.
  // A higher weight leads to a better fit.
  float starting_weight = 0.5;
  float weight_multiplier = 4.5 / all_forecast_properties.size ();
  
  for (unsigned int i = 0; i < all_forecast_properties.size(); i++) {
    
    float forecast = rate_24h_forecast (output, all_forecast_properties[i], weights);
    float observed = all_forecast_properties[i].next_day_observed_average_hourly_rate;

    // Conventional wisdom says that the summed square residuals is a good indicator for forecasting.
    // But on crypto coins, this does not work so well.
    // So here it does not take the square, but just the absolute value.
    float residual = abs (forecast - observed);
    // Apply the weight at this position of the simulator.
    float weight = starting_weight + (i * weight_multiplier);
    residual *= weight;
    vector <string> msg = {
      "hour", to_string (all_forecast_properties[i].last_observed_hour),
      "observed", float2string (observed),
      "forecast", float2string (forecast),
      "residual", float2string (residual),
    };
    //out->add (msg);
    //to_tty (msg);
    sum_absolute_residuals += residual;
    // The absolute percentage change.
    float absolute_change_percentage = abs (get_percentage_change (observed, forecast));
    if (absolute_change_percentage > maximum_residual_percentage) {
      maximum_residual_percentage = absolute_change_percentage;
    }
    average_residual_percentage += absolute_change_percentage;
  }
  
  average_residual_percentage /= all_forecast_properties.size();
}


// Iterate over the rates learning data.
// Optimize the weights to get the best forecasts.
float optimize_forecast_weights (void * output,
                                 vector <forecast_properties> & all_forecast_properties,
                                 forecast_weights & weights)
{
  to_output * out = (to_output *) output;

  // Requires at least three data points.
  if (all_forecast_properties.size() < 3) return numeric_limits<float>::infinity();

  // The object with the weights is assumed to have been initialized already.
  // Initialize the fitting parameters.
  int iterations = 0;
  int no_convergence_counter = 0;
  float sum_absolute_residuals;
  float dummy;
  
  // Result of the initial curve fitting.
  get_forecast_residuals (output, all_forecast_properties, weights, sum_absolute_residuals, dummy, dummy);
  
  // Limit the number of iterations for finding maximum convergence.
  int maximum_iterations = 5000;
  while ((iterations < maximum_iterations) && (no_convergence_counter < 10)) {

    // Variables for this convergence iteration.
    bool convergence = false;
    float weight_saved, weight_up, weight_down, residue_up, residue_down;
    
    // Optimize the weight for the hourly change.
    weight_saved = weights.hourly_change_weight;
    weight_up = weight_saved * 1.01;
    weights.hourly_change_weight = weight_up;
    get_forecast_residuals (output, all_forecast_properties, weights, residue_up, dummy, dummy);
    weight_down = weight_saved * 0.99;
    weights.hourly_change_weight = weight_down;
    get_forecast_residuals (output, all_forecast_properties, weights, residue_down, dummy, dummy);
    if (residue_up < sum_absolute_residuals) {
      weights.hourly_change_weight = weight_up;
      sum_absolute_residuals = residue_up;
      convergence = true;
    }
    else if (residue_down < sum_absolute_residuals) {
      weights.hourly_change_weight = weight_down;
      sum_absolute_residuals = residue_down;
      convergence = true;
    }
    else {
      weights.hourly_change_weight = weight_saved;
    }

    // Optimize the weight for the daily change.
    weight_saved = weights.daily_change_weight;
    weight_up = weight_saved * 1.01;
    weights.daily_change_weight = weight_up;
    get_forecast_residuals (output, all_forecast_properties, weights, residue_up, dummy, dummy);
    weight_down = weight_saved * 0.99;
    weights.daily_change_weight = weight_down;
    get_forecast_residuals (output, all_forecast_properties, weights, residue_down, dummy, dummy);
    if (residue_up < sum_absolute_residuals) {
      weights.daily_change_weight = weight_up;
      sum_absolute_residuals = residue_up;
      convergence = true;
    }
    else if (residue_down < sum_absolute_residuals) {
      weights.daily_change_weight = weight_down;
      sum_absolute_residuals = residue_down;
      convergence = true;
    }
    else {
      weights.daily_change_weight = weight_saved;
    }

    // Optimize the weight for the weekly change.
    weight_saved = weights.weekly_change_weight;
    weight_up = weight_saved * 1.01;
    weights.weekly_change_weight = weight_up;
    get_forecast_residuals (output, all_forecast_properties, weights, residue_up, dummy, dummy);
    weight_down = weight_saved * 0.99;
    weights.weekly_change_weight = weight_down;
    get_forecast_residuals (output, all_forecast_properties, weights, residue_down, dummy, dummy);
    if (residue_up < sum_absolute_residuals) {
      weights.weekly_change_weight = weight_up;
      sum_absolute_residuals = residue_up;
      convergence = true;
    }
    else if (residue_down < sum_absolute_residuals) {
      weights.weekly_change_weight = weight_down;
      sum_absolute_residuals = residue_down;
      convergence = true;
    }
    else {
      weights.weekly_change_weight = weight_saved;
    }

    // Iterate over the hours for simulation to optimize all their weights.
    for (int hours : deep_learning_hours () ) {
      weight_saved = weights.hours_curve_weights [hours];
      weight_up = weight_saved * 1.01;
      weights.hours_curve_weights [hours] = weight_up;
      get_forecast_residuals (output, all_forecast_properties, weights, residue_up, dummy, dummy);
      weight_down = weight_saved * 0.99;
      weights.hours_curve_weights [hours] = weight_down;
      get_forecast_residuals (output, all_forecast_properties, weights, residue_down, dummy, dummy);
      if (residue_up < sum_absolute_residuals) {
        weights.hours_curve_weights [hours] = weight_up;
        sum_absolute_residuals = residue_up;
        convergence = true;
      }
      else if (residue_down < sum_absolute_residuals) {
        weights.hours_curve_weights [hours] = weight_down;
        sum_absolute_residuals = residue_down;
        convergence = true;
      }
      else {
        weights.hours_curve_weights [hours] = weight_saved;
      }
    }

    // Update the counters.
    iterations++;
    if (convergence) {
      no_convergence_counter = 0;
    } else {
      no_convergence_counter++;
    }

  }
  
  out->add ({"Iterations:", to_string (iterations), "no convergence counter:", to_string (no_convergence_counter)});
  
  // If there were too many iterations,
  // it means that a fitting curve was not found.
  if (iterations >= maximum_iterations) {
    return numeric_limits<float>::infinity();
  }
  
  // Done.
  return sum_absolute_residuals;
}
