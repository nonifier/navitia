/* Copyright © 2001-2014, Canal TP and/or its affiliates. All rights reserved.

This file is part of Navitia,
    the software to build cool stuff with public transport.

Hope you'll enjoy and contribute to this project,
    powered by Canal TP (www.canaltp.fr).
Help us simplify mobility and open public transport:
    a non ending quest to the responsive locomotion way of traveling!

LICENCE: This program is free software; you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU Affero General Public License for more details.

You should have received a copy of the GNU Affero General Public License
along with this program. If not, see <http://www.gnu.org/licenses/>.

Stay tuned using
twitter @navitia
IRC #navitia on freenode
https://groups.google.com/d/forum/navitia
www.navitia.io
*/

#pragma once
#include "type/type.pb.h"
#include "type/response.pb.h"
#include "type/request.pb.h"
#include "utils/flat_enum_map.h"
#include "type/rt_level.h"
#include <limits>
#include "raptor.h"
#include "routing/routing.h"

namespace navitia{
    namespace type{
        struct EntryPoint;
        struct AccessibiliteParams;
        class Data;
    }
    namespace georef{
        struct StreetNetwork;
    }
    class time_duration;
    struct PbCreator;
}

namespace navitia {
namespace routing {

struct RAPTOR;

struct NightBusFilter {

    static constexpr double default_max_factor = 3;
    static constexpr int32_t default_base_factor = 3600; /*seconds*/

