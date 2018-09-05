# Copyright (c) 2001-2017, Canal TP and/or its affiliates. All rights reserved.
#
# This file is part of Navitia,
#     the software to build cool stuff with public transport.
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
from . import helper_future
from navitiacommon import type_pb2
from jormungandr import utils
from collections import namedtuple
import logging

PlaceFreeAccessResult = namedtuple('PlaceFreeAccessResult', ['crowfly', 'odt', 'free_radius'])


class PlacesFreeAccess:
    """
    stop_points that are accessible freely from a given place: odt, stop_points of a stop_area, etc.
    """
    def __init__(self, future_manager, instance, requested_place_obj):
        """

        :param instance: instance of the coverage, all outside services callings pass through it(street network,
                         auto completion)
        :param requested_place_obj:
        """
        self._future_manager = future_manager
        self._instance = instance
        self._requested_place_obj = requested_place_obj
        self._value = None
        self._async_request()

    def _do_request(self):
        logger = logging.getLogger(__name__)
        logger.debug("requesting places with free access from %s", self._requested_place_obj.uri)

        stop_points = []
        place = self._requested_place_obj

        if place.embedded_type == type_pb2.STOP_AREA:
            stop_points = self._instance.georef.get_stop_points_for_stop_area(place.uri)
        elif place.embedded_type == type_pb2.ADMINISTRATIVE_REGION:
            stop_points = [sp for sa in place.administrative_region.main_stop_areas
                           for sp in sa.stop_points]
        elif place.embedded_type == type_pb2.STOP_POINT:
            stop_points = [place.stop_point]

        crowfly = {stop_point.uri for stop_point in stop_points}

        coord = utils.get_pt_object_coord(place)
        odt = set()

        if coord:
            odt_sps = self._instance.georef.get_odt_stop_points(coord)
            [odt.add(stop_point.uri) for stop_point in odt_sps]

        logger.debug("finish places with free access from %s", self._requested_place_obj.uri)

        # free_radius is empty until the proximities by crowfly are available
        free_radius = set()

        return PlaceFreeAccessResult(crowfly, odt, free_radius)

    def _async_request(self):
        self._value = self._future_manager.create_future(self._do_request)

    def wait_and_get(self):
        return self._value.wait_and_get()
