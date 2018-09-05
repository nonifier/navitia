# Copyright (c) 2001-2017, Canal TP and/or its affiliates. All rights reserved.
#
# This file is part of Navitia,
# the software to build cool stuff with public transport.
#
# Hope you'll enjoy and contribute to this project,
#     powered by Canal TP (www.canaltp.fr).
# Help us simplify mobility and open public transport:
#     a non ending quest to the responsive locomotion way of traveling!
#
# LICENCE: This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU Affero General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU Affero General Public License for more details.
#
# You should have received a copy of the GNU Affero General Public License
# along with this program. If not, see <http://www.gnu.org/licenses/>.
#
# Stay tuned using
# twitter @navitia
# IRC #navitia on freenode
# https://groups.google.com/d/forum/navitia
# www.navitia.io
from __future__ import absolute_import
from jormungandr.street_network.geovelo import Geovelo
from navitiacommon import type_pb2, response_pb2
import pybreaker
from mock import MagicMock
from .streetnetwork_test_utils import make_pt_object
from jormungandr.utils import str_to_time_stamp, PeriodExtremity
import requests_mock
import json

MOCKED_REQUEST = {'walking_speed': 1, 'bike_speed': 3.33}


def direct_path_response_valid():
    """
    A mock of a valid response from geovelo.
    Reply to POST of {"starts":[[48.803064,2.443385, "refStart1"]],
                      "ends":[[48.802049,2.426482, "refEnd1"]]}
    Modify with caution as it will affect every tests using these start and end uris.
    """
    return [
        {
            "distances": {
                "discouragedRoads": 65.0,
                "normalRoads": 3569.0,
                "recommendedRoads": 7759.0,
                "total": 11393.0
            },
            "duration": 3155,
            "estimatedDatetimeOfArrival": "2017-02-24T16:52:08.711",
            "estimatedDatetimeOfDeparture": "2017-02-24T15:59:33.711",
            "id": "bG9jPTQ4Ljg4Nzk0LDIuMzE0MzM4JmxvYz00OC44Mjk5MjcsMi4zNzY3NDcjQkVHSU5ORVIjRmFsc2UjQkVHSU5ORVIjMTMjRmFsc2UjRmFsc2UjMjAxNy0wMi0yNCAxNTo1OTozMy43MTEwNjgjVFJBRElUSU9OQUwjMCMwI1JFQ09NTUVOREVEI0ZhbHNl",
            "sections": [
                {
                    "details": {
                        "averageSpeed": 13,
                        "bikeType": "TRADITIONAL",
                        "direction": "Boulevard de Clichy",
                        "distances": {
                            "cycleway": 7563.0,
                            "discouragedRoads": 65.0,
                            "footway": 363.0,
                            "greenway": 0.0,
                            "lane": 78.0,
                            "livingstreet": 500.0,
                            "normalRoads": 3569.0,
                            "opposite": 141.0,
                            "pedestrian": 206.0,
                            "recommendedRoads": 7759.0,
                            "residential": 0.0,
                            "sharebusway": 118.0,
                            "steps": 0.0,
                            "total": 11393.0,
                            "zone30": 793.0
                        },
                        "elevations": None,
                        "instructions": [
                            [
                                "direction",
                                "roadName",
                                "roadLength",
                                "facility",
                                "cyclability",
                                "geometryIndex",
                                "orientation",
                                "cityNames"
                            ],
                            [
                                "HEAD_ON",
                                "Rue Cardinet",
                                58,
                                "SHAREBUSWAY",
                                3,
                                0,
                                "SW",
                                ""
                            ],
                            [
                                "GO_STRAIGHT",
                                "Rue Jouffroy d'Abbans",
                                40,
                                "RESIDENTIAL",
                                4,
                                3,
                                "W",
                                ""
                            ],
                            [
                                "REACHED_YOUR_DESTINATION",
                                "",
                                0,
                                "NONE",
                                3,
                                307,
                                "N",
                                ""
                            ]
                        ],
                        "profile": "BEGINNER",
                        "verticalGain": 51
                    },
                    "duration": 3155,
                    "estimatedDatetimeOfArrival": "2017-02-24T16:52:08.711",
                    "estimatedDatetimeOfDeparture": "2017-02-24T15:59:33.711",
                    "geometry": "_yzf|AszglClL`ShClEzCrHj@nNnBfD~AoHfDeD`nBmeC|AqBvAuAhWyWjVqUbJsJdd@uf@uPwh@sBgG{JoYmEkMeEwLxv@iu@}Hwj@k@}DcCqQ{Ims@m@yEbBcDbCyEhEiIhSm`@rPy\\bI}OfBwD|H}CaNehA}MwiAzA_LiBsPa@sDoBaSeAoKiCcHqBQmG{Rqk@meAa@oHRwSDuDv^yfChAmAnAkHrFqZnA{Gb@wDxc@m{Btd@a|Bz@cErHum@xLobBfBqMd@aHAkGmA{LaAkCaUwoBo@sFiR{aB_@wJuFoe@u@sDm@_CyTonB`@yDy@gIdA{EyBsTNoG_OqzBe@uDuAaLjD~DzAmAzNoLxCcBtDqEj_A}y@`_@i\\vFcF~c@w^~GuFbJgJ`JaHnCmDri@kd@nQcOxCiC~\\wYfCuBlC_CpB_BtCeCxDoEdx@}q@pG_Fpi@yd@lDuBhO_NjD}CxKyJ~IcIlIsHp^{\\dRwN~DuDjBqBxEkEzh@md@xHwG~AqAhGgFzMeLrM_Lp{@_t@lEeDfKsJvQ_PxDuCnCoCzO_NlIcHhSyQzFcF~CoCdsA{hAfDoCtSqStDyCjAmEn@uJ`Ywk@xEeJxHePwWabAaB{GkKqc@wL}n@sAuG`@qFxB}A~vBc|AfUkPjE}C|b@uZ`w@oj@~j@ca@tl@mb@zMgIzIoEvNiIlHaJjIcFfNwFhH_ChMuBpMoAlK_@rLH~Lp@|OjCfFjAtD|FfCz@lBp@nHe@xkCl~@vaAf]b{@pZzMhFnCb@nFpBtGkWlBuBbBcC|NgTzWu_@fB_CtCcC|@_Cvb@qgAtw@usBdRnMtJnGb\\vTy@hEvCtAfFdC`Bv@nEtBtExBhW`Q|HtE|ChBrAjAdL_Nb^qc@zUkYxCqDlCmDxRkTrAwAlEyDdH{GhZmm@lCgFfDkIvA|@lv@xe@pCfBpDlBpC_I|DuKr[gy@xAwDhKgX`FyMzBkFdCsG|JoWdk@c{AdLsZzEcObFgMbMa\\fR}f@nBzF`c@d`BjAdF`Mra@hGfSzw@f}BxF~LfCzDzA@lCzPnZdu@p^x{@rFzNzCfIpLl[fAvCvxA}`BPaDhFkGnHkJvF_IrKoIfCiDrz@nkBvKzFdEoDfGkEbO{MbFsErDzAz^_\\t_@e]lE_Gpc@ql@r_@wg@fk@gv@fa@{h@rNcRvLvWrWrk@fB`ElD~H`Pv]pEdKhAbCbN|[fBnElBtE~AlF`Mlu@rZoXjE{Dt^e\\|AnDzAhDzJwK]eAg@CTpBrGeBzBkBk@eBHa@J_AeAY}@}ClCiCw@wJ[cAeB?gScp@YtCxBjH~DvM",
                    "transportMode": "BIKE",
                    "waypoints": [
                        {
                            "latitude": 48.88794,
                            "longitude": 2.314338,
                            "title": None
                        },
                        {
                            "latitude": 48.829927,
                            "longitude": 2.376747,
                            "title": None
                        }
                    ],
                    "waypointsIndices": [
                        0,
                        308
                    ]
                }
            ],
            "title": "RECOMMENDED",
            "waypoints": [
                {
                    "latitude": 48.88794,
                    "longitude": 2.314338,
                    "title": None
                },
                {
                    "latitude": 48.829927,
                    "longitude": 2.376747,
                    "title": None
                }
            ]
        }
    ]