    struct Params
    {
        DateTime requested_datetime;
        bool clockwise;
        double max_factor;
        int32_t base_factor;
    };
};


void add_direct_path(PbCreator& pb_creator,
                     const georef::Path& path,
                     const type::EntryPoint& origin,
                     const type::EntryPoint& destination,
                     const std::vector<bt::ptime>& datetimes,
                     const bool clockwise);

/**
 * @brief Used for classic Pt request
 */
void make_response(navitia::PbCreator& pb_creator,
                   RAPTOR &raptor,
                   const type::EntryPoint &origin,
                   const type::EntryPoint &destination,
                   const std::vector<uint64_t> &datetimes,
                   const bool clockwise,
                   const type::AccessibiliteParams& accessibilite_params,
                   const std::vector<std::string>& forbidden,
                   const std::vector<std::string>& allowed,
                   georef::StreetNetwork& worker,
                   const type::RTLevel rt_level,
                   const navitia::time_duration& transfer_penalty,
                   const uint32_t max_duration=std::numeric_limits<uint32_t>::max(),
                   const uint32_t max_transfers=std::numeric_limits<uint32_t>::max(),
                   const uint32_t max_extra_second_pass = 0,
                   const uint32_t free_radius_from = 0,
                   const uint32_t free_radius_to = 0,
                   const boost::optional<uint32_t>& min_nb_journeys = boost::none,
                   const double night_bus_filter_max_factor = NightBusFilter::default_max_factor,
                   const int32_t night_bus_filter_base_factor = NightBusFilter::default_base_factor,
                   const boost::optional<uint32_t>& timeframe_duration = boost::none);

void make_isochrone(navitia::PbCreator& pb_creator,
                    RAPTOR &raptor,
                    type::EntryPoint origin,
                    const uint64_t datetime, bool clockwise,
                    const type::AccessibiliteParams & accessibilite_params,
                    std::vector<std::string> forbidden,
                    std::vector<std::string> allowed,
                    georef::StreetNetwork & worker,
                    const type::RTLevel rt_level,
                    int max_duration = 3600,
                    uint32_t max_transfers=std::numeric_limits<uint32_t>::max());

/**
 * @brief Used for Pt with distributed mode
 */
void make_pt_response(navitia::PbCreator& pb_creator,
                      RAPTOR &raptor,
                      const std::vector<type::EntryPoint> &origins,
                      const std::vector<type::EntryPoint> &destinations,
                      const uint64_t timestamp,
                      const bool clockwise,
                      const type::AccessibiliteParams& accessibilite_params,
                      const std::vector<std::string>& forbidden,
                      const std::vector<std::string>& allowed,
                      const type::RTLevel rt_level,
                      const navitia::time_duration& transfer_penalty,
                      const uint32_t max_duration=std::numeric_limits<uint32_t>::max(),
                      const uint32_t max_transfers=std::numeric_limits<uint32_t>::max(),
                      const uint32_t max_extra_second_pass = 0,
                      const boost::optional<navitia::time_duration>& direct_path_duration = boost::none,
                      const boost::optional<uint32_t>& min_nb_journeys = boost::none,
                      const double night_bus_filter_max_factor = NightBusFilter::default_max_factor,
                      const int32_t night_bus_filter_base_factor = NightBusFilter::default_base_factor,
                      const boost::optional<uint32_t>& timeframe_duration = boost::none);


boost::optional<routing::map_stop_point_duration>
get_stop_points(const type::EntryPoint &ep,
                const type::Data& data,
                georef::StreetNetwork & worker,
                const uint32_t free_radius = 0,
                bool use_second = false);

/**
 * @brief Filtering with free radius constraint
 * If SP are inside, we set the SP time duration to 0.
 * It is very useful to bring the starting points back to the same station.
 *
 * @param sp_list The current stop point list to filter
 * @param path_finder The current path finder (arrival or departure)
 * @param ep The starting point (center of the radius)
 * @param data The data struct that contains circle filter method
 * @param free_radius The radius for filtering with circle (meters)
 */
void free_radius_filter(routing::map_stop_point_duration& sp_list,
                        georef::PathFinder& path_finder,
                        const type::EntryPoint& ep,
                        const type::Data& data,
                        const  uint32_t free_radius);

/**
 * @brief Remove direct path
 *
 * @param journeys Raptor Journeys list
 */
void filter_direct_path(RAPTOR::Journeys& journeys);

/**
* @brief Check if a journey is way later than another journey
*/
bool is_way_later(const Journey & j1, const Journey & j2,
               const NightBusFilter::Params & params);

/**
* @brief Remove from the list, the journeys that are
* way later than the best.
*
* @param journeys A container of Journeys
* @param params the night bus filter parameters
*/
void filter_late_journeys(RAPTOR::Journeys & journeys,
                          const NightBusFilter::Params & params);


/**
* @brief Prepare the horizon for the next Raptor call
*
* @param journeys A container of journeys.
* @param clockwise Leave after or Arrive before
*/
DateTime prepare_next_call_for_raptor(const RAPTOR::Journeys& journeys, const bool clockwise);

void make_graphical_isochrone(navitia::PbCreator& pb_creator,
                              RAPTOR &raptor_max,
                              const type::EntryPoint& origin,
                              const uint64_t departure_datetime,
                              const std::vector<DateTime>& boundary_duration,uint32_t max_transfers,
                              const type::AccessibiliteParams& accessibilite_params,
                              const std::vector<std::string>& forbidden,
                              const std::vector<std::string>& allowed,
                              bool clockwise,
                              const nt::RTLevel rt_level,
                              georef::StreetNetwork & worker,
                              const double& speed);

void make_heat_map(navitia::PbCreator& pb_creator,
                   RAPTOR &raptor,
                   const type::EntryPoint& center,
                   const uint64_t departure_datetime,
                   DateTime max_duration,
                   uint32_t max_transfers,
                   const type::AccessibiliteParams& accessibilite_params,
                   const std::vector<std::string>& forbidden,
                   const std::vector<std::string>& allowed,
                   bool clockwise,
                   const nt::RTLevel rt_level,
                   georef::StreetNetwork & worker,
                   const double& speed,
                   const navitia::type::Mode_e mode,
                   const uint32_t resolution);

void make_pathes(PbCreator& pb_creator,
                 const std::vector<navitia::routing::Path>& paths,
                 georef::StreetNetwork& worker,
                 const georef::Path& direct_path,
                 const type::EntryPoint& origin,
                 const type::EntryPoint& destination,
                 const std::vector<bt::ptime>& datetimes,
                 const bool clockwise,
                 const uint32_t free_radius_from = 0,
                 const uint32_t free_radius_to = 0);

/**
 * @brief This function determines the breaking condition that depends on nb of found journeys,
 * min_nb_journeys and time frame.
 *
 * @details
 *
 *   requested_datetime           requested_datetime + timeframe_duration
 *   -------[--------------------------------------------]-------------------------->
 *                  minimal period to explore
 *
 * timeframe_limit (request_date_secs + timeframe_duration):
 *          The end datetime of original request's timeframe (we have to explore at minimum this period)
 *
 *                   min_nb_journeys is set?
 *                    /               \
 *                   / Y               \ N
 *          timeframe is set?        timeframe is set?
 *            /       \                  /     \
 *           / Y       \ N              / Y      \ N
 *       Case 1      Case 2          Case 3     Case 4
 *
 *
 */
bool keep_going(const uint32_t total_nb_journeys,
                const uint32_t nb_try,
                const bool clockwise,
                const DateTime request_date_secs,
                const boost::optional<uint32_t>& min_nb_journeys,
                const boost::optional<DateTime>& timeframe_limit,
                const uint32_t max_transfers);
} // namespace routing
} // namespace navitia
