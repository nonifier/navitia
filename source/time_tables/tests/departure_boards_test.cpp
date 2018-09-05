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

#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE test_ed
#include <boost/test/unit_test.hpp>
#include "ed/build_helper.h"
#include "type/type.h"
#include "tests/utils_test.h"
#include "departure_board_test_data.h"
#include "routing/raptor.h"
#include "kraken/apply_disruption.h"
#include "kraken/make_disruption_from_chaos.h"

struct logger_initialized {
    logger_initialized()   { init_logger(); }
};
BOOST_GLOBAL_FIXTURE( logger_initialized );

static int32_t time_to_int(int h, int m, int s) {
    auto dur = navitia::time_duration(h, m, s);
    return dur.total_seconds(); //time are always number of seconds from midnight
}

using namespace navitia::timetables;

static boost::gregorian::date date(std::string str) {
    return boost::gregorian::from_undelimited_string(str);
}

//for more concice test
static pt::ptime d(std::string str) {
    return boost::posix_time::from_iso_string(str);
}

BOOST_AUTO_TEST_CASE(departureboard_test1) {
    ed::builder b("20150615");
    b.vj("A", "110011000001", "", true, "vj1", "")("stop1", 10*3600, 10*3600)("stop2", 10*3600 + 30*60,10*3600 + 30*60);
    b.vj("B", "110000001111", "", true, "vj2", "")("stop1", 10*3600 + 10*60, 10*3600 + 10*60)("stop2", 10*3600 + 40*60,10*3600 + 40*60)("stop3", 10*3600 + 50*60,10*3600 + 50*60);

    const auto it1 = b.sas.find("stop2");
    b.data->pt_data->routes.front()->destination= it1->second; // Route A
    const auto it2 = b.sas.find("stop3");
    b.data->pt_data->routes.back()->destination= it2->second; // Route B
    b.finish();
    b.data->pt_data->sort_and_index();
    b.data->build_raptor();
    b.data->pt_data->build_uri();

    boost::gregorian::date begin = boost::gregorian::date_from_iso_string("20150615");
    boost::gregorian::date end = boost::gregorian::date_from_iso_string("20150630");

    b.data->meta->production_date = boost::gregorian::date_period(begin, end);
    pbnavitia::Response resp;
    // normal departure board
    {
    auto * data_ptr = b.data.get();
    navitia::PbCreator pb_creator(data_ptr, bt::second_clock::universal_time(), null_time_period);
    departure_board(pb_creator, "stop_point.uri=stop1", {}, {}, d("20150615T094500"), 43200, 0,
                    10, 0, nt::RTLevel::Base, std::numeric_limits<size_t>::max());

    resp = pb_creator.get_response();
    BOOST_REQUIRE_EQUAL(resp.stop_schedules_size(), 2);
    BOOST_REQUIRE_EQUAL(resp.stop_schedules(0).date_times_size(),1);
    BOOST_REQUIRE_EQUAL(resp.stop_schedules(1).date_times_size(),1);
    }
    // no departure for route "A"
    {
    auto * data_ptr = b.data.get();
    navitia::PbCreator pb_creator(data_ptr, bt::second_clock::universal_time(), null_time_period);
    departure_board(pb_creator, "stop_point.uri=stop1", {}, {}, d("20150616T094500"), 43200, 0,
                    10, 0, nt::RTLevel::Base, std::numeric_limits<size_t>::max());
    resp = pb_creator.get_response();
    BOOST_REQUIRE_EQUAL(resp.stop_schedules_size(), 2);
    BOOST_REQUIRE_EQUAL(resp.stop_schedules(0).date_times_size(),0);
    BOOST_REQUIRE_EQUAL(resp.stop_schedules(0).route().name(), "A");
    BOOST_CHECK_EQUAL(resp.stop_schedules(0).response_status(), pbnavitia::ResponseStatus::no_departure_this_day);
    BOOST_REQUIRE_EQUAL(resp.stop_schedules(1).route().name(), "B");
    BOOST_REQUIRE_EQUAL(resp.stop_schedules(1).date_times_size(),1);
    }
    // no departure for all routes
    {
    auto * data_ptr = b.data.get();
    navitia::PbCreator pb_creator(data_ptr, bt::second_clock::universal_time(), null_time_period);
    departure_board(pb_creator, "stop_point.uri=stop1", {}, {}, d("20150619T094500"), 43200, 0,
                    10, 0, nt::RTLevel::Base, std::numeric_limits<size_t>::max());
    resp = pb_creator.get_response();
    BOOST_REQUIRE_EQUAL(resp.stop_schedules_size(), 2);
    BOOST_REQUIRE_EQUAL(resp.stop_schedules(0).date_times_size(),0);
    BOOST_REQUIRE_EQUAL(resp.stop_schedules(0).route().name(), "A");
    BOOST_CHECK_EQUAL(resp.stop_schedules(0).response_status(), pbnavitia::ResponseStatus::no_departure_this_day);
    BOOST_REQUIRE_EQUAL(resp.stop_schedules(1).route().name(), "B");
    BOOST_REQUIRE_EQUAL(resp.stop_schedules(1).date_times_size(),0);
    BOOST_CHECK_EQUAL(resp.stop_schedules(1).response_status(), pbnavitia::ResponseStatus::no_departure_this_day);
    }
    // no departure for route "B"
    {
    auto * data_ptr = b.data.get();
    navitia::PbCreator pb_creator(data_ptr, bt::second_clock::universal_time(), null_time_period);
    departure_board(pb_creator, "stop_point.uri=stop1", {}, {}, d("20150621T094500"), 43200, 0,
                    10, 0, nt::RTLevel::Base, std::numeric_limits<size_t>::max());
    resp = pb_creator.get_response();
    BOOST_REQUIRE_EQUAL(resp.stop_schedules_size(), 2);
    BOOST_REQUIRE_EQUAL(resp.stop_schedules(0).date_times_size(),1);
    BOOST_REQUIRE_EQUAL(resp.stop_schedules(0).route().name(), "A");
    BOOST_REQUIRE_EQUAL(resp.stop_schedules(1).route().name(), "B");
    BOOST_CHECK_EQUAL(resp.stop_schedules(1).response_status(), pbnavitia::ResponseStatus::no_departure_this_day);
    }
    // Terminus for route "A"
    {
    auto * data_ptr = b.data.get();
    navitia::PbCreator pb_creator(data_ptr, bt::second_clock::universal_time(), null_time_period);
    departure_board(pb_creator, "stop_point.uri=stop2", {}, {}, d("20150615T094500"), 43200, 0,
                    10, 0, nt::RTLevel::Base, std::numeric_limits<size_t>::max());
    resp = pb_creator.get_response();
    BOOST_REQUIRE_EQUAL(resp.stop_schedules_size(), 2);
    BOOST_REQUIRE_EQUAL(resp.stop_schedules(0).route().name(), "A");
    BOOST_CHECK_EQUAL(resp.stop_schedules(0).response_status(), pbnavitia::ResponseStatus::terminus);
    BOOST_REQUIRE_EQUAL(resp.stop_schedules(1).route().name(), "B");
    BOOST_CHECK_EQUAL(resp.stop_schedules(1).date_times_size(), 1);
    }
    // Terminus for route "B"
    {
    auto * data_ptr = b.data.get();
    navitia::PbCreator pb_creator(data_ptr, bt::second_clock::universal_time(), null_time_period);
    departure_board(pb_creator, "stop_point.uri=stop3", {}, {}, d("20150615T094500"), 43200, 0,
                    10, 0, nt::RTLevel::Base, std::numeric_limits<size_t>::max());
    resp = pb_creator.get_response();
    BOOST_REQUIRE_EQUAL(resp.stop_schedules_size(), 1);
    BOOST_REQUIRE_EQUAL(resp.stop_schedules(0).route().name(), "B");
    BOOST_CHECK_EQUAL(resp.stop_schedules(0).response_status(), pbnavitia::ResponseStatus::terminus);
    }
    {
    auto * data_ptr = b.data.get();
    navitia::PbCreator pb_creator(data_ptr, bt::second_clock::universal_time(), null_time_period);
    departure_board(pb_creator, "stop_point.uri=stop2", {}, {}, d("20120701T094500"), 86400, 0,
                    10, 0, nt::RTLevel::Base, std::numeric_limits<size_t>::max());
    resp = pb_creator.get_response();
    BOOST_REQUIRE_EQUAL(resp.error().id(), pbnavitia::Error::date_out_of_bounds);
    }

    // normal departure board with impact on stop_point
    {
    using btp = boost::posix_time::time_period;
    b.impact(nt::RTLevel::Adapted, "Disruption 1")
            .severity(nt::disruption::Effect::UNKNOWN_EFFECT)
            .on(nt::Type_e::StopPoint, "stop1")
            .application_periods(btp("20150615T010000"_dt, "20150625T235900"_dt))
            .publish(btp("20150615T010000"_dt, "20150625T235900"_dt))
            .msg("Disruption on stop_point stop1");

    auto * data_ptr = b.data.get();
    navitia::PbCreator pb_creator(data_ptr, "20150616T080000"_dt, null_time_period);
    departure_board(pb_creator, "stop_point.uri=stop1", {}, {}, d("20150615T094500"), 43200, 0,
                    10, 0, nt::RTLevel::Base, std::numeric_limits<size_t>::max());

    resp = pb_creator.get_response();
    BOOST_REQUIRE_EQUAL(resp.stop_schedules_size(), 2);
    BOOST_REQUIRE_EQUAL(resp.stop_schedules(0).date_times_size(),1);
    BOOST_REQUIRE_EQUAL(resp.stop_schedules(1).date_times_size(),1);
    BOOST_REQUIRE_EQUAL(resp.stop_schedules(1).stop_point().uri(),"stop1");
    BOOST_REQUIRE_EQUAL(resp.stop_schedules(1).stop_point().impact_uris_size(),1);
    const auto* impact = navitia::test::get_impact(resp.stop_schedules(1).stop_point().impact_uris(0), resp);
    BOOST_REQUIRE(impact);
    BOOST_REQUIRE_EQUAL(impact->messages_size(),1);
    BOOST_REQUIRE_EQUAL(impact->messages(0).text(), "Disruption on stop_point stop1");

    //current_datetime out of bounds
    pb_creator.init(data_ptr, "20150626T110000"_dt, null_time_period);
    pb_creator.now = "20150626T110000"_dt;
    departure_board(pb_creator, "stop_point.uri=stop1", {}, {}, d("20150615T094500"), 43200, 0,
                    10, 0, nt::RTLevel::Base, std::numeric_limits<size_t>::max());

    resp = pb_creator.get_response();
    BOOST_REQUIRE_EQUAL(resp.stop_schedules_size(), 2);
    BOOST_REQUIRE_EQUAL(resp.stop_schedules(0).date_times_size(),1);
    BOOST_REQUIRE_EQUAL(resp.stop_schedules(1).date_times_size(),1);
    BOOST_REQUIRE_EQUAL(resp.stop_schedules(1).stop_point().uri(),"stop1");
    BOOST_REQUIRE_EQUAL(resp.stop_schedules(1).stop_point().impact_uris_size(),0);
    }
}