def direct_path_response_zero():
    return [
        {
            "distances": {
                "discouragedRoads": 0.0,
                "normalRoads": 0.0,
                "recommendedRoads": 0.0,
                "total": 0.0
            },
            "duration": 0,
            "estimatedDatetimeOfArrival": "2017-03-27T19:00:32.434",
            "estimatedDatetimeOfDeparture": "2017-03-27T19:00:32.434",
            "id": "bG9jPTQ4LjAsMi4wJmxvYz00OC4wLDIuMCNCRUdJTk5FUiNGYWxzZSNCRUdJTk5FUiMxMyNGYWxzZSNGYWxzZSMyMDE3LTAzLTI3IDE5OjAwOjMyLjQzNDMyNyNUUkFESVRJT05BTCMwIzAjUkVDT01NRU5ERUQjRmFsc2U=",
            "sections": [
                {
                    "details": {
                        "averageSpeed": 13,
                        "bikeType": "TRADITIONAL",
                        "direction": None,
                        "distances": {
                            "cycleway": 0.0,
                            "discouragedRoads": 0.0,
                            "footway": 0.0,
                            "greenway": 0.0,
                            "lane": 0.0,
                            "livingstreet": 0.0,
                            "normalRoads": 0.0,
                            "opposite": 0.0,
                            "pedestrian": 0.0,
                            "recommendedRoads": 0.0,
                            "residential": 0.0,
                            "sharebusway": 0.0,
                            "steps": 0.0,
                            "total": 0.0,
                            "zone30": 0.0
                        },
                        "elevations": None,
                        "instructions": [
                            [
                                "direction",
                                "roadName",
                                "roadLength",
                                "facility",
                                "cyclability",
                                "geometryIndex",
                                "orientation",
                                "cityNames"
                            ],
                            [
                                "HEAD_ON",
                                "voie pietonne",
                                0,
                                "FOOTWAY",
                                4,
                                0,
                                "N",
                                ""
                            ],
                            [
                                "REACHED_YOUR_DESTINATION",
                                "",
                                0,
                                "NONE",
                                3,
                                1,
                                "N",
                                ""
                            ]
                        ],
                        "profile": "BEGINNER",
                        "verticalGain": 0
                    },
                    "duration": 0,
                    "estimatedDatetimeOfArrival": "2017-03-27T19:00:32.434",
                    "estimatedDatetimeOfDeparture": "2017-03-27T19:00:32.434",
                    "geometry": "wytpzAg}ayB??",
                    "transportMode": "BIKE",
                    "waypoints": [
                        {
                            "latitude": 48.0,
                            "longitude": 2.0,
                            "title": None
                        },
                        {
                            "latitude": 48.0,
                            "longitude": 2.0,
                            "title": None
                        }
                    ],
                    "waypointsIndices": [
                        0,
                        2
                    ]
                }
            ],
            "title": "RECOMMENDED",
            "waypoints": [
                {
                    "latitude": 48.0,
                    "longitude": 2.0,
                    "title": None
                },
                {
                    "latitude": 48.0,
                    "longitude": 2.0,
                    "title": None
                }
            ]
        }
    ]


