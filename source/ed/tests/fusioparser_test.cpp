/* Copyright © 2001-2015, Canal TP and/or its affiliates. All rights reserved.

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

#include "ed/data.h"
#include "ed/connectors/gtfs_parser.h"
#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE test_ed
#include <boost/test/unit_test.hpp>
#include <map>
#include <boost/range/algorithm.hpp>
#include "conf.h"
#include "ed/build_helper.h"
#include "tests/utils_test.h"
#include "ed/connectors/fusio_parser.h"

namespace bg = boost::gregorian;

struct logger_initialized {
    logger_initialized()   { init_logger(); }
};
BOOST_GLOBAL_FIXTURE( logger_initialized );

const std::string ntfs_path = std::string(navitia::config::fixtures_dir) + "/ed/ntfs";

BOOST_AUTO_TEST_CASE(parse_small_ntfs_dataset) {
    using namespace ed;

    Data data;
    connectors::FusioParser parser(ntfs_path);

    parser.fill(data);

    //we check that the data have been correctly loaded
    BOOST_REQUIRE_EQUAL(data.networks.size(), 1);
    BOOST_CHECK_EQUAL(data.networks[0]->name, "ligne flixible");
    BOOST_CHECK_EQUAL(data.networks[0]->uri, "ligneflexible");

    BOOST_REQUIRE_EQUAL(data.stop_areas.size(), 11);
    BOOST_CHECK_EQUAL(data.stop_areas[0]->name, "Arrêt A");
    BOOST_CHECK_EQUAL(data.stop_areas[0]->uri, "SA:A");
    BOOST_CHECK_CLOSE(data.stop_areas[0]->coord.lat(), 45.0296, 0.1);
    BOOST_CHECK_CLOSE(data.stop_areas[0]->coord.lon(), 0.5881, 0.1);

    // Check website, license of contributor
    BOOST_REQUIRE_EQUAL(data.contributors.size(), 1);
    BOOST_REQUIRE_EQUAL(data.contributors[0]->website, "http://www.canaltp.fr");
    BOOST_REQUIRE_EQUAL(data.contributors[0]->license, "LICENSE");

    // Check datasets
    BOOST_REQUIRE_EQUAL(data.datasets.size(), 5);
    BOOST_CHECK_EQUAL(data.datasets[0]->desc, "dataset_desc1");
    BOOST_CHECK_EQUAL(data.datasets[0]->uri, "dataset_id1");
    BOOST_CHECK_EQUAL(data.datasets[0]->validation_period, boost::gregorian::date_period("20180312"_d, "20180401"_d));
    BOOST_CHECK_EQUAL(data.datasets[0]->realtime_level == nt::RTLevel::Base, true);
    BOOST_CHECK_EQUAL(data.datasets[0]->system, "dataset_system1");
    BOOST_CHECK_EQUAL(data.vehicle_journeys[0]->dataset->uri, "dataset_id1");

    //timzeone check
    //no timezone is given for the stop area in this dataset, to the agency time zone (the default one) is taken
    for (auto sa: data.stop_areas) {
        BOOST_CHECK_EQUAL(sa->time_zone_with_name.first, "Europe/Paris");
    }

    BOOST_REQUIRE_EQUAL(data.stop_points.size(), 8);
    BOOST_CHECK_EQUAL(data.stop_points[0]->uri, "SP:A");
    BOOST_CHECK_EQUAL(data.stop_points[0]->name, "Arrêt A");
    BOOST_CHECK_CLOSE(data.stop_points[0]->coord.lat(), 45.0296, 0.1);
    BOOST_CHECK_CLOSE(data.stop_points[0]->coord.lon(), 0.5881, 0.1);
    BOOST_CHECK_EQUAL(data.stop_points[0]->stop_area, data.stop_areas[0]);
    BOOST_CHECK_EQUAL(data.stop_points[0]->fare_zone, "4");

    // check stops properties
    navitia::type::hasProperties has_properties;
    has_properties.set_property(navitia::type::hasProperties::WHEELCHAIR_BOARDING);
    has_properties.set_property(navitia::type::hasProperties::BIKE_ACCEPTED);
    BOOST_CHECK_EQUAL(data.stop_points[0]->accessible(has_properties.properties()), true);
    BOOST_CHECK_EQUAL(data.stop_points[1]->accessible(has_properties.properties()), false);

    for (int i = 1; i < 8; i++){
        BOOST_CHECK_EQUAL(data.stop_points[i]->accessible(has_properties.properties()), false);
    }

    //we should have one zonal stop point
    BOOST_REQUIRE_EQUAL(boost::count_if(data.stop_points, [](const types::StopPoint* sp) {
                            return sp->is_zonal && sp->name == "Beleymas" && sp->area;
                        }), 1);

    BOOST_REQUIRE_EQUAL(data.lines.size(), 4);
    BOOST_REQUIRE_EQUAL(data.routes.size(), 4);

    //chekc comments
    BOOST_REQUIRE_EQUAL(data.comment_by_id.size(), 2);
    BOOST_CHECK_EQUAL(data.comment_by_id["bob"], "bob is in the kitchen");
    BOOST_CHECK_EQUAL(data.comment_by_id["bobette"], "test comment");

    //7 objects have comments
    //the file contains wrongly formated comments, but they are skiped
    BOOST_REQUIRE_EQUAL(data.comments.size(), 5);

    std::map<ed::types::pt_object_header, std::vector<std::string>> expected_comments = {
        {ed::types::make_pt_object(data.routes[0]), {"bob"}},
        {ed::types::make_pt_object(data.vehicle_journeys[4]), {"bobette"}},
        {ed::types::make_pt_object(data.vehicle_journeys[5]), {"bobette"}}, //split too
        {ed::types::make_pt_object(data.stop_areas[2]), {"bob"}},
        {ed::types::make_pt_object(data.stop_points[3]), {"bobette"}}
    };

    BOOST_CHECK_EQUAL_COLLECTIONS(data.comments.begin(), data.comments.end(),
                                  expected_comments.begin(), expected_comments.end());


    std::map<const ed::types::StopTime*, std::vector<std::string>> expected_st_comments = {
            {{data.stops[6]}, {"bob", "bobette"}}, //stoptimes are split
            {{data.stops[7]}, {"bob", "bobette"}},
            {{data.stops[12]}, {"bob"}},
            {{data.stops[13]}, {"bob"}},
    };

    BOOST_CHECK_EQUAL_COLLECTIONS(data.stoptime_comments.begin(), data.stoptime_comments.end(),
                                  expected_st_comments.begin(), expected_st_comments.end());
    // manage itl
    const types::VehicleJourney* vj = data.vehicle_journeys.front();
    BOOST_REQUIRE_EQUAL(vj->stop_time_list[0]->local_traffic_zone, std::numeric_limits<uint16_t>::max());
    BOOST_REQUIRE_EQUAL(vj->stop_time_list[1]->local_traffic_zone, 1);
    BOOST_REQUIRE_EQUAL(vj->stop_time_list[2]->local_traffic_zone, 1);
    BOOST_REQUIRE_EQUAL(vj->stop_time_list[3]->local_traffic_zone, 2);
    BOOST_REQUIRE_EQUAL(vj->stop_time_list[4]->local_traffic_zone, std::numeric_limits<uint16_t>::max());

    // stop_headsign
    BOOST_REQUIRE_EQUAL(vj->stop_time_list[0]->headsign, "N1");
    BOOST_REQUIRE_EQUAL(vj->stop_time_list[1]->headsign, "N1");
    BOOST_REQUIRE_EQUAL(vj->stop_time_list[2]->headsign, "N1");
    BOOST_REQUIRE_EQUAL(vj->stop_time_list[3]->headsign, "N2");
    BOOST_REQUIRE_EQUAL(vj->stop_time_list[4]->headsign, "N2");

    //vj_headsign
    BOOST_REQUIRE_EQUAL(vj->name, "vehiclejourney1");

    navitia::type::hasVehicleProperties has_vehicle_properties;
    has_vehicle_properties.set_vehicle(navitia::type::hasVehicleProperties::BIKE_ACCEPTED);
    BOOST_CHECK_EQUAL(vj->accessible(has_vehicle_properties.vehicles()), true);

    for (const auto vj : data.vehicle_journeys) {
        for (size_t position = 0; position < vj->stop_time_list.size(); ++position) {
            BOOST_CHECK_EQUAL(vj->stop_time_list[position]->order, position);
        }
    }

    // feed_info
    std::map<std::string, std::string> feed_info_test ={
        {"feed_start_date","20150325"},
        {"feed_end_date","20150826"},
        {"feed_publisher_name","Ile de France open data"},
        {"feed_publisher_url","http://www.canaltp.fr"},
        {"feed_license","ODBL"},
        {"feed_creation_datetime","20150415T153234"}
    };
    BOOST_CHECK_EQUAL_COLLECTIONS(data.feed_infos.begin(), data.feed_infos.end(),
                                  feed_info_test.begin(), feed_info_test.end());

    BOOST_REQUIRE_EQUAL(data.meta.production_date,
                        boost::gregorian::date_period(boost::gregorian::date(2015, 3, 25),
                                                      boost::gregorian::date(2015, 8, 27)));

    /* Line groups.
     * 3 groups in the file, 3 use cases :
     *   - The first one has 2 lines, both linked to the group in line_group_links.txt.
     *     One link is twice in the file and one of the entry should be ignored,
     *   - The second one has only one line, linked only via the main_line_id field of line_groups.txt,
     *   - The third one has an unknown main_line_id and should be ignored
     *
     * At the start of the vector data.line_group_links we are going to have each main_line, since
     * those links are created in LineGroupFusioHandler, to allow use case number 2 where the line is not
     * in line_group_links.txt.
     */

    // Check group, only 2 must be created since the last one in line_groups.txt reference an unknown main_line_id
    BOOST_REQUIRE_EQUAL(data.line_groups.size(), 2);
    BOOST_REQUIRE_EQUAL(data.line_group_links.size(), 3);

    // First group
    BOOST_REQUIRE(data.line_groups[0]->main_line != nullptr);

    BOOST_CHECK_EQUAL(data.line_groups[0]->main_line->uri, "l2");
    BOOST_CHECK_EQUAL(data.line_group_links[0].line_group->uri, "lg1");
    BOOST_CHECK_EQUAL(data.line_group_links[2].line_group->uri, "lg1");
    BOOST_CHECK_EQUAL(data.line_group_links[0].line->uri, "l2");
    BOOST_CHECK_EQUAL(data.line_group_links[2].line->uri, "l3");
    BOOST_CHECK_EQUAL(data.line_group_links[0].line->uri, "l2");
    BOOST_CHECK_EQUAL(data.line_group_links[2].line->uri, "l3");

    // Second group, only one line, only defined as the main_line
    BOOST_REQUIRE(data.line_groups[1]->main_line != nullptr);

    BOOST_CHECK_EQUAL(data.line_groups[1]->main_line->uri, "l3");
    BOOST_CHECK_EQUAL(data.line_group_links[1].line_group->uri, "lg2");
    BOOST_CHECK_EQUAL(data.line_group_links[1].line->uri, "l3");

    // stop_headsign
    const types::VehicleJourney* vj1 = data.vehicle_journeys.at(6);

    // headsign of vj and stop_times
    BOOST_REQUIRE_EQUAL(vj1->name.substr(0,6), "NULL");

    BOOST_REQUIRE_EQUAL(vj1->stop_time_list[0]->headsign, "HS");
    BOOST_REQUIRE_EQUAL(vj1->stop_time_list[1]->headsign, "HS");
    BOOST_REQUIRE_EQUAL(vj1->stop_time_list[2]->headsign, "HS");
    BOOST_REQUIRE_EQUAL(vj1->stop_time_list[3]->headsign, "HS");
    BOOST_REQUIRE_EQUAL(vj1->stop_time_list[4]->headsign, "NULL");
    BOOST_REQUIRE_EQUAL(vj1->stop_time_list[5]->headsign, "NULL");

    // Shift the frequency
    data.finalize_frequency();
    // Frequencies and boarding / alighting times (times are in UTC, first vj of trip_5 is UTC+1 in NTFS)
    vj1 = data.vehicle_journeys.at(8);
    BOOST_REQUIRE_EQUAL(vj1->is_frequency(), true);
    // start time is moved 5 minutes in the past because of boarding_time
    BOOST_REQUIRE_EQUAL(vj1->start_time, "04:55:00"_t);
    BOOST_REQUIRE_EQUAL(vj1->end_time, "26:55:00"_t);
    BOOST_REQUIRE_EQUAL(vj1->headway_secs, "01:00:00"_t);

    BOOST_REQUIRE_EQUAL(vj1->stop_time_list.size(), 4);
    BOOST_REQUIRE_EQUAL(vj1->stop_time_list.at(0)->departure_time, 300);
    BOOST_REQUIRE_EQUAL(vj1->stop_time_list.at(0)->boarding_time, 0);
    BOOST_REQUIRE_EQUAL(vj1->stop_time_list.at(0)->alighting_time, 300);
    BOOST_REQUIRE_EQUAL(vj1->stop_time_list.at(1)->departure_time, 900);
    BOOST_REQUIRE_EQUAL(vj1->stop_time_list.at(1)->boarding_time, 0);
    BOOST_REQUIRE_EQUAL(vj1->stop_time_list.at(1)->alighting_time, 1800);
    BOOST_REQUIRE_EQUAL(vj1->stop_time_list.at(2)->departure_time, 1500);
    BOOST_REQUIRE_EQUAL(vj1->stop_time_list.at(2)->boarding_time, 600);
    BOOST_REQUIRE_EQUAL(vj1->stop_time_list.at(2)->alighting_time, 2400);
    BOOST_REQUIRE_EQUAL(vj1->stop_time_list.at(3)->departure_time, 2100);
    BOOST_REQUIRE_EQUAL(vj1->stop_time_list.at(3)->boarding_time, 2100);
    BOOST_REQUIRE_EQUAL(vj1->stop_time_list.at(3)->alighting_time, 2400);
}
/*
complete production without beginning_date
*/
BOOST_AUTO_TEST_CASE(complete_production_date_without_beginning_date) {
    using namespace ed;
    connectors::FusioParser parser(ntfs_path);
    std::string beginning_date;
    bg::date start_date(bg::date(2015, 1, 1));
    bg::date end_date(bg::date(2015, 1, 15));

    bg::date_period period = parser.complete_production_date(beginning_date, start_date, end_date);
    BOOST_CHECK_EQUAL(period, bg::date_period(start_date, end_date + bg::days(1)));

}