BOOST_AUTO_TEST_CASE(departureboard_test_with_impacts) {
    ed::builder b("20150615");
    b.vj("A", "110011000001", "", true, "vj1", "")("stop1", 10*3600, 10*3600)("stop2", 10*3600 + 30*60,10*3600 + 30*60);
    b.vj("B", "110000001111", "", true, "vj2", "")("stop1", 10*3600 + 10*60, 10*3600 + 10*60)("stop2", 10*3600 + 40*60,10*3600 + 40*60)("stop3", 10*3600 + 50*60,10*3600 + 50*60);

    const auto it1 = b.sas.find("stop2");
    b.data->pt_data->routes.front()->destination= it1->second; // Route A
    const auto it2 = b.sas.find("stop3");
    b.data->pt_data->routes.back()->destination= it2->second; // Route B
    b.finish();
    b.data->pt_data->sort_and_index();
    b.data->build_raptor();
    b.data->pt_data->build_uri();

    boost::gregorian::date begin = boost::gregorian::date_from_iso_string("20150615");
    boost::gregorian::date end = boost::gregorian::date_from_iso_string("20150630");

    b.data->meta->production_date = boost::gregorian::date_period(begin, end);
    pbnavitia::Response resp;

    // no departure for all routes
    {
    auto * data_ptr = b.data.get();
    navitia::PbCreator pb_creator(data_ptr, bt::second_clock::universal_time(), null_time_period);
    departure_board(pb_creator, "stop_point.uri=stop1", {}, {}, d("20150619T094500"), 43200, 0,
                    10, 0, nt::RTLevel::Base, std::numeric_limits<size_t>::max());
    resp = pb_creator.get_response();
    BOOST_REQUIRE_EQUAL(resp.stop_schedules_size(), 2);
    BOOST_REQUIRE_EQUAL(resp.stop_schedules(0).date_times_size(),0);
    BOOST_REQUIRE_EQUAL(resp.stop_schedules(0).route().name(), "A");
    BOOST_CHECK_EQUAL(resp.stop_schedules(0).response_status(), pbnavitia::ResponseStatus::no_departure_this_day);
    BOOST_REQUIRE_EQUAL(resp.stop_schedules(1).route().name(), "B");
    BOOST_REQUIRE_EQUAL(resp.stop_schedules(1).date_times_size(),0);
    BOOST_CHECK_EQUAL(resp.stop_schedules(1).response_status(), pbnavitia::ResponseStatus::no_departure_this_day);
    }
    // no departure for route "B"
    {
    auto * data_ptr = b.data.get();
    navitia::PbCreator pb_creator(data_ptr, bt::second_clock::universal_time(), null_time_period);
    departure_board(pb_creator, "stop_point.uri=stop1", {}, {}, d("20150621T094500"), 43200, 0,
                    10, 0, nt::RTLevel::Base, std::numeric_limits<size_t>::max());
    resp = pb_creator.get_response();
    BOOST_REQUIRE_EQUAL(resp.stop_schedules_size(), 2);
    BOOST_REQUIRE_EQUAL(resp.stop_schedules(0).date_times_size(),1);
    BOOST_REQUIRE_EQUAL(resp.stop_schedules(0).route().name(), "A");
    BOOST_REQUIRE_EQUAL(resp.stop_schedules(1).route().name(), "B");
    BOOST_CHECK_EQUAL(resp.stop_schedules(1).response_status(), pbnavitia::ResponseStatus::no_departure_this_day);
    }

    // normal departure board with severity = NO_SERVICE on stop_point
    {
    using btp = boost::posix_time::time_period;

    navitia::apply_disruption(b.impact(nt::RTLevel::Adapted, "Disruption 1")
                              .severity(nt::disruption::Effect::NO_SERVICE)
                              .on(nt::Type_e::StopPoint, "stop1")
                              .application_periods(btp("20150615T010000"_dt, "20150625T235900"_dt))
                              .publish(btp("20150615T010000"_dt, "20150625T235900"_dt))
                              .msg("Disruption on stop_point stop1")
                              .get_disruption(),
                              *b.data->pt_data, *b.data->meta);

    // stop_schedules with rtLevel = Base
    auto * data_ptr = b.data.get();
    navitia::PbCreator pb_creator(data_ptr, "20150616T080000"_dt, null_time_period);
    departure_board(pb_creator, "stop_point.uri=stop1", {}, {}, d("20150615T094500"), 43200, 0,
                    10, 0, nt::RTLevel::Base, std::numeric_limits<size_t>::max());

    resp = pb_creator.get_response();
    BOOST_REQUIRE_EQUAL(resp.stop_schedules_size(), 2);
    BOOST_REQUIRE_EQUAL(resp.stop_schedules(0).date_times_size(),1);
    BOOST_REQUIRE_EQUAL(resp.stop_schedules(1).date_times_size(),1);
    BOOST_REQUIRE_EQUAL(resp.stop_schedules(1).stop_point().uri(),"stop1");
    BOOST_REQUIRE_EQUAL(resp.stop_schedules(1).stop_point().impact_uris_size(),1);
    const auto* impact = navitia::test::get_impact(resp.stop_schedules(1).stop_point().impact_uris(0), resp);
    BOOST_REQUIRE(impact);
    BOOST_REQUIRE_EQUAL(impact->messages_size(),1);
    BOOST_REQUIRE_EQUAL(impact->messages(0).text(), "Disruption on stop_point stop1");

    // stop_schedules on stop1 with rtLevel = RealTime
    pb_creator.init(data_ptr, "20150616T080000"_dt, null_time_period);
    departure_board(pb_creator, "stop_point.uri=stop1", {}, {}, d("20150615T094500"), 43200, 0,
                    10, 0, nt::RTLevel::RealTime, std::numeric_limits<size_t>::max());

    resp = pb_creator.get_response();
    BOOST_REQUIRE_EQUAL(resp.stop_schedules_size(), 2);
    BOOST_REQUIRE_EQUAL(resp.stop_schedules(0).date_times_size(),0);
    BOOST_REQUIRE_EQUAL(resp.stop_schedules(1).date_times_size(),0);
    BOOST_REQUIRE_EQUAL(resp.stop_schedules(0).response_status(),pbnavitia::ResponseStatus::active_disruption);
    BOOST_REQUIRE_EQUAL(resp.stop_schedules(1).response_status(),pbnavitia::ResponseStatus::active_disruption);
    BOOST_REQUIRE_EQUAL(resp.stop_schedules(1).stop_point().uri(),"stop1");
    BOOST_REQUIRE_EQUAL(resp.stop_schedules(1).stop_point().impact_uris_size(),1);
    }
}


BOOST_AUTO_TEST_CASE(partial_terminus_test1) {
    /*
     * Check partial terminus tag
     *
     * 2VJ on the line, one A->B->C and one A->B
     *
     * stop schedule for B must say it is a partial_terminus and stop_schedule on C must say it is a real terminus
     * */
    ed::builder b("20150615");
    b.vj("A", "11111111", "", true, "vj1", "")("stop1", 10*3600, 10*3600)
                                              ("stop2", 10*3600 + 30*60, 10*3600 + 30*60);
    b.vj("A", "10111111", "", true, "vj2", "")("stop1", 10*3600 + 30*60, 10*3600 + 30*60)
                                              ("stop2", 11*3600,11*3600)
                                              ("stop3", 11*3600 + 30*60,36300 + 30*60);
    const auto it = b.sas.find("stop3");
    b.data->pt_data->routes.front()->destination= it->second;

    b.finish();
    b.data->pt_data->sort_and_index();
    b.data->build_raptor();
    b.data->pt_data->build_uri();

    boost::gregorian::date begin = boost::gregorian::date_from_iso_string("20150615");
    boost::gregorian::date end = boost::gregorian::date_from_iso_string("20150630");

    b.data->meta->production_date = boost::gregorian::date_period(begin, end);
    auto * data_ptr = b.data.get();
    navitia::PbCreator pb_creator(data_ptr, bt::second_clock::universal_time(), null_time_period);
    departure_board(pb_creator, "stop_point.uri=stop1", {}, {}, d("20150615T094500"), 86400, 0,
                    10, 0, nt::RTLevel::Base, std::numeric_limits<size_t>::max());

    pbnavitia::Response resp = pb_creator.get_response();
    BOOST_REQUIRE_EQUAL(resp.stop_schedules_size(), 1);
    pbnavitia::StopSchedule stop_schedule = resp.stop_schedules(0);
    BOOST_CHECK(stop_schedule.date_times_size() == 2);
    BOOST_CHECK_EQUAL(stop_schedule.date_times(0).properties().destination().destination(), "stop2");
    BOOST_CHECK_EQUAL(stop_schedule.date_times(0).properties().vehicle_journey_id(), "vj1");

    {
        // VJ1 not in response, stop2 is terminus
        navitia::PbCreator pb_creator;
        pb_creator.init(b.data.get(), bt::second_clock::universal_time(), null_time_period);
        departure_board(pb_creator, "stop_point.uri=stop2", {}, {}, d("20150615T094500"), 86400, 0,
                        10, 0, nt::RTLevel::Base, std::numeric_limits<size_t>::max());

        resp = pb_creator.get_response();
        BOOST_REQUIRE_EQUAL(resp.stop_schedules_size(), 1);
        stop_schedule = resp.stop_schedules(0);
        BOOST_CHECK(stop_schedule.date_times_size() == 1);
        BOOST_CHECK_EQUAL(stop_schedule.date_times(0).properties().vehicle_journey_id(), "vj2");
    }

    {
        // Terminus
        navitia::PbCreator pb_creator;
        pb_creator.init(b.data.get(), bt::second_clock::universal_time(), null_time_period);
        departure_board(pb_creator, "stop_point.uri=stop3", {}, {}, d("20150615T094500"), 86400, 0,
                        10, 0, nt::RTLevel::Base, std::numeric_limits<size_t>::max());

        resp = pb_creator.get_response();

        BOOST_REQUIRE_EQUAL(resp.stop_schedules_size(), 1);
        BOOST_REQUIRE_EQUAL(resp.stop_schedules(0).date_times_size(), 0);
        BOOST_REQUIRE_EQUAL(resp.stop_schedules(0).response_status(), pbnavitia::ResponseStatus::terminus);
    }
}