def isochrone_response_valid():
    """
    reply to POST of {"starts":[[48.85568,2.326355, "refStart1"]],
                      "ends":[[48.852291,2.359829, "refEnd1"],
                              [48.854607,2.388582, "refEnd2"]]}
    """
    return [["start_reference","end_reference","duration"],
            ["refStart1","refEnd1",1051],
            ["refStart1","refEnd2",1656]]


def pt_object_summary_test():
    summary = Geovelo._pt_object_summary_isochrone(make_pt_object(type_pb2.ADDRESS, lon=1.12, lat=13.15, uri='toto'))
    assert summary == [13.15, 1.12, 'toto']


def make_data_test():
    origins = [make_pt_object(type_pb2.ADDRESS, lon=2, lat=48.2, uri='refStart1')]
    destinations = [make_pt_object(type_pb2.ADDRESS, lon=3, lat=48.3, uri='refEnd1'),
                    make_pt_object(type_pb2.ADDRESS, lon=4, lat=48.4, uri='refEnd2')]
    data = Geovelo._make_request_arguments_isochrone(origins, destinations)
    assert json.loads(json.dumps(data)) == json.loads('''{
            "starts": [[48.2, 2.0, "refStart1"]], "ends": [[48.3, 3.0, "refEnd1"], [48.4, 4.0, "refEnd2"]],
            "transportMode": "BIKE",
            "bikeDetails": {"profile": "MEDIAN", "averageSpeed": 12, "bikeType": "TRADITIONAL"}}''')


def call_geovelo_func_with_circuit_breaker_error_test():
    instance = MagicMock()
    geovelo = Geovelo(instance=instance,
                      service_url='http://bob.com')
    geovelo.breaker = MagicMock()
    geovelo.breaker.call = MagicMock(side_effect=pybreaker.CircuitBreakerError())
    assert geovelo._call_geovelo(geovelo.service_url) == None