/*
complete production with beginning_date
*/
BOOST_AUTO_TEST_CASE(complete_production_date_with_beginning_date) {
    using namespace ed;
    connectors::FusioParser parser(ntfs_path);
    /*
beginning_date  20150103

                                                    |--------------------------------------|
                                                start_date(20150105)               end_date(20150115)
production date :                                   |-----------------------------------------|
                                                 start_date                                 end_date + 1 Day
    */
    std::string beginning_date("20150103");
    bg::date start_date(bg::date(2015, 1, 5));
    bg::date end_date(bg::date(2015, 1, 15));

    bg::date_period period = parser.complete_production_date(beginning_date, start_date, end_date);

    BOOST_CHECK_EQUAL(period, bg::date_period(start_date, end_date + bg::days(1)));


    /*
beginning_date                                          20150107
                                                           |

                                                    |--------------------------------------|
                                                start_date(20150105)               end_date(20150115)
production date :                                          |---------------------------------|
                                                        beginning_date                   end_date + 1 Day
    */
    beginning_date = "20150107";
    start_date = bg::date(2015, 1, 5);
    end_date = bg::date(2015, 1, 15);

    period = parser.complete_production_date(beginning_date, start_date, end_date);

    BOOST_CHECK_EQUAL(period, bg::date_period(bg::date(2015, 1, 7), end_date + bg::days(1)));
}