BOOST_AUTO_TEST_CASE(terminus_multiple_route) {
    /*
     * Check partial terminus tag
     *
     * 1 line, 2 route, bob and bobette (one forward, and one backward)
     * Bob    :  A -> B -> C
     * Bobette:  C -> B -> A
     *
     * for a stop schedule on A, A must be the terminus only for bobette
     * */
    ed::builder b("20160802");
    b.vj("bob")    ("A", "10:00"_t)("B", "11:00"_t)("C", "12:00"_t);
    b.vj("bobette")("C", "10:00"_t)("B", "11:00"_t)("A", "12:00"_t);

    b.finish();
    b.data->pt_data->sort_and_index();
    b.data->build_raptor();
    b.data->pt_data->build_uri();
    auto * data_ptr = b.data.get();
    navitia::PbCreator pb_creator(data_ptr, bt::second_clock::universal_time(), null_time_period);
    departure_board(pb_creator, "stop_point.uri=A", {}, {}, d("20160802T090000"), 86400, 0,
                    10, 0, nt::RTLevel::Base, std::numeric_limits<size_t>::max());

    pbnavitia::Response resp = pb_creator.get_response();
    BOOST_REQUIRE_EQUAL(resp.stop_schedules_size(), 2);
    for (const auto& stop_sched: resp.stop_schedules()) {
        if (stop_sched.route().name() == "bob") {
            BOOST_CHECK_EQUAL(stop_sched.response_status(), pbnavitia::ResponseStatus::none);
        } else if (stop_sched.route().name() == "bobette") {
            BOOST_CHECK_EQUAL(stop_sched.response_status(), pbnavitia::ResponseStatus::terminus);
        } else {
            BOOST_FAIL("wrong route name");
        }
    }
}

// Test that departure_board manage to output departures even if there is no service for multiple days
BOOST_AUTO_TEST_CASE(departure_board_multiple_days) {
    ed::builder b("20180101");
    b.vj("A", "10000001")("stop1", "10:00:00"_t, "10:00:00"_t)
                         ("stop2", "10:30:00"_t, "10:30:00"_t);
    b.finish();
    b.data->pt_data->sort_and_index();
    b.data->build_raptor();
    b.data->pt_data->build_uri();
    auto * data_ptr = b.data.get();

    // We look for a departure on the first and try to reach the departure next sunday too
    {
        navitia::PbCreator pb_creator(data_ptr, bt::second_clock::universal_time(), null_time_period);
        departure_board(pb_creator, "stop_point.uri=stop1", {}, {}, d("20180101T100000"), 86400, 0,
                        10, 0, nt::RTLevel::Base, std::numeric_limits<size_t>::max());

        pbnavitia::Response resp = pb_creator.get_response();
        BOOST_REQUIRE_EQUAL(resp.stop_schedules_size(), 1);
        BOOST_REQUIRE_EQUAL(resp.stop_schedules(0).date_times_size(), 1);
    }
    {
        navitia::PbCreator pb_creator(data_ptr, bt::second_clock::universal_time(), null_time_period);
        departure_board(pb_creator, "stop_point.uri=stop1", {}, {}, d("20180101T100000"), 86400*6, 0,
                        10, 0, nt::RTLevel::Base, std::numeric_limits<size_t>::max());

        pbnavitia::Response resp = pb_creator.get_response();
        BOOST_REQUIRE_EQUAL(resp.stop_schedules_size(), 1);
        BOOST_REQUIRE_EQUAL(resp.stop_schedules(0).date_times_size(), 1);
    }
    {
        navitia::PbCreator pb_creator(data_ptr, bt::second_clock::universal_time(), null_time_period);
        departure_board(pb_creator, "stop_point.uri=stop1", {}, {}, d("20180101T100000"), 86400*7 - 1, 0,
                        10, 0, nt::RTLevel::Base, std::numeric_limits<size_t>::max());

        pbnavitia::Response resp = pb_creator.get_response();
        BOOST_REQUIRE_EQUAL(resp.stop_schedules_size(), 1);
        BOOST_REQUIRE_EQUAL(resp.stop_schedules(0).date_times_size(), 1);
    }
    {
        navitia::PbCreator pb_creator(data_ptr, bt::second_clock::universal_time(), null_time_period);
        departure_board(pb_creator, "stop_point.uri=stop1", {}, {}, d("20180101T100000"), 86400*7, 0,
                        10, 0, nt::RTLevel::Base, std::numeric_limits<size_t>::max());

        pbnavitia::Response resp = pb_creator.get_response();
        BOOST_REQUIRE_EQUAL(resp.stop_schedules_size(), 1);
        BOOST_REQUIRE_EQUAL(resp.stop_schedules(0).date_times_size(), 2);
    }
    // We have no departure on the second and try to reach the departure on sunday
    {
        navitia::PbCreator pb_creator(data_ptr, bt::second_clock::universal_time(), null_time_period);
        departure_board(pb_creator, "stop_point.uri=stop1", {}, {}, d("20180102T100000"), 86400, 0,
                        10, 0, nt::RTLevel::Base, std::numeric_limits<size_t>::max());

        pbnavitia::Response resp = pb_creator.get_response();
        BOOST_REQUIRE_EQUAL(resp.stop_schedules_size(), 1);
        BOOST_REQUIRE_EQUAL(resp.stop_schedules(0).date_times_size(), 0);
    }
    {
        navitia::PbCreator pb_creator(data_ptr, bt::second_clock::universal_time(), null_time_period);
        departure_board(pb_creator, "stop_point.uri=stop1", {}, {}, d("20180102T100000"), 86400*5, 0,
                        10, 0, nt::RTLevel::Base, std::numeric_limits<size_t>::max());

        pbnavitia::Response resp = pb_creator.get_response();
        BOOST_REQUIRE_EQUAL(resp.stop_schedules_size(), 1);
        BOOST_REQUIRE_EQUAL(resp.stop_schedules(0).date_times_size(), 0);
    }
    {
        navitia::PbCreator pb_creator(data_ptr, bt::second_clock::universal_time(), null_time_period);
        departure_board(pb_creator, "stop_point.uri=stop1", {}, {}, d("20180102T100000"), 86400*6 - 1, 0,
                        10, 0, nt::RTLevel::Base, std::numeric_limits<size_t>::max());

        pbnavitia::Response resp = pb_creator.get_response();
        BOOST_REQUIRE_EQUAL(resp.stop_schedules_size(), 1);
        BOOST_REQUIRE_EQUAL(resp.stop_schedules(0).date_times_size(), 0);
    }
    {
        navitia::PbCreator pb_creator(data_ptr, bt::second_clock::universal_time(), null_time_period);
        departure_board(pb_creator, "stop_point.uri=stop1", {}, {}, d("20180102T100000"), 86400*6, 0,
                        10, 0, nt::RTLevel::Base, std::numeric_limits<size_t>::max());

        pbnavitia::Response resp = pb_creator.get_response();
        BOOST_REQUIRE_EQUAL(resp.stop_schedules_size(), 1);
        BOOST_REQUIRE_EQUAL(resp.stop_schedules(0).date_times_size(), 1);
    }
}