def call_geovelo_func_with_unknown_exception_test():
    instance = MagicMock()
    geovelo = Geovelo(instance=instance,
                      service_url='http://bob.com')
    geovelo.breaker = MagicMock()
    geovelo.breaker.call = MagicMock(side_effect=ValueError())
    assert geovelo._call_geovelo(geovelo.service_url) == None


def get_matrix_test():
    resp_json = isochrone_response_valid()
    matrix = Geovelo._get_matrix(resp_json)
    assert matrix.rows[0].routing_response[0].duration == 1051
    assert matrix.rows[0].routing_response[0].routing_status == response_pb2.reached
    assert matrix.rows[0].routing_response[1].duration == 1656
    assert matrix.rows[0].routing_response[1].routing_status == response_pb2.reached


def direct_path_geovelo_test():
    instance = MagicMock()
    geovelo = Geovelo(instance=instance,
                      service_url='http://bob.com')
    resp_json = direct_path_response_valid()

    origin = make_pt_object(type_pb2.ADDRESS, lon=2, lat=48.2, uri='refStart1')
    destination = make_pt_object(type_pb2.ADDRESS, lon=3, lat=48.3, uri='refEnd1')
    fallback_extremity = PeriodExtremity(str_to_time_stamp('20161010T152000'), False)
    with requests_mock.Mocker() as req:
        req.post('http://bob.com/api/v2/computedroutes?instructions=true&elevations=false&geometry=true'
                 '&single_result=true&bike_stations=false&objects_as_ids=true&', json=resp_json)
        geovelo_resp = geovelo.direct_path_with_fp('bike',
                                                   origin,
                                                   destination,
                                                   fallback_extremity,
                                                   MOCKED_REQUEST,
                                                   None)
        assert geovelo_resp.status_code == 200
        assert geovelo_resp.response_type == response_pb2.ITINERARY_FOUND
        assert len(geovelo_resp.journeys) == 1
        assert geovelo_resp.journeys[0].duration == 3155 # 52min35s
        assert len(geovelo_resp.journeys[0].sections) == 1
        assert geovelo_resp.journeys[0].arrival_date_time == str_to_time_stamp('20161010T152000')
        assert geovelo_resp.journeys[0].departure_date_time == str_to_time_stamp('20161010T142725')
        assert geovelo_resp.journeys[0].sections[0].type == response_pb2.STREET_NETWORK
        assert geovelo_resp.journeys[0].sections[0].type == response_pb2.STREET_NETWORK
        assert geovelo_resp.journeys[0].sections[0].duration == 3155
        assert geovelo_resp.journeys[0].sections[0].length == 11393
        assert geovelo_resp.journeys[0].sections[0].street_network.coordinates[2].lon == 2.314258
        assert geovelo_resp.journeys[0].sections[0].street_network.coordinates[2].lat == 48.887428
        assert geovelo_resp.journeys[0].sections[0].origin == origin
        assert geovelo_resp.journeys[0].sections[0].destination == destination
        assert geovelo_resp.journeys[0].sections[0].street_network.path_items[1].name == "Rue Jouffroy d'Abbans"
        assert geovelo_resp.journeys[0].sections[0].street_network.path_items[1].direction == 0
        assert geovelo_resp.journeys[0].sections[0].street_network.path_items[1].length == 40
        assert geovelo_resp.journeys[0].sections[0].street_network.path_items[1].duration == 11