/*
 Test start_date and en_date in file feed_info, with beginning_date

beginning_date  20150310

                                                    |--------------------------------------|
                                                start_date(20150325)               end_date(20150826)
production date :                                   |-----------------------------------------|
                                                 start_date                                 end_date + 1 Day

 */
BOOST_AUTO_TEST_CASE(ntfs_with_feed_start_end_date_1) {

    using namespace ed;


    ed::Data data;
    ed::connectors::FusioParser parser(ntfs_path);
    parser.fill(data, "20150310");

    // feed_info
    std::map<std::string, std::string> feed_info_test ={
        {"feed_start_date","20150325"},
        {"feed_end_date","20150826"},
        {"feed_publisher_name","Ile de France open data"},
        {"feed_publisher_url","http://www.canaltp.fr"},
        {"feed_license","ODBL"},
        {"feed_creation_datetime","20150415T153234"}
    };
    BOOST_CHECK_EQUAL_COLLECTIONS(data.feed_infos.begin(), data.feed_infos.end(),
                                  feed_info_test.begin(), feed_info_test.end());

    BOOST_REQUIRE_EQUAL(data.meta.production_date,
                        boost::gregorian::date_period(boost::gregorian::date(2015, 3, 25),
                                                      boost::gregorian::date(2015, 8, 27)));

}