BOOST_AUTO_TEST_CASE(departure_board_multiple_days_with_freq) {
    ed::builder b("20180101");
    b.frequency_vj("A", "10:00:00"_t, "11:00:00"_t, "00:30:00"_t, "", "10000001")
                    ("stop1", "10:00:00"_t, "10:00:00"_t)
                    ("stop2", "10:30:00"_t, "10:30:00"_t);
    b.finish();
    b.data->pt_data->sort_and_index();
    b.data->build_raptor();
    b.data->pt_data->build_uri();
    auto * data_ptr = b.data.get();

    // We look for a departure on the first and try to reach the departure next sunday too
    {
        navitia::PbCreator pb_creator(data_ptr, bt::second_clock::universal_time(), null_time_period);
        departure_board(pb_creator, "stop_point.uri=stop1", {}, {}, d("20180101T100000"), 86400, 0,
                        10, 0, nt::RTLevel::Base, std::numeric_limits<size_t>::max());

        pbnavitia::Response resp = pb_creator.get_response();
        BOOST_REQUIRE_EQUAL(resp.stop_schedules_size(), 1);
        BOOST_REQUIRE_EQUAL(resp.stop_schedules(0).date_times_size(), 3);
    }
    {
        navitia::PbCreator pb_creator(data_ptr, bt::second_clock::universal_time(), null_time_period);
        departure_board(pb_creator, "stop_point.uri=stop1", {}, {}, d("20180101T100000"), 86400*6, 0,
                        10, 0, nt::RTLevel::Base, std::numeric_limits<size_t>::max());

        pbnavitia::Response resp = pb_creator.get_response();
        BOOST_REQUIRE_EQUAL(resp.stop_schedules_size(), 1);
        BOOST_REQUIRE_EQUAL(resp.stop_schedules(0).date_times_size(), 3);
    }
    {
        navitia::PbCreator pb_creator(data_ptr, bt::second_clock::universal_time(), null_time_period);
        departure_board(pb_creator, "stop_point.uri=stop1", {}, {}, d("20180101T100000"), 86400*7 - 1, 0,
                        10, 0, nt::RTLevel::Base, std::numeric_limits<size_t>::max());

        pbnavitia::Response resp = pb_creator.get_response();
        BOOST_REQUIRE_EQUAL(resp.stop_schedules_size(), 1);
        BOOST_REQUIRE_EQUAL(resp.stop_schedules(0).date_times_size(), 3);
    }
    {
        navitia::PbCreator pb_creator(data_ptr, bt::second_clock::universal_time(), null_time_period);
        departure_board(pb_creator, "stop_point.uri=stop1", {}, {}, d("20180101T100000"), 86400*7, 0,
                        10, 0, nt::RTLevel::Base, std::numeric_limits<size_t>::max());

        pbnavitia::Response resp = pb_creator.get_response();
        BOOST_REQUIRE_EQUAL(resp.stop_schedules_size(), 1);
        BOOST_REQUIRE_EQUAL(resp.stop_schedules(0).date_times_size(), 4);
    }
    {
        navitia::PbCreator pb_creator(data_ptr, bt::second_clock::universal_time(), null_time_period);
        departure_board(pb_creator, "stop_point.uri=stop1", {}, {}, d("20180101T100000"), 86400*8, 0,
                        10, 0, nt::RTLevel::Base, std::numeric_limits<size_t>::max());

        pbnavitia::Response resp = pb_creator.get_response();
        BOOST_REQUIRE_EQUAL(resp.stop_schedules_size(), 1);
        BOOST_REQUIRE_EQUAL(resp.stop_schedules(0).date_times_size(), 6);
    }
    // We have no departure on the second and try to reach the departure on sunday
    {
        navitia::PbCreator pb_creator(data_ptr, bt::second_clock::universal_time(), null_time_period);
        departure_board(pb_creator, "stop_point.uri=stop1", {}, {}, d("20180102T100000"), 86400, 0,
                        10, 0, nt::RTLevel::Base, std::numeric_limits<size_t>::max());

        pbnavitia::Response resp = pb_creator.get_response();
        BOOST_REQUIRE_EQUAL(resp.stop_schedules_size(), 1);
        BOOST_REQUIRE_EQUAL(resp.stop_schedules(0).date_times_size(), 0);
    }
    {
        navitia::PbCreator pb_creator(data_ptr, bt::second_clock::universal_time(), null_time_period);
        departure_board(pb_creator, "stop_point.uri=stop1", {}, {}, d("20180102T100000"), 86400*5, 0,
                        10, 0, nt::RTLevel::Base, std::numeric_limits<size_t>::max());

        pbnavitia::Response resp = pb_creator.get_response();
        BOOST_REQUIRE_EQUAL(resp.stop_schedules_size(), 1);
        BOOST_REQUIRE_EQUAL(resp.stop_schedules(0).date_times_size(), 0);
    }
    {
        navitia::PbCreator pb_creator(data_ptr, bt::second_clock::universal_time(), null_time_period);
        departure_board(pb_creator, "stop_point.uri=stop1", {}, {}, d("20180102T100000"), 86400*6 - 1, 0,
                        10, 0, nt::RTLevel::Base, std::numeric_limits<size_t>::max());

        pbnavitia::Response resp = pb_creator.get_response();
        BOOST_REQUIRE_EQUAL(resp.stop_schedules_size(), 1);
        BOOST_REQUIRE_EQUAL(resp.stop_schedules(0).date_times_size(), 0);
    }
    {
        navitia::PbCreator pb_creator(data_ptr, bt::second_clock::universal_time(), null_time_period);
        departure_board(pb_creator, "stop_point.uri=stop1", {}, {}, d("20180102T100000"), 86400*6, 0,
                        10, 0, nt::RTLevel::Base, std::numeric_limits<size_t>::max());

        pbnavitia::Response resp = pb_creator.get_response();
        BOOST_REQUIRE_EQUAL(resp.stop_schedules_size(), 1);
        BOOST_REQUIRE_EQUAL(resp.stop_schedules(0).date_times_size(), 1);
    }
    {
        navitia::PbCreator pb_creator(data_ptr, bt::second_clock::universal_time(), null_time_period);
        departure_board(pb_creator, "stop_point.uri=stop1", {}, {}, d("20180102T100000"), 86400*7, 0,
                        10, 0, nt::RTLevel::Base, std::numeric_limits<size_t>::max());

        pbnavitia::Response resp = pb_creator.get_response();
        BOOST_REQUIRE_EQUAL(resp.stop_schedules_size(), 1);
        BOOST_REQUIRE_EQUAL(resp.stop_schedules(0).date_times_size(), 3);
    }

}

BOOST_FIXTURE_TEST_CASE(test_data_set, calendar_fixture) {
    //simple test on the data set creation

    //we check that each vj is associated with the right calendar
    //NOTE: this is better checked in the UT for associated cal
    BOOST_REQUIRE_EQUAL(b.data->pt_data->meta_vjs["week"]->associated_calendars.size(), 1);
    BOOST_REQUIRE(b.data->pt_data->meta_vjs["week"]->associated_calendars.at("week_cal"));
    BOOST_REQUIRE_EQUAL(b.data->pt_data->meta_vjs["week_bis"]->associated_calendars.size(), 1);
    BOOST_REQUIRE(b.data->pt_data->meta_vjs["week_bis"]->associated_calendars.at("week_cal"));
    BOOST_REQUIRE_EQUAL(b.data->pt_data->meta_vjs["weekend"]->associated_calendars.size(), 1);
    BOOST_REQUIRE(b.data->pt_data->meta_vjs["weekend"]->associated_calendars.at("weekend_cal"));
    BOOST_REQUIRE_EQUAL(b.data->pt_data->meta_vjs["all"]->associated_calendars.size(), 2);
    BOOST_REQUIRE(b.data->pt_data->meta_vjs["all"]->associated_calendars.at("week_cal"));
    BOOST_REQUIRE(b.data->pt_data->meta_vjs["all"]->associated_calendars.at("weekend_cal"));
    BOOST_REQUIRE(b.data->pt_data->meta_vjs["wednesday"]->associated_calendars.empty());
}

/*
 * unknown calendar in request => error
 */
BOOST_FIXTURE_TEST_CASE(test_no_weekend, calendar_fixture) {

    //when asked on non existent calendar, we get an error
    const boost::optional<const std::string> calendar_id{"bob_the_calendar"};
    auto * data_ptr = b.data.get();
    navitia::PbCreator pb_creator(data_ptr, bt::second_clock::universal_time(), null_time_period);
    departure_board(pb_creator, "stop_point.uri=stop1", calendar_id, {}, d("20120615T080000"), 86400, 0,
                    10, 0, nt::RTLevel::Base, std::numeric_limits<size_t>::max());
    pbnavitia::Response resp = pb_creator.get_response();
    BOOST_REQUIRE(resp.has_error());
    BOOST_REQUIRE(! resp.error().message().empty());
}

/*
 * For this test we want to get the schedule for the week end
 * we thus will get the 'week end' vj + the 'all' vj
 */
BOOST_FIXTURE_TEST_CASE(test_calendar_weekend, calendar_fixture) {
    const boost::optional<const std::string> calendar_id{"weekend_cal"};
    auto * data_ptr = b.data.get();
    navitia::PbCreator pb_creator(data_ptr, bt::second_clock::universal_time(), null_time_period);
    departure_board(pb_creator, "stop_point.uri=stop1", calendar_id, {}, d("20120615T080000"), 86400, 0,
                    10, 0, nt::RTLevel::Base, std::numeric_limits<size_t>::max());

    pbnavitia::Response resp = pb_creator.get_response();

    BOOST_REQUIRE(! resp.has_error());
    BOOST_CHECK_EQUAL(resp.stop_schedules_size(), 1);
    pbnavitia::StopSchedule stop_schedule = resp.stop_schedules(0);
    BOOST_REQUIRE_EQUAL(stop_schedule.date_times_size(), 2);
    auto stop_date_time = stop_schedule.date_times(0);
    BOOST_CHECK_EQUAL(stop_date_time.time(), time_to_int(15, 10, 00));
    BOOST_CHECK_EQUAL(stop_date_time.date(), 0); //no date

    stop_date_time = stop_schedule.date_times(1);
    BOOST_CHECK_EQUAL(stop_date_time.time(), time_to_int(20, 10, 00));
    BOOST_CHECK_EQUAL(stop_date_time.date(), 0); //no date
    //the vj 'wednesday' is never matched
}

/*
 * For this test we want to get the schedule for the week
 * we thus will get the 2 'week' vj + the 'all' vj
 */
BOOST_FIXTURE_TEST_CASE(test_calendar_week, calendar_fixture) {
    boost::optional<const std::string> calendar_id{"week_cal"};
    auto * data_ptr = b.data.get();
    navitia::PbCreator pb_creator(data_ptr, bt::second_clock::universal_time(), null_time_period);
    departure_board(pb_creator, "stop_point.uri=stop1", calendar_id, {}, d("20120615T080000"), 86400, 0,
                    10, 0, nt::RTLevel::Base, std::numeric_limits<size_t>::max());

    pbnavitia::Response resp = pb_creator.get_response();
    BOOST_REQUIRE(! resp.has_error());
    BOOST_CHECK_EQUAL(resp.stop_schedules_size(), 1);
    pbnavitia::StopSchedule stop_schedule = resp.stop_schedules(0);
    BOOST_REQUIRE_EQUAL(stop_schedule.date_times_size(), 3);
    auto stop_date_time = stop_schedule.date_times(0);
    BOOST_CHECK_EQUAL(stop_date_time.time(), time_to_int(10, 10, 00));
    BOOST_CHECK_EQUAL(stop_date_time.date(), 0); //no date
    stop_date_time = stop_schedule.date_times(1);
    BOOST_CHECK_EQUAL(stop_date_time.time(), time_to_int(11, 10, 00));
    BOOST_CHECK_EQUAL(stop_date_time.date(), 0); //no date
    stop_date_time = stop_schedule.date_times(2);
    BOOST_CHECK_EQUAL(stop_date_time.time(), time_to_int(15, 10, 00));
    BOOST_CHECK_EQUAL(stop_date_time.date(), 0); //no date
    //the vj 'wednesday' is never matched
}

/*
 * when asked with a calendar not associated with the line, we got an empty schedule
 */
