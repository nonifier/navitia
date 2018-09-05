/* Copyright © 2001-2018, Canal TP and/or its affiliates. All rights reserved.

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

#include <memory>
#include <map>

#include <boost/optional.hpp>
#include <boost/utility.hpp>

#include "type/type.pb.h"

#if ENABLE_PROMETHEUS
#include <prometheus/exposer.h>
#include <prometheus/counter.h>

//forward declare
namespace prometheus{
class Registry;
class Counter;
class Histogram;
}
#endif

namespace navitia {
class Metrics: boost::noncopyable {

// This stub is for retro compatibility
#if ENABLE_PROMETHEUS
    protected:
        std::unique_ptr<prometheus::Exposer> exposer;
        std::shared_ptr<prometheus::Registry> registry;
        std::map<pbnavitia::API, prometheus::Histogram*> request_histogram;
    public:
        Metrics(const boost::optional<std::string>& endpoint, const std::string& coverage);
        void observe_api(pbnavitia::API api, double duration) const;
#else
    public:
        Metrics(const boost::optional<std::string>&, const std::string&) {}
        void observe_api(pbnavitia::API, double) const {}
#endif
};

}