/*
 Test start_date and en_date in file feed_info, with beginning_date

beginning_date                                          20150327
                                                           |

                                                    |--------------------------------------|
                                                start_date(20150325)               end_date(20150826)
production date :                                          |---------------------------------|
                                                        beginning_date                   end_date + 1 Day

 */
BOOST_AUTO_TEST_CASE(ntfs_with_feed_start_end_date_2) {
    ed::Data data;

    ed::connectors::FusioParser parser(ntfs_path);
    parser.fill(data, "20150327");

    // feed_info
    std::map<std::string, std::string> feed_info_test ={
        {"feed_start_date","20150325"},
        {"feed_end_date","20150826"},
        {"feed_publisher_name","Ile de France open data"},
        {"feed_publisher_url","http://www.canaltp.fr"},
        {"feed_license","ODBL"},
        {"feed_creation_datetime","20150415T153234"}
    };
    BOOST_CHECK_EQUAL_COLLECTIONS(data.feed_infos.begin(), data.feed_infos.end(),
                                  feed_info_test.begin(), feed_info_test.end());
    BOOST_REQUIRE_EQUAL(data.meta.production_date,
                        boost::gregorian::date_period(boost::gregorian::date(2015, 3, 27),
                                                      boost::gregorian::date(2015, 8, 27)));
}