BOOST_FIXTURE_TEST_CASE(test_not_associated_cal, calendar_fixture) {
    boost::optional<const std::string> calendar_id{"not_associated_cal"};
    auto * data_ptr = b.data.get();
    navitia::PbCreator pb_creator(data_ptr, bt::second_clock::universal_time(), null_time_period);
    departure_board(pb_creator, "stop_point.uri=stop1", calendar_id, {}, d("20120615T080000"), 86400, 0,
                    10, 0, nt::RTLevel::Base, std::numeric_limits<size_t>::max());


    using btp = boost::posix_time::time_period;
    navitia::apply_disruption(b.impact(nt::RTLevel::Adapted, "Disruption StopR1")
                              .severity(nt::disruption::Effect::NO_SERVICE)
                              .on(nt::Type_e::StopPoint, "StopR1")
                              .application_periods(btp("20120612T010000"_dt, "20120625T235900"_dt))
                              .publish(btp("20120612T010000"_dt, "20120625T235900"_dt))
                              .msg("Disruption on stop_point StopR1")
                              .get_disruption(),
                              *b.data->pt_data, *b.data->meta);


    b.data->pt_data->sort_and_index();
    b.data->build_uri();
    b.data->complete();
    b.data->build_raptor();
    {
        navitia::PbCreator pb_creator;
        pb_creator.init(b.data.get(), bt::second_clock::universal_time(), null_time_period);
        departure_board(pb_creator, "stop_point.uri=stop1", calendar_id, {}, d("20120615T080000"), 86400, 0,
                        10, 0, nt::RTLevel::Base, std::numeric_limits<size_t>::max());

        pbnavitia::Response resp = pb_creator.get_response();
        BOOST_REQUIRE(! resp.has_error());
        BOOST_CHECK_EQUAL(resp.stop_schedules_size(), 1);
        pbnavitia::StopSchedule stop_schedule = resp.stop_schedules(0);
        BOOST_REQUIRE_EQUAL(stop_schedule.date_times_size(), 0);
    }
    {
        navitia::PbCreator pb_creator;
        pb_creator.init(b.data.get(), bt::second_clock::universal_time(), null_time_period);
        departure_board(pb_creator, "stop_point.uri=StopR2", calendar_id, {}, d("20120615T080000"), 86400, 0,
                        10, 0, nt::RTLevel::Base, std::numeric_limits<size_t>::max());

        pbnavitia::Response resp = pb_creator.get_response();
        BOOST_REQUIRE(! resp.has_error());
        BOOST_CHECK_EQUAL(resp.stop_schedules_size(), 1);
        pbnavitia::StopSchedule stop_schedule = resp.stop_schedules(0);
        BOOST_REQUIRE_EQUAL(stop_schedule.date_times_size(), 2);
        BOOST_CHECK_EQUAL(stop_schedule.date_times(0).time(), time_to_int(10, 30, 00));
        BOOST_CHECK_EQUAL(stop_schedule.date_times(1).time(), time_to_int(10, 30, 00));
    }
}

BOOST_FIXTURE_TEST_CASE(test_calendar_with_exception, calendar_fixture) {
    //we add a new calendar that nearly match a vj
    auto nearly_cal = new navitia::type::Calendar(b.data->meta->production_date.begin());
    nearly_cal->uri = "nearly_cal";
    nearly_cal->active_periods.push_back({beg, end_of_year});
    nearly_cal->week_pattern = std::bitset<7>{"1111100"};
    //we add 2 exceptions (2 add), one by week
    navitia::type::ExceptionDate exception_date;
    exception_date.date = date("20120618");
    exception_date.type = navitia::type::ExceptionDate::ExceptionType::add;
    nearly_cal->exceptions.push_back(exception_date);
    exception_date.date = date("20120619");
    exception_date.type = navitia::type::ExceptionDate::ExceptionType::add;
    nearly_cal->exceptions.push_back(exception_date);

    b.data->pt_data->calendars.push_back(nearly_cal);
    b.lines["line:A"]->calendar_list.push_back(nearly_cal);

    // load metavj calendar association from database (association is tested in ed/tests/associated_calendar_test.cpp)
    navitia::type::AssociatedCalendar* associated_nearly_calendar = new navitia::type::AssociatedCalendar();
    associated_nearly_calendar->calendar = nearly_cal;
    exception_date.date = date("20120618");
    exception_date.type = navitia::type::ExceptionDate::ExceptionType::sub;
    associated_nearly_calendar->exceptions.push_back(exception_date);
    exception_date.date = date("20120619");
    exception_date.type = navitia::type::ExceptionDate::ExceptionType::sub;
    associated_nearly_calendar->exceptions.push_back(exception_date);
    b.data->pt_data->associated_calendars.push_back(associated_nearly_calendar);
    auto* week_mvj = b.data->pt_data->meta_vjs.get_mut("week");
    week_mvj->associated_calendars[nearly_cal->uri] = associated_nearly_calendar;
    auto* week_bis_mvj = b.data->pt_data->meta_vjs.get_mut("week_bis");
    week_bis_mvj->associated_calendars[nearly_cal->uri] = associated_nearly_calendar;
    auto* all_mvj = b.data->pt_data->meta_vjs.get_mut("all");
    all_mvj->associated_calendars[nearly_cal->uri] = associated_nearly_calendar;

    //call all the init again
    b.finish();
    b.data->build_uri();
    b.data->pt_data->sort_and_index();
    b.data->build_raptor();

    b.data->complete();

    boost::optional<const std::string> calendar_id{"nearly_cal"};
    auto * data_ptr = b.data.get();
    navitia::PbCreator pb_creator(data_ptr, bt::second_clock::universal_time(), null_time_period);
    departure_board(pb_creator, "stop_point.uri=stop1", calendar_id, {}, d("20120615T080000"), 86400, 0,
                    10, 0, nt::RTLevel::Base, std::numeric_limits<size_t>::max());

    pbnavitia::Response resp = pb_creator.get_response();
    //it should match only the 'all' vj
    BOOST_REQUIRE(! resp.has_error());
    BOOST_CHECK_EQUAL(resp.stop_schedules_size(), 1);
    pbnavitia::StopSchedule stop_schedule = resp.stop_schedules(0);
    BOOST_REQUIRE_EQUAL(stop_schedule.date_times_size(), 3);
    auto stop_date_time = stop_schedule.date_times(0);
    BOOST_CHECK_EQUAL(stop_date_time.time(), time_to_int(10, 10, 00));
    BOOST_CHECK_EQUAL(stop_date_time.date(), 0); //no date

    auto properties = stop_date_time.properties();
    BOOST_REQUIRE_EQUAL(properties.exceptions_size(), 2);
    auto exception = properties.exceptions(0);
    BOOST_REQUIRE_EQUAL(exception.uri(), "exception:120120618");
    BOOST_REQUIRE_EQUAL(exception.date(), "20120618");
    BOOST_REQUIRE_EQUAL(exception.type(), pbnavitia::ExceptionType::Remove);

    exception = properties.exceptions(1);
    BOOST_REQUIRE_EQUAL(exception.uri(), "exception:120120619");
    BOOST_REQUIRE_EQUAL(exception.date(), "20120619");
    BOOST_REQUIRE_EQUAL(exception.type(), pbnavitia::ExceptionType::Remove);
}

/*
 * For this test we want to get the schedule for the week
 * We'll apply an impact on stop1 of metaVj "week"
 * thus we should find only one stop_schedule on stop2 of Vj
 */
BOOST_FIXTURE_TEST_CASE(test_calendar_with_impact, calendar_fixture) {
    boost::optional<const std::string> calendar_id{"week_cal"};

    using btp = boost::posix_time::time_period;

    navitia::apply_disruption(b.impact(nt::RTLevel::Adapted, "Disruption stop2")
                              .severity(nt::disruption::Effect::NO_SERVICE)
                              .on(nt::Type_e::StopPoint, "stop2")
                              .application_periods(btp("20120612T010000"_dt, "20120625T235900"_dt))
                              .publish(btp("20120612T010000"_dt, "20120625T235900"_dt))
                              .msg("Disruption on stop_point stop2")
                              .get_disruption(),
                              *b.data->pt_data, *b.data->meta);
    b.data->pt_data->sort_and_index();
    b.data->build_uri();
    b.data->complete();
    b.data->build_raptor();
    auto * data_ptr = b.data.get();
    navitia::PbCreator pb_creator(data_ptr, bt::second_clock::universal_time(), null_time_period);
    departure_board(pb_creator, "stop_point.uri=stop1", calendar_id, {}, d("20120614T080000"), 86400, 0,
                    10, 0, nt::RTLevel::Base, std::numeric_limits<size_t>::max());

    pbnavitia::Response resp = pb_creator.get_response();
    BOOST_REQUIRE(! resp.has_error());
    BOOST_CHECK_EQUAL(resp.stop_schedules_size(), 1);
    pbnavitia::StopSchedule stop_schedule = resp.stop_schedules(0);
    BOOST_REQUIRE_EQUAL(stop_schedule.date_times_size(), 3);
    BOOST_CHECK_EQUAL(stop_schedule.date_times(0).time(), time_to_int(10, 10, 00));
    BOOST_CHECK_EQUAL(stop_schedule.date_times(1).time(), time_to_int(11, 10, 00));
    BOOST_CHECK_EQUAL(stop_schedule.date_times(2).time(), time_to_int(15, 10, 00));

}

