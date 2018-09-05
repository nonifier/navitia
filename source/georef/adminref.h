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
#include "type/type.h"

#include <boost/geometry/geometries/polygon.hpp>
#include <unordered_map>
#include <RTree/RTree.h>

namespace nt = navitia::type;

namespace navitia {

    namespace georef {
        typedef boost::geometry::model::polygon<navitia::type::GeographicalCoord> polygon_type;
        typedef boost::geometry::model::multi_polygon<polygon_type> multi_polygon_type;

        struct Admin : nt::Header, nt::Nameable {
            const static type::Type_e type = type::Type_e::Admin;
            /**
              http://wiki.openstreetmap.org/wiki/Key:admin_level#admin_level
              Level = 2  : Pays
              Level = 4  : Région
              Level = 6  : Département
              Level = 8  : Commune
              Level = 10 : Quartier
            */
            int level;

            // Is the admin came from the original dataset (and not
            // from another source)
            bool from_original_dataset = true;

            std::string insee;
            std::string label;
            std::string comment;

            nt::GeographicalCoord coord;
            multi_polygon_type boundary;
            std::vector<const Admin*> admin_list;
            std::vector<const nt::StopArea*> main_stop_areas;

            // TODO ODT NTFSv0.3: remove that when we stop to support NTFSv0.1
            std::vector<const nt::StopPoint*> odt_stop_points; // zone odt stop points for the admin
            std::vector<std::string> postal_codes;

            Admin():level(-1){}
            Admin(int lev):level(lev){}
            std::string get_range_postal_codes();
            std::string postal_codes_to_string() const;
            template<class Archive> void serialize(Archive & ar, const unsigned int ) {
                ar & idx & level & from_original_dataset & insee
                        & name & uri & coord & admin_list & main_stop_areas & label & odt_stop_points & postal_codes;
            }
        };

        using AdminRtree = RTree<Admin*, double, 2>;
        AdminRtree build_admins_tree(const std::vector<Admin*> admins);
    }
}
