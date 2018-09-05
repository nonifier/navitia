# coding=utf-8

#  Copyright (c) 2001-2014, Canal TP and/or its affiliates. All rights reserved.
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
from __future__ import absolute_import, print_function, unicode_literals, division
from flask_restful import fields, abort
from jormungandr import i_manager
from jormungandr.interfaces.v1.fields import error,\
    PbField, NonNullList, NonNullNested,\
    feed_publisher, Links, JsonString, place, \
    ListLit, beta_endpoint, context
from jormungandr.timezone import set_request_timezone
from jormungandr.interfaces.v1.errors import ManageError
from jormungandr.utils import date_to_timestamp
from jormungandr.interfaces.parsers import UnsignedInteger
from jormungandr.interfaces.v1.journey_common import JourneyCommon
from jormungandr.interfaces.v1.fields import DateTime
from jormungandr.interfaces.v1.serializer.api import GraphicalIsrochoneSerializer
from jormungandr.interfaces.v1.decorators import get_serializer

graphical_isochrone = {
    "geojson": JsonString(),
    "max_duration": fields.Integer(),
    "min_duration": fields.Integer(),
    'from': PbField(place, attribute='origin'),
    "to": PbField(place, attribute="destination"),
    'requested_date_time': DateTime(),
    'min_date_time': DateTime(),
    'max_date_time': DateTime()
}


graphical_isochrones = {
    "isochrones": NonNullList(NonNullNested(graphical_isochrone), attribute="graphical_isochrones"),
    "error": PbField(error, attribute='error'),
    "feed_publishers": fields.List(NonNullNested(feed_publisher)),
    "links": fields.List(Links()),
    "warnings": ListLit([fields.Nested(beta_endpoint)]),
    'context': context
}


class GraphicalIsochrone(JourneyCommon):

    def __init__(self):
        super(GraphicalIsochrone, self).__init__(output_type_serializer=GraphicalIsrochoneSerializer)
        parser_get = self.parsers["get"]
        parser_get.add_argument("min_duration", type=UnsignedInteger(), default=0)
        parser_get.add_argument("boundary_duration[]", type=UnsignedInteger(), action="append")

    @get_serializer(serpy=GraphicalIsrochoneSerializer, marshall=graphical_isochrones)
    @ManageError()
    def get(self, region=None, lon=None, lat=None, uri=None):

        args = self.parsers['get'].parse_args()
        self.region = i_manager.get_region(region, lon, lat)
        args.update(self.parse_args(region, uri))

        if not (args['destination'] or args['origin']):
            abort(400, message="you should provide a 'from' or a 'to' argument")
        if not args['max_duration'] and not args["boundary_duration[]"]:
            abort(400, message="you should provide a 'boundary_duration[]' or a 'max_duration' argument")
        if args['destination'] and args['origin']:
            abort(400, message="you cannot provide a 'from' and a 'to' argument")
        if 'ridesharing' in args['origin_mode'] or 'ridesharing' in args['destination_mode']:
            abort(400, message='ridesharing isn\'t available on isochrone')

        set_request_timezone(self.region)
        original_datetime = args['original_datetime']
        if original_datetime:
            new_datetime = self.convert_to_utc(original_datetime)
        args['datetime'] = date_to_timestamp(new_datetime)

        response = i_manager.dispatch(args, "graphical_isochrones", self.region)

        return response

    def options(self, **kwargs):
        return self.api_description(**kwargs)