struct small_cal_fixture {
    ed::builder b;
    small_cal_fixture(): b("20120614") {
        //vj1 has stoptimes all day from 00:10 every hour
        b.frequency_vj("line:A", 60*10, 24*60*60 + 60*10 - 1, 60*60, "network:R", "1111111", "", true, "vj1")
                ("stop1", 0, 0)
                ("stop2", 10, 20); //we need stop1 not to be the terminus

        //we add a calendar that match the vj
        auto cal = new navitia::type::Calendar(b.data->meta->production_date.begin());
        cal->uri = "cal";
        cal->active_periods.emplace_back(boost::gregorian::from_undelimited_string("20120614"),
                                         boost::gregorian::from_undelimited_string("20120621"));
        cal->week_pattern = std::bitset<7>{"1111111"};

        b.data->pt_data->calendars.push_back(cal);
        b.lines["line:A"]->calendar_list.push_back(cal);

        //we add a calendar with no activity
        auto empty_cal = new navitia::type::Calendar(b.data->meta->production_date.begin());
        empty_cal->uri = "empty_cal";
        empty_cal->active_periods.emplace_back(boost::gregorian::from_undelimited_string("20120614"),
                                         boost::gregorian::from_undelimited_string("20120621"));
        empty_cal->week_pattern = std::bitset<7>{"0000000"};

        b.data->pt_data->calendars.push_back(empty_cal);
        b.lines["line:A"]->calendar_list.push_back(empty_cal);

        // load metavj calendar association from database
        navitia::type::AssociatedCalendar* associated_calendar = new navitia::type::AssociatedCalendar();
        associated_calendar->calendar = cal;
        b.data->pt_data->associated_calendars.push_back(associated_calendar);
        b.data->pt_data->meta_vjs.get_mut("vj1")->associated_calendars[cal->uri] = associated_calendar;

        //call all the init again
        b.finish();
        b.data->build_uri();
        b.data->pt_data->sort_and_index();
        b.data->build_raptor();

        b.data->complete();
    }
};

/**
 * test that when asked for a schedule from a given time in the day
 * we have the schedule from this time and 'cycling' to the next day
 */

BOOST_FIXTURE_TEST_CASE(test_calendar_start_time, small_cal_fixture) {
    auto * data_ptr = b.data.get();
    navitia::PbCreator pb_creator(data_ptr, bt::second_clock::universal_time(), null_time_period);
    departure_board(pb_creator, "stop_point.uri=stop1", std::string("cal"), {}, d("20120615T080000"), 86400, 0,
                    10, 0, nt::RTLevel::Base, std::numeric_limits<size_t>::max());

    pbnavitia::Response resp = pb_creator.get_response();
    //we should get a nice schedule, first stop at 08:10, last at 07:10
    BOOST_REQUIRE(! resp.has_error());

    BOOST_CHECK_EQUAL(resp.stop_schedules_size(), 1);
    pbnavitia::StopSchedule stop_schedule = resp.stop_schedules(0);
    BOOST_REQUIRE_EQUAL(stop_schedule.date_times_size(), 24);

    for (size_t i = 0; i < 24; ++i) {
        auto hour = (i + 8) % 24;
        auto stop_date_time = stop_schedule.date_times(i);

        BOOST_CHECK_EQUAL(stop_date_time.time(), time_to_int(hour, 10, 00));
        BOOST_CHECK_EQUAL(stop_date_time.date(), 0); //no date
    }
}

/**
 * test the departarture board with a calenday without activity
 * No board must be returned
 */

BOOST_FIXTURE_TEST_CASE(test_not_matched_cal, small_cal_fixture) {
    auto * data_ptr = b.data.get();
    navitia::PbCreator pb_creator(data_ptr, bt::second_clock::universal_time(), null_time_period);
    departure_board(pb_creator, "stop_point.uri=stop1", std::string("empty_cal"), {}, d("20120615T080000"),
                    86400, 0, 10, 0, nt::RTLevel::Base, std::numeric_limits<size_t>::max());

    pbnavitia::Response resp = pb_creator.get_response();
    BOOST_REQUIRE(! resp.has_error());
    //no error but no results
    BOOST_CHECK_EQUAL(resp.stop_schedules_size(), 1);
    pbnavitia::StopSchedule stop_schedule = resp.stop_schedules(0);
    BOOST_REQUIRE_EQUAL(stop_schedule.date_times_size(), 0);
}

/**
 * test that when asked for a schedule from a given *period* in a day
 * we have the schedule from this time and finishing at the end of the period
 */
BOOST_FIXTURE_TEST_CASE(test_calendar_start_time_period, small_cal_fixture) {

    size_t nb_hour = 5;
    auto duration = nb_hour*60*60;
    auto * data_ptr = b.data.get();
    navitia::PbCreator pb_creator(data_ptr, bt::second_clock::universal_time(), null_time_period);
    departure_board(pb_creator, "stop_point.uri=stop1", std::string("cal"), {}, d("20120615T080000"),
                    duration, 0, 10, 0, nt::RTLevel::Base, std::numeric_limits<size_t>::max());

    pbnavitia::Response resp = pb_creator.get_response();
    //we should get a nice schedule, first stop at 08:10, last at 13:10
    BOOST_REQUIRE(! resp.has_error());

    BOOST_CHECK_EQUAL(resp.stop_schedules_size(), 1);
    pbnavitia::StopSchedule stop_schedule = resp.stop_schedules(0);
    BOOST_REQUIRE_EQUAL(stop_schedule.date_times_size(), nb_hour);

    for (size_t i = 0; i < nb_hour; ++i) {
        auto hour = i + 8;
        auto stop_date_time = stop_schedule.date_times(i);

        BOOST_CHECK_EQUAL(stop_date_time.time(), time_to_int(hour, 10, 00));
        BOOST_CHECK_EQUAL(stop_date_time.date(), 0); //no date
    }
}

/**
 * test that when asked for a schedule from a given *period* in a day,
 * it works even if the period extend to the next day
 * we have the schedule from this time and finishing at the end of the period
 */
BOOST_FIXTURE_TEST_CASE(test_calendar_start_time_period_before, small_cal_fixture) {

    //we ask for a schedule from 20:00 to 04:00
    size_t nb_hour = 8;
    auto duration = nb_hour*60*60;
    auto * data_ptr = b.data.get();
    navitia::PbCreator pb_creator(data_ptr, bt::second_clock::universal_time(), null_time_period);
    departure_board(pb_creator, "stop_point.uri=stop1", std::string("cal"), {}, d("20120615T200000"),
                    duration, 0, 10, 0, nt::RTLevel::Base, std::numeric_limits<size_t>::max());

    pbnavitia::Response resp = pb_creator.get_response();
    //we should get a nice schedule, first stop at 20:10, last at 04:10
    BOOST_REQUIRE(! resp.has_error());

    BOOST_CHECK_EQUAL(resp.stop_schedules_size(), 1);
    pbnavitia::StopSchedule stop_schedule = resp.stop_schedules(0);
    BOOST_REQUIRE_EQUAL(stop_schedule.date_times_size(), nb_hour);

    for (size_t i = 0; i < nb_hour; ++i) {
        auto hour = (i + 20)%24;
        auto stop_date_time = stop_schedule.date_times(i);

        BOOST_CHECK_EQUAL(stop_date_time.time(), time_to_int(hour, 10, 00));
        BOOST_CHECK_EQUAL(stop_date_time.date(), 0); //no date
    }
}


BOOST_FIXTURE_TEST_CASE(test_journey, calendar_fixture) {
    // some jormungandr test use the calendar_fixture for simple journey computation, so we add a simple test on journey
    navitia::routing::RAPTOR raptor(*(b.data));
    navitia::type::PT_Data& d = *b.data->pt_data;

    auto res1 = raptor.compute(d.stop_areas_map["stop1"], d.stop_areas_map["stop2"], 8*60*60, 0,
            navitia::DateTimeUtils::inf, nt::RTLevel::Base, 2_min, true);

    //we must have a journey
    BOOST_REQUIRE_EQUAL(res1.size(), 1);
}

/*
 *  Calling a stop_schedule asking only for base schedule data should return base schedule data 
 */
BOOST_FIXTURE_TEST_CASE(base_stop_schedule, departure_board_fixture) {
    auto * data_ptr = b.data.get();
    navitia::PbCreator pb_creator(data_ptr, bt::second_clock::universal_time(), null_time_period);
    departure_board(pb_creator, "stop_point.uri=S1", {}, {}, d("20160101T070000"),
                    86400, 0, 10, 0, nt::RTLevel::Base, std::numeric_limits<size_t>::max());
    pbnavitia::Response resp = pb_creator.get_response();

    auto find_sched = [](const std::string& route) {
        return [&route](const pbnavitia::StopSchedule& s) { return s.route().uri() == route; };
    };
    BOOST_REQUIRE_EQUAL(resp.stop_schedules_size(), 2);
    auto it_sc1 = boost::find_if(resp.stop_schedules(), find_sched("A:0"));
    BOOST_CHECK(it_sc1 != std::end(resp.stop_schedules()));
    const auto& sc1 = *it_sc1;
    BOOST_CHECK_EQUAL(sc1.stop_point().uri(), "S1");
    BOOST_CHECK_EQUAL(sc1.route().uri(), "A:0");
    BOOST_REQUIRE_EQUAL(sc1.date_times_size(), 3);
    BOOST_CHECK_EQUAL(sc1.date_times(0).time(), "09:00"_t);
    BOOST_CHECK_EQUAL(sc1.date_times(1).time(), "10:00"_t);
    BOOST_CHECK_EQUAL(sc1.date_times(2).time(), "11:00"_t);

    auto it_sc2 = boost::find_if(resp.stop_schedules(), find_sched("B:1"));
    BOOST_CHECK(it_sc2 != std::end(resp.stop_schedules()));
    const auto& sc2 = *it_sc2;
    BOOST_CHECK_EQUAL(sc2.stop_point().uri(), "S1");
    BOOST_CHECK_EQUAL(sc2.route().uri(), "B:1");
    BOOST_REQUIRE_EQUAL(sc2.date_times_size(), 1);
    BOOST_CHECK_EQUAL(sc2.date_times(0).time(), "11:30"_t);
}