BOOST_AUTO_TEST_CASE(sync_ntfs) {
    ed::Data data;

    ed::connectors::FusioParser parser(ntfs_path + "_v5");
    parser.fill(data, "20150327");

    //we check that the data have been correctly loaded
    BOOST_REQUIRE_EQUAL(data.lines.size(), 4);
    BOOST_REQUIRE_EQUAL(data.stop_point_connections.size(), 1);
    BOOST_REQUIRE_EQUAL(data.stop_points.size(), 8);
    BOOST_CHECK_EQUAL(data.lines[0]->name, "ligne A Flexible");
    BOOST_CHECK_EQUAL(data.lines[0]->uri, "l1");
    BOOST_CHECK_EQUAL(data.lines[0]->text_color, "FFD700");    
    BOOST_REQUIRE_EQUAL(data.routes.size(), 4);

    navitia::type::hasProperties has_properties;
    has_properties.set_property(navitia::type::hasProperties::WHEELCHAIR_BOARDING);
    has_properties.set_property(navitia::type::hasProperties::BIKE_ACCEPTED);
    BOOST_CHECK_EQUAL(data.stop_point_connections[0]->accessible(has_properties.properties()), true);
    BOOST_CHECK_EQUAL(data.stop_points[0]->accessible(has_properties.properties()), true);

    for (int i = 1; i < 8; i++){
        BOOST_CHECK_EQUAL(data.stop_points[i]->accessible(has_properties.properties()), false);
    }

    BOOST_REQUIRE_EQUAL(data.datasets.size(), 1);
    BOOST_REQUIRE_EQUAL(data.contributors.size(), 1);
    BOOST_REQUIRE_EQUAL(data.contributors[0], data.datasets[0]->contributor);
    BOOST_CHECK_EQUAL(data.datasets[0]->desc, "dataset_test");
    BOOST_CHECK_EQUAL(data.datasets[0]->uri, "d1");
    BOOST_CHECK_EQUAL(data.datasets[0]->validation_period, boost::gregorian::date_period("20150826"_d, "20150926"_d));
    BOOST_CHECK_EQUAL(data.datasets[0]->realtime_level == nt::RTLevel::Base, true);
    BOOST_CHECK_EQUAL(data.datasets[0]->system, "obiti");
    
    // accepted side-effect (no link) as ntfs_v5 fixture does not contain datasets.txt, which is now required
    BOOST_CHECK(data.vehicle_journeys[0]->dataset == nullptr);

    const ed::types::VehicleJourney* vj = data.vehicle_journeys.front();
    navitia::type::hasVehicleProperties has_vehicle_properties;
    has_vehicle_properties.set_vehicle(navitia::type::hasVehicleProperties::BIKE_ACCEPTED);
    BOOST_CHECK_EQUAL(vj->accessible(has_vehicle_properties.vehicles()), true);

    vj = data.vehicle_journeys.back();
    BOOST_CHECK_EQUAL(vj->accessible(has_vehicle_properties.vehicles()), false);
}
