# coding=utf-8

# Copyright (c) 2001-2016, Canal TP and/or its affiliates. All rights reserved.
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
import logging
from jormungandr import utils, new_relic
import abc

# Using abc.ABCMeta in a way it is compatible both with Python 2.7 and Python 3.x
# http://stackoverflow.com/a/38668373/1614576
ABC = abc.ABCMeta(str("ABC"), (object,), {})


# Regarding to the type of direct path, some special treatments may be done in connector
class StreetNetworkPathType:
    DIRECT = 0
    BEGINNING_FALLBACK = 1
    ENDING_FALLBACK = 2

from collections import namedtuple
StreetNetworkPathKey = namedtuple('StreetNetworkPathKey', ['mode', 'orig_uri', 'dest_uri',
                                                           'streetnetwork_path_type', 'period_extremity'])


class AbstractStreetNetworkService(ABC):
    @abc.abstractmethod
    def get_street_network_routing_matrix(self, origins, destinations, street_network_mode, max_duration, request, **kwargs):
        pass

    @abc.abstractmethod
    def status(self):
        pass

    def direct_path_with_fp(self, mode, pt_object_origin, pt_object_destination,
                            fallback_extremity, request, direct_path_type):
        resp = self._direct_path(mode, pt_object_origin, pt_object_destination,
                                 fallback_extremity, request, direct_path_type)

        self._add_feed_publisher(resp)
        return resp


    @abc.abstractmethod
    def _direct_path(self, mode, pt_object_origin, pt_object_destination, fallback_extremity, request, direct_path_type):
        '''
        :param fallback_extremity: is a PeriodExtremity (a datetime and it's meaning on the fallback period)
        :param direct_path_type : direct_path need to be treated differently regarding to the used connector
        '''
        pass

    @abc.abstractmethod
    def make_path_key(self, mode, orig_uri, dest_uri, streetnetwork_path_type, period_extremity):
        """
        This method is used for the caching method. It's connector specific.
        :param orig_uri, dest_uri, mode: matters obviously
        :param streetnetwork_path_type: whether it's a fallback at the beginning,
            the end of journey or a direct path without PT also matters especially for car (to know if we park before or after)
        :param period_extremity: is a PeriodExtremity (a datetime and it's meaning on the fallback period)

        """
        pass

    def feed_publisher(self):
        return None

    def record_external_failure(self, message):
        utils.record_external_failure(message, 'streetnetwork', unicode(self.sn_system_id))

    def record_call(self, status, **kwargs):
        """
        status can be in: ok, failure
        """
        params = {'streetnetwork_id': unicode(self.sn_system_id), 'status': status}
        params.update(kwargs)
        new_relic.record_custom_event('streetnetwork', params)

    def _add_feed_publisher(self, resp):
        sn_feed = self.feed_publisher()
        if sn_feed:
            feed = resp.feed_publishers.add()
            feed.id = sn_feed.id
            feed.name = sn_feed.name
            feed.license = sn_feed.license
            feed.url = sn_feed.url


class StreetNetwork(object):

    @staticmethod
    def get_street_network_services(instance, street_network_configurations):
        log = logging.getLogger(__name__)
        street_network_services = []
        for config in street_network_configurations:
            # Set default arguments
            if 'args' not in config:
                config['args'] = {}
            if 'service_url' not in config['args']:
                config['args'].update({'service_url': None})
            if 'instance' not in config['args']:
                config['args'].update({'instance': instance})
            # for retrocompatibility, since 'modes' was originaly outside 'args'
            if 'modes' not in config['args']:
                config['args']['modes'] = config.get('modes', [])

            try:
                service = utils.create_object(config)
            except KeyError as e:
                raise KeyError('impossible to build a StreetNetwork, missing mandatory field in configuration: {}'
                               .format(e.message))

            street_network_services.append(service)
            log.info('** StreetNetwork {} used for direct_path with mode: {} **'
                     .format(type(service).__name__, service.modes))
        return street_network_services