// same as above but with realtime data
// all A's vj have 7 mn delay
BOOST_FIXTURE_TEST_CASE(rt_stop_schedule, departure_board_fixture) {
    auto * data_ptr = b.data.get();
    navitia::PbCreator pb_creator(data_ptr, bt::second_clock::universal_time(), null_time_period);
    departure_board(pb_creator, "stop_point.uri=S1", {}, {}, d("20160101T070000"),
                    86400, 0, 10, 0, nt::RTLevel::RealTime,
                    std::numeric_limits<size_t>::max());
    pbnavitia::Response resp = pb_creator.get_response();

    BOOST_REQUIRE_EQUAL(resp.stop_schedules_size(), 2);
    auto find_sched = [](const std::string& route) {
        return [&route](const pbnavitia::StopSchedule& s) { return s.route().uri() == route; };
    };
    auto it_sc1 = boost::find_if(resp.stop_schedules(), find_sched("A:0"));
    BOOST_CHECK(it_sc1 != std::end(resp.stop_schedules()));
    const auto& sc1 = *it_sc1;
    BOOST_CHECK_EQUAL(sc1.stop_point().uri(), "S1");
    BOOST_CHECK_EQUAL(sc1.route().uri(), "A:0");
    BOOST_REQUIRE_EQUAL(sc1.date_times_size(), 3);
    BOOST_CHECK_EQUAL(sc1.date_times(0).time(), "09:07"_t);
    BOOST_CHECK_EQUAL(sc1.date_times(1).time(), "10:07"_t);
    BOOST_CHECK_EQUAL(sc1.date_times(2).time(), "11:07"_t);

    auto it_sc2 = boost::find_if(resp.stop_schedules(), find_sched("B:1"));
    BOOST_CHECK(it_sc2 != std::end(resp.stop_schedules()));
    const auto& sc2 = *it_sc2;
    BOOST_CHECK_EQUAL(sc2.stop_point().uri(), "S1");
    BOOST_CHECK_EQUAL(sc2.route().uri(), "B:1");
    BOOST_REQUIRE_EQUAL(sc2.date_times_size(), 1);
    BOOST_CHECK_EQUAL(sc2.date_times(0).time(), "11:30"_t);
}

/*
 * same as base_stop_schedule but we limit the number of results per schedule
 */
BOOST_FIXTURE_TEST_CASE(base_stop_schedule_limit_per_schedule, departure_board_fixture) {
    auto * data_ptr = b.data.get();
    navitia::PbCreator pb_creator(data_ptr, bt::second_clock::universal_time(), null_time_period);
    departure_board(pb_creator, "stop_point.uri=S1", {}, {}, d("20160101T070000"),
                    86400, 0, 10, 0, nt::RTLevel::Base, 2);
    pbnavitia::Response resp = pb_creator.get_response();

    auto find_sched = [](const std::string& route) {
        return [&route](const pbnavitia::StopSchedule& s) { return s.route().uri() == route; };
    };
    BOOST_REQUIRE_EQUAL(resp.stop_schedules_size(), 2);
    auto it_sc1 = boost::find_if(resp.stop_schedules(), find_sched("A:0"));
    BOOST_CHECK(it_sc1 != std::end(resp.stop_schedules()));
    const auto& sc1 = *it_sc1;
    BOOST_CHECK_EQUAL(sc1.stop_point().uri(), "S1");
    BOOST_CHECK_EQUAL(sc1.route().uri(), "A:0");
    BOOST_REQUIRE_EQUAL(sc1.date_times_size(), 2);
    BOOST_CHECK_EQUAL(sc1.date_times(0).time(), "09:00"_t);
    BOOST_CHECK_EQUAL(sc1.date_times(1).time(), "10:00"_t);

    // does not change anything the B:1
    auto it_sc2 = boost::find_if(resp.stop_schedules(), find_sched("B:1"));
    BOOST_CHECK(it_sc2 != std::end(resp.stop_schedules()));
    const auto& sc2 = *it_sc2;
    BOOST_CHECK_EQUAL(sc2.stop_point().uri(), "S1");
    BOOST_CHECK_EQUAL(sc2.route().uri(), "B:1");
    BOOST_REQUIRE_EQUAL(sc2.date_times_size(), 1);
    BOOST_CHECK_EQUAL(sc2.date_times(0).time(), "11:30"_t);
}

/*
 * Check if length_of_time return the correct duration
 */
BOOST_AUTO_TEST_CASE(length_of_time_test1) {
    auto almost_midnight = 24_h-1_s;
    // Check when the arguments are in the same day
    BOOST_CHECK_EQUAL(length_of_time(7_h, 23_h),  16_h);
    BOOST_CHECK_EQUAL(length_of_time(0_h, almost_midnight), navitia::time_duration(23,59,59));
    // Check when the arguments are in two different days
    BOOST_CHECK_EQUAL(length_of_time(23_h, 7_h), navitia::time_duration(8,00,00));
    BOOST_CHECK_EQUAL(length_of_time(almost_midnight, 0_h), navitia::time_duration(00,00,01));
    // Check when the duration is null
    BOOST_CHECK_EQUAL(length_of_time(23_h, 23_h), navitia::time_duration(00,00,00));
}

/*
 * Asking if a date is between an opening and a closing time
 */
BOOST_AUTO_TEST_CASE(between_openin_and_closing_test1) {
    auto almost_midnight = 24_h-1_s;
    // When the line is open
    BOOST_CHECK(between_opening_and_closing(16_h, 7_h, 23_h));
    BOOST_CHECK(between_opening_and_closing(7_h, 7_h, 23_h));
    BOOST_CHECK(between_opening_and_closing(23_h, 7_h, 23_h));
    BOOST_CHECK(between_opening_and_closing(16_h, 0_h, almost_midnight));
    // When the line is closed
    BOOST_CHECK(!between_opening_and_closing(16_h, 23_h, 7_h));
    BOOST_CHECK(!between_opening_and_closing(16_h, almost_midnight, 0_h));
}

/*
 * Check if a line is closed for a given date and a given duration from this date
 */
BOOST_AUTO_TEST_CASE(line_closed_test1) {
    pt::ptime pt_12h = "20140101T120000"_dt;
    pt::ptime pt_6h = "20140101T060000"_dt;
    pt::ptime pt_22h = "20140101T220000"_dt;
    //When the line is not closed
    BOOST_CHECK(!line_closed(5_min, 7_h, 23_h,pt_12h));
    BOOST_CHECK(!line_closed(2_h, 7_h, 23_h,pt_6h));
    BOOST_CHECK(!line_closed(2_h, 7_h, 23_h,pt_22h));
    BOOST_CHECK(!line_closed(1_h, 7_h, 23_h,pt_22h));
    //When the line is not closed and the duration is bigger than the time between the opening and the closing
    BOOST_CHECK(!line_closed(18_h, 7_h, 23_h,pt_6h));
    //When the line is not closed and the opening is not the same date of the closing
    BOOST_CHECK(!line_closed(2_h, 23_h, 7_h,pt_22h));
    // When the line is closed
    BOOST_CHECK(line_closed(5_min, 7_h, 23_h,pt_6h));
    BOOST_CHECK(line_closed(5_min, 23_h, 7_h,pt_12h));

}

/**
 * test that when asked for a schedule from a given *period* in a day,
 * if the line is closed the ResponseStatus is no_active_circulation_this_day
 */
BOOST_AUTO_TEST_CASE(departureboard_test_with_lines_closed) {
    using pbnavitia::ResponseStatus;
    ed::builder b("20150615");
    b.vj("A", "110011000001", "", true, "vj1", "")
            ("stop1", "10:00"_t, "10:00"_t)
            ("stop2", "10:30"_t, "10:30"_t);
    b.vj("B", "110000001111", "", true, "vj2", "")
            ("stop1", "00:10"_t, "00:10"_t)
            ("stop2", "01:40"_t, "01:40"_t)
            ("stop3", "02:50"_t, "02:50"_t);
    // Add opening and closing time A is opened on one day and B on two days
    b.lines["A"]->opening_time = boost::posix_time::time_duration(9,0,0);
    b.lines["A"]->closing_time = boost::posix_time::time_duration(21,0,0);
    b.lines["B"]->opening_time = boost::posix_time::time_duration(23,30,0);
    b.lines["B"]->closing_time = boost::posix_time::time_duration(6,0,0);
    b.finish();
    b.data->pt_data->sort_and_index();
    b.data->build_raptor();
    b.data->pt_data->build_uri();
    pbnavitia::Response resp;
    auto * data_ptr = b.data.get();
    navitia::PbCreator pb_creator(data_ptr, bt::second_clock::universal_time(), null_time_period);
    // Request when the line is closed
    departure_board(pb_creator, "stop_point.uri=stop1", {}, {}, d("20150615T063000"), 600, 0,
                    10, 0, nt::RTLevel::Base, std::numeric_limits<size_t>::max());
    resp = pb_creator.get_response();
    BOOST_CHECK_EQUAL(resp.stop_schedules(0).response_status(), ResponseStatus::no_active_circulation_this_day);
    BOOST_CHECK_EQUAL(resp.stop_schedules(1).response_status(), ResponseStatus::no_active_circulation_this_day);
}

// Check with depth 3 than disable_geojson doesn't fill geojson in the response
BOOST_AUTO_TEST_CASE(departureboard_no_geojson) {
    using pbnavitia::ResponseStatus;
    ed::builder b("20161026");

    b.vj("A", "11111", "", true, "vj1", "")("stop1", "10:00"_t, "10:00"_t)("stop2", "10:30"_t, "10:30"_t);
    b.finish();
    b.data->pt_data->sort_and_index();
    b.data->build_raptor();
    b.data->pt_data->build_uri();

    pbnavitia::Response resp;
    auto * data_ptr = b.data.get();
    navitia::PbCreator pb_creator(data_ptr, bt::second_clock::universal_time(), null_time_period);
    departure_board(pb_creator, "stop_point.uri=stop1", {}, {}, d("20161026T093000"), 600, 3,
                    10, 0, nt::RTLevel::Base, std::numeric_limits<size_t>::max());
    resp = pb_creator.get_response();

    BOOST_REQUIRE_EQUAL(resp.stop_schedules_size(), 1);
    BOOST_REQUIRE(resp.stop_schedules(0).route().has_geojson());
    BOOST_REQUIRE(resp.stop_schedules(0).route().line().has_geojson());

    navitia::PbCreator pb_creator_no_geojson(data_ptr, bt::second_clock::universal_time(), null_time_period, true);
    departure_board(pb_creator_no_geojson, "stop_point.uri=stop1", {}, {}, d("20161026T093000"), 600, 3,
                    10, 0, nt::RTLevel::Base, std::numeric_limits<size_t>::max());
    resp = pb_creator_no_geojson.get_response();

    BOOST_REQUIRE_EQUAL(resp.stop_schedules_size(), 1);
    BOOST_REQUIRE(!resp.stop_schedules(0).route().has_geojson());
    BOOST_REQUIRE(!resp.stop_schedules(0).route().line().has_geojson());
}