def direct_path_geovelo_zero_test():
    instance = MagicMock()
    geovelo = Geovelo(instance=instance,
                      service_url='http://bob.com')
    resp_json = direct_path_response_zero()

    origin = make_pt_object(type_pb2.ADDRESS, lon=2, lat=48, uri='refStart1')
    destination = make_pt_object(type_pb2.ADDRESS, lon=2, lat=48, uri='refEnd1')
    fallback_extremity = PeriodExtremity(str_to_time_stamp('20161010T152000'), False)
    with requests_mock.Mocker() as req:
        req.post('http://bob.com/api/v2/computedroutes?instructions=true&elevations=false&geometry=true'
                 '&single_result=true&bike_stations=false&objects_as_ids=true&', json=resp_json)
        geovelo_resp = geovelo.direct_path_with_fp('bike',
                                                   origin,
                                                   destination,
                                                   fallback_extremity,
                                                   MOCKED_REQUEST,
                                                   None)
        assert geovelo_resp.status_code == 200
        assert geovelo_resp.response_type == response_pb2.ITINERARY_FOUND
        assert len(geovelo_resp.journeys) == 1
        assert geovelo_resp.journeys[0].duration == 0
        assert len(geovelo_resp.journeys[0].sections) == 1
        assert geovelo_resp.journeys[0].arrival_date_time == str_to_time_stamp('20161010T152000')
        assert geovelo_resp.journeys[0].departure_date_time == str_to_time_stamp('20161010T152000')
        assert geovelo_resp.journeys[0].sections[0].type == response_pb2.STREET_NETWORK
        assert geovelo_resp.journeys[0].sections[0].type == response_pb2.STREET_NETWORK
        assert geovelo_resp.journeys[0].sections[0].duration == 0
        assert geovelo_resp.journeys[0].sections[0].length == 0
        assert geovelo_resp.journeys[0].sections[0].origin == origin
        assert geovelo_resp.journeys[0].sections[0].destination == destination
        assert geovelo_resp.journeys[0].sections[0].street_network.path_items[0].name == "voie pietonne"
        assert geovelo_resp.journeys[0].sections[0].street_network.path_items[0].direction == 0
        assert geovelo_resp.journeys[0].sections[0].street_network.path_items[0].length == 0
        assert geovelo_resp.journeys[0].sections[0].street_network.path_items[0].duration == 0

        assert len(geovelo_resp.feed_publishers) == 1
        assert geovelo_resp.feed_publishers[0].id == 'geovelo'
        assert geovelo_resp.feed_publishers[0].name == 'geovelo'
        assert geovelo_resp.feed_publishers[0].license == 'Private'
        assert geovelo_resp.feed_publishers[0].url == 'http://about.geovelo.fr/cgu/'


def isochrone_geovelo_test():
    instance = MagicMock()
    geovelo = Geovelo(instance=instance,
                      service_url='http://bob.com')
    resp_json = isochrone_response_valid()

    origins = [make_pt_object(type_pb2.ADDRESS, lon=2, lat=48.2, uri='refStart1')]
    destinations = [make_pt_object(type_pb2.ADDRESS, lon=3, lat=48.3, uri='refEnd1'),
                   make_pt_object(type_pb2.ADDRESS, lon=4, lat=48.4, uri='refEnd2')]

    with requests_mock.Mocker() as req:
        req.post('http://bob.com/api/v2/routes_m2m', json=resp_json, status_code=200)
        geovelo_response = geovelo.get_street_network_routing_matrix(
            origins,
            destinations,
            'bike',
            13371337,
            MOCKED_REQUEST)
        assert geovelo_response.rows[0].routing_response[0].duration == 1051
        assert geovelo_response.rows[0].routing_response[0].routing_status == response_pb2.reached
        assert geovelo_response.rows[0].routing_response[1].duration == 1656
        assert geovelo_response.rows[0].routing_response[1].routing_status == response_pb2.reached

def distances_durations_test():
    """
    Check that the response from geovelo is correctly formatted with 'distances' and 'durations' sections
    """
    instance = MagicMock()
    geovelo = Geovelo(instance=instance,
                      service_url='http://bob.com')
    resp_json = direct_path_response_valid()

    origin = make_pt_object(type_pb2.ADDRESS, lon=2, lat=48.2, uri='refStart1')
    destination = make_pt_object(type_pb2.ADDRESS, lon=3, lat=48.3, uri='refEnd1')
    fallback_extremity = PeriodExtremity(str_to_time_stamp('20161010T152000'), True)

    proto_resp = geovelo._get_response(resp_json, origin, destination, fallback_extremity)
    assert proto_resp.journeys[0].durations.total == 3155
    assert proto_resp.journeys[0].durations.bike == 3155
    assert proto_resp.journeys[0].distances.bike == 11393.0

def make_request_arguments_bike_details_test():
    """
    Check that the bikeDetails is well formatted for the request with right averageSpeed value
    """
    instance = MagicMock()
    geovelo = Geovelo(instance=instance, service_url='http://bob.com')
    data = geovelo._make_request_arguments_bike_details(bike_speed_mps=3.33)
    assert json.loads(json.dumps(data)) == json.loads('''{"profile": "MEDIAN", "averageSpeed": 12,
    "bikeType": "TRADITIONAL"}''')

    data = geovelo._make_request_arguments_bike_details(bike_speed_mps=4.1)
    assert json.loads(json.dumps(data)) == json.loads('''{"profile": "MEDIAN", "averageSpeed": 15,
    "bikeType": "TRADITIONAL"}''')



