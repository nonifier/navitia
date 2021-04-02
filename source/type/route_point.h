/* Copyright © 2001-2021, Canal TP and/or its affiliates. All rights reserved.

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
channel `#navitia` on riot https://riot.im/app/#/room/#navitia:matrix.org
https://groups.google.com/d/forum/navitia
www.navitia.io
*/

#pragma once

#include "type/fwd_type.h"
#include "type/stop_point.h"
#include "utils/idx_map.h"

#include <boost/range/any_range.hpp>

#include <utility>

namespace navitia {
namespace type {

struct RoutePoint {
    idx_t idx;
    StopPoint* stop_point;
    Route* route;
};

using RoutePointRefs = std::vector<std::reference_wrapper<type::RoutePoint>>;
using StopPointRange = boost::any_range<StopPoint, boost::forward_traversal_tag, StopPoint&, std::ptrdiff_t>;

RoutePointRefs route_points_from(const StopPointRange& sps);
RoutePointRefs route_points_from(const std::vector<StopPoint*>& sps);

}  // namespace type
}  // namespace navitia