// Test that frequency are correctly displayed and that we get departure_times and not boarding_times
BOOST_AUTO_TEST_CASE(route_schedule_with_boarding_time_frequency_and_calendar) {
    ed::builder b("20170101");

    boost::gregorian::date begin = boost::gregorian::date_from_iso_string("20170101");
    boost::gregorian::date end = boost::gregorian::date_from_iso_string("20180101");

    b.vj("L1").uri("vj:0")
        ("stop1", "8:00"_t, "8:00"_t, std::numeric_limits<uint16_t>::max(), false, true, 0, 300)
        ("stop2", "8:05"_t, "8:05"_t, std::numeric_limits<uint16_t>::max(), true, true, 900, 900)
        ("stop3", "8:10"_t, "8:10"_t, std::numeric_limits<uint16_t>::max(), true, false, 300, 0);

    b.vj("L1").uri("vj:2")
        ("stop1", "23:50"_t, "23:50"_t, std::numeric_limits<uint16_t>::max(), false, true, 0, 0)
        ("stop2", "23:55"_t, "23:55"_t, std::numeric_limits<uint16_t>::max(), true, true, 600, 1800)
        ("stop3", "24:10"_t, "24:10"_t, std::numeric_limits<uint16_t>::max(), true, false, 0, 900);

    b.frequency_vj("L1", "18:00"_t, "19:00"_t, "00:30"_t).uri("vj:1")
        ("stop1", "18:00"_t, "18:00"_t, std::numeric_limits<uint16_t>::max(), false, true, 0, 300)
        ("stop2", "18:05"_t, "18:05"_t, std::numeric_limits<uint16_t>::max(), true, true, 900, 900)
        ("stop3", "18:10"_t, "18:10"_t, std::numeric_limits<uint16_t>::max(), true, false, 300, 0);

    navitia::type::Calendar* c = new navitia::type::Calendar(begin);
    c->uri = "C1";
    c->active_periods.push_back({begin, end});
    c->week_pattern = std::bitset<7>("1111111");
    b.data->pt_data->calendars.push_back(c);
    b.data->pt_data->calendars_map[c->uri] = c;

    auto a1 = new navitia::type::AssociatedCalendar;
    a1->calendar = c;
    b.data->pt_data->associated_calendars.push_back(a1);
    b.data->pt_data->meta_vjs.get_mut("vj:0")->associated_calendars.insert({c->uri, a1});
    b.data->pt_data->meta_vjs.get_mut("vj:1")->associated_calendars.insert({c->uri, a1});

    b.finish();
    b.data->pt_data->sort_and_index();
    b.data->build_raptor();
    b.data->pt_data->build_uri();

    auto * data_ptr = b.data.get();
    navitia::PbCreator pb_creator_cal(data_ptr, bt::second_clock::universal_time(), null_time_period);
    departure_board(pb_creator_cal, "stop_point.uri=stop1", c->uri, {}, d("20170103T123000"), 86400, 3,
                    10, 0, nt::RTLevel::Base, std::numeric_limits<size_t>::max());
    auto resp = pb_creator_cal.get_response();
    BOOST_REQUIRE_EQUAL(resp.stop_schedules_size(), 1);
    auto stop_schedule = resp.stop_schedules(0);
    BOOST_CHECK_EQUAL(stop_schedule.stop_point().uri(), "stop1");
    BOOST_CHECK_EQUAL(stop_schedule.route().uri(), "L1:0");
    BOOST_REQUIRE_EQUAL(stop_schedule.date_times_size(), 4);
    BOOST_CHECK_EQUAL(stop_schedule.date_times(0).time(), "18:00"_t);
    BOOST_CHECK_EQUAL(stop_schedule.date_times(1).time(), "18:30"_t);
    BOOST_CHECK_EQUAL(stop_schedule.date_times(2).time(), "19:00"_t);
    BOOST_CHECK_EQUAL(stop_schedule.date_times(3).time(), "08:00"_t);

    navitia::PbCreator pb_creator_nocal(data_ptr, bt::second_clock::universal_time(), null_time_period);
    departure_board(pb_creator_nocal, "stop_point.uri=stop1", {}, {}, d("20170103T123000"), 86400, 3,
                    10, 0, nt::RTLevel::Base, std::numeric_limits<size_t>::max());
    resp = pb_creator_nocal.get_response();
    BOOST_REQUIRE_EQUAL(resp.stop_schedules_size(), 1);
    stop_schedule = resp.stop_schedules(0);
    BOOST_CHECK_EQUAL(stop_schedule.stop_point().uri(), "stop1");
    BOOST_CHECK_EQUAL(stop_schedule.route().uri(), "L1:0");
    BOOST_REQUIRE_EQUAL(stop_schedule.date_times_size(), 5);
    BOOST_CHECK_EQUAL(stop_schedule.date_times(0).time(), "18:00"_t);
    BOOST_CHECK_EQUAL(stop_schedule.date_times(1).time(), "18:30"_t);
    BOOST_CHECK_EQUAL(stop_schedule.date_times(2).time(), "19:00"_t);
    BOOST_CHECK_EQUAL(stop_schedule.date_times(3).time(), "23:50"_t);
    BOOST_CHECK_EQUAL(stop_schedule.date_times(4).time(), "08:00"_t);
}

// Check that results are in the right order even if the boarding_time are in a different one
BOOST_AUTO_TEST_CASE(route_schedule_with_boarding_time_order_check) {
    ed::builder b("20170101");

    b.vj("L1").uri("vj:0")
        ("stop1", "8:00"_t, "8:00"_t, std::numeric_limits<uint16_t>::max(), false, true, 900, 0)
        ("stop2", "8:05"_t, "8:05"_t, std::numeric_limits<uint16_t>::max(), true, true, 900, 0)
        ("stop3", "8:10"_t, "8:10"_t, std::numeric_limits<uint16_t>::max(), true, false, 900, 0);

    b.vj("L1").uri("vj:1")
        ("stop1", "8:05"_t, "8:05"_t, std::numeric_limits<uint16_t>::max(), false, true, 0, 900)
        ("stop2", "8:10"_t, "8:10"_t, std::numeric_limits<uint16_t>::max(), true, true, 0, 900)
        ("stop3", "8:15"_t, "8:15"_t, std::numeric_limits<uint16_t>::max(), true, false, 0, 900);

    b.finish();
    b.data->pt_data->sort_and_index();
    b.data->build_raptor();
    b.data->pt_data->build_uri();
    auto * data_ptr = b.data.get();

    navitia::PbCreator pb_creator(data_ptr, bt::second_clock::universal_time(), null_time_period);
    departure_board(pb_creator, "stop_point.uri=stop1", {}, {}, d("20170103T123000"), 86400, 3,
                    10, 0, nt::RTLevel::Base, std::numeric_limits<size_t>::max());
    auto resp = pb_creator.get_response();
    BOOST_REQUIRE_EQUAL(resp.stop_schedules_size(), 1);
    auto stop_schedule = resp.stop_schedules(0);
    BOOST_CHECK_EQUAL(stop_schedule.stop_point().uri(), "stop1");
    BOOST_CHECK_EQUAL(stop_schedule.route().uri(), "L1:0");
    BOOST_REQUIRE_EQUAL(stop_schedule.date_times_size(), 2);
    BOOST_CHECK_EQUAL(stop_schedule.date_times(0).time(), "08:00"_t);
    BOOST_CHECK_EQUAL(stop_schedule.date_times(1).time(), "08:05"_t);
}

BOOST_AUTO_TEST_CASE(stop_schedules_order_by_line_route_stop_point) {
    ed::builder b("20170101");

    b.sa("Rubens", 42, 42, false)("RubensLR")("RubensRL");
    b.sa("Jenner", 42, 42, false)("JennerLR")("JennerRL");

    b.vj("line:13").route("route:13lr")
        ("RubensLR", "9:00"_t)
        ("JennerLR", "10:00"_t);

    b.vj("line:5").route("route:5lr")
        ("RubensLR", "9:05"_t)
        ("JennerLR", "10:05"_t);

    b.vj("line:13").route("route:13rl")
        ("JennerRL", "10:00"_t)
        ("RubensRL", "11:00"_t);

    b.vj("line:5").route("route:5rl")
        ("JennerRL", "10:05"_t)
        ("RubensRL", "11:05"_t);

    b.finish();
    b.data->pt_data->build_uri();

    auto* line13 = b.data->pt_data->lines_map["line:13"];
    line13->code = "13";
    line13->name = "Alice";
    auto* route13rl = b.data->pt_data->routes_map["route:13rl"];
    route13rl->name = "Right - Left";
    auto* route13lr = b.data->pt_data->routes_map["route:13lr"];
    route13lr->name = "Left - Right";

    auto* line5 = b.data->pt_data->lines_map["line:13"];
    line5->code = "5";
    line5->name = "Zoe";
    auto* route5rl = b.data->pt_data->routes_map["route:5rl"];
    route5rl->name = "Right - Left";
    auto* route5lr = b.data->pt_data->routes_map["route:5lr"];
    route5lr->name = "Left - Right";

    b.data->pt_data->sort_and_index();
    b.data->build_raptor();
    auto * data_ptr = b.data.get();

    navitia::PbCreator pb_creator(data_ptr, bt::second_clock::universal_time(), null_time_period);
    departure_board(pb_creator, "network.uri=base_network", {}, {}, d("20170103T070000"), 86400, 3,
                    20, 0, nt::RTLevel::Base, std::numeric_limits<size_t>::max());
    auto resp = pb_creator.get_response();
    BOOST_REQUIRE_EQUAL(resp.stop_schedules_size(), 8);

    auto test = [&](int stop_schedule, const char* route, const char* stop_point) {
        BOOST_CHECK_EQUAL(resp.stop_schedules(stop_schedule).route().uri(), route);
        BOOST_CHECK_EQUAL(resp.stop_schedules(stop_schedule).stop_point().uri(), stop_point);
    };
    test(0, "route:5lr", "JennerLR");
    test(1, "route:5lr", "RubensLR");
    test(2, "route:5rl", "JennerRL");
    test(3, "route:5rl", "RubensRL");
    test(4, "route:13lr", "JennerLR");
    test(5, "route:13lr", "RubensLR");
    test(6, "route:13rl", "JennerRL");
    test(7, "route:13rl", "RubensRL");
}
