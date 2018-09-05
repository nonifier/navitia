# Copyright (c) 2001-2014, Canal TP and/or its affiliates. All rights reserved.
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

from .tests_mechanism import AbstractTestFixture, dataset
from .check_utils import *


# Looking for (identifiable) objects impacted by 'disruptions' inputed
def impacted_ids(disruptions):
    # for the impacted object returns:
    #  * id
    #  * or the headsign (for the display_information field)
    #  * or the id of the vehicle_journey for stop_schedule.date_time
    def get_id(obj):
        id = obj.impacted_object.get('id')
        if id is None:
            # display_information case
            id = obj.impacted_object.get('headsign')
        if id is None:
            # stop_schedule.date_time case
            assert obj.impacted_object['links'][0]['type'] == 'vehicle_journey'
            id = obj.impacted_object['links'][0]['id']
        return id

    return set(get_id(o) for o in disruptions['line_section_on_line_1'])


@dataset({"line_sections_test": {}})
class TestLineSections(AbstractTestFixture):

    def default_query(self, q, **kwargs):
        """query navitia with a current date in the publication period of the impacts"""
        return self.query_region('{}?_current_datetime=20170101T100000'.format(q), **kwargs)

    def has_disruption(self, q, object_get, disruption_uri, **kwargs):
        r = self.default_query(q, **kwargs)
        return has_disruption(r, object_get, disruption_uri)

    def has_disruption(self, object_get, disruption_uri):
        """Little helper calling the detail of an object and checking it's disruptions"""
        r = self.default_query('{col}/{uri}'.format(col=object_get.collection, uri=object_get.uri))
        return has_disruption(r, object_get, disruption_uri)

    def has_tf_disruption(self, q, disruption):
        """
        checks a disruption is present in traffic report response to
        the request provided
        """
        r = self.default_query(q)

        return any(
            disruption == d['disruption_id'] for d in r.get('disruptions', [])
        )

    def has_tf_linked_disruption(self, q, disruption, object_get):
        """
        checks a disruption is present in traffic report response to
        the request provided
        also check that the disruption is linked to the specified object
        """
        r = self.default_query(q)

        if not (r.get('disruptions')
                and r.get('traffic_reports')
                and r['traffic_reports'][0].get(object_get.collection)):
            return False

        if disruption not in [d['disruption_id'] for d in r['disruptions']]:
            return False

        for obj in r['traffic_reports'][0][object_get.collection]:
            if obj['id'] == object_get.uri:
                return any(
                    disruption == link['id'] for link in obj.get('links', [])
                )

        return False

    def test_line_section_structure(self):
        r = self.default_query('stop_points/C_1')

        assert len(get_not_null(r, 'disruptions')) == 1
        is_valid_line_section_disruption(r['disruptions'][0])

    def test_on_stop_areas(self):
        """
        the line section disruption is not linked to a stop area, we cannot directly find our disruption
        """
        assert not self.has_disruption(ObjGetter('stop_areas', 'A'), 'line_section_on_line_1')
        assert not self.has_disruption(ObjGetter('stop_areas', 'B'), 'line_section_on_line_1')
        assert not self.has_disruption(ObjGetter('stop_areas', 'C'), 'line_section_on_line_1')
        assert not self.has_disruption(ObjGetter('stop_areas', 'D'), 'line_section_on_line_1')
        assert not self.has_disruption(ObjGetter('stop_areas', 'E'), 'line_section_on_line_1')
        assert not self.has_disruption(ObjGetter('stop_areas', 'F'), 'line_section_on_line_1')

    def test_on_stop_points(self):
        """
        the line section disruption should be linked to the impacted stop_points
        """
        assert not self.has_disruption(ObjGetter('stop_points', 'A_1'), 'line_section_on_line_1')
        assert not self.has_disruption(ObjGetter('stop_points', 'B_1'), 'line_section_on_line_1')
        assert self.has_disruption(ObjGetter('stop_points', 'C_1'), 'line_section_on_line_1')
        assert self.has_disruption(ObjGetter('stop_points', 'D_1'), 'line_section_on_line_1')
        assert self.has_disruption(ObjGetter('stop_points', 'E_1'), 'line_section_on_line_1')
        assert not self.has_disruption(ObjGetter('stop_points', 'F_1'), 'line_section_on_line_1')

    def test_on_vehicle_journeys(self):
        """
        the line section disruption should be linked to the impacted vehicle journeys
        """
        assert self.has_disruption(ObjGetter('vehicle_journeys', 'vj:1:1'), 'line_section_on_line_1')
        assert not self.has_disruption(ObjGetter('vehicle_journeys', 'vj:1:2'), 'line_section_on_line_1')

    def test_traffic_reports_on_stop_areas(self):
        """
        we should be able to find the related line section disruption with /traffic_report
        """

        # line_section_on_line_1
        scenario = {
            'A': False,
            'B': False,
            'C': True,
            'D': True,
            'E': True,
            'F': False,
        }

        for sa, result in scenario.iteritems():
            assert result == self.has_tf_disruption(
                'stop_areas/{}/traffic_reports'.format(sa),
                'line_section_on_line_1',
            )
            assert result == self.has_tf_linked_disruption(
                'stop_areas/{}/traffic_reports'.format(sa),
                'line_section_on_line_1',
                ObjGetter('stop_areas', sa)
            )

        # line_section_on_line_1_other_effect
        scenario = {
            'A': False,
            'B': False,
            'C': False,
            'D': False,
            'E': True,
            'F': True,
        }

        for sa, result in scenario.iteritems():
            assert result == self.has_tf_disruption(
                'stop_areas/{}/traffic_reports'.format(sa),
                'line_section_on_line_1_other_effect',
            )
            assert result == self.has_tf_linked_disruption(
                'stop_areas/{}/traffic_reports'.format(sa),
                'line_section_on_line_1_other_effect',
                ObjGetter('stop_areas', sa)
            )

    def test_traffic_reports_on_networks(self):
        def has_dis(q):
            r = self.default_query(q)
            return 'line_section_on_line_1' in (d['disruption_id'] for d in r['disruptions'])

        assert has_dis('networks/base_network/traffic_reports')
        assert not has_dis('networks/network:other/traffic_reports')
        r = self.default_query('traffic_reports')
        assert len(r['traffic_reports']) == 1 # only one network (base_network) is disrupted
        assert len(r['traffic_reports'][0]['stop_areas']) == 4
        for sa in r['traffic_reports'][0]['stop_areas']:
            assert len(sa['links']) == (2 if sa['id'] == 'E' else 1)

    def test_traffic_reports_on_lines(self):
        """
        we should be able to find the related line section disruption with /traffic_report
        """

        # line_section_on_line_1
        assert self.has_tf_disruption(
            'lines/line:1/traffic_reports',
            'line_section_on_line_1'
        )
        assert self.has_tf_linked_disruption(
            'lines/line:1/traffic_reports',
            'line_section_on_line_1',
            ObjGetter('lines', 'line:1')
        )

        assert not self.has_tf_disruption(
            'lines/line:2/traffic_reports',
            'line_section_on_line_1'
        )
        assert not self.has_tf_linked_disruption(
            'lines/line:2/traffic_reports',
            'line_section_on_line_1',
            ObjGetter('lines', 'line:2')
        )

        # line_section_on_line_1_other_effect
        assert self.has_tf_disruption(
            'lines/line:1/traffic_reports',
            'line_section_on_line_1_other_effect',
        )
        assert self.has_tf_linked_disruption(
            'lines/line:1/traffic_reports',
            'line_section_on_line_1_other_effect',
            ObjGetter('lines', 'line:1')
        )

        # There is a disruption and it affects 1 stop of the line
        # whereas the line itself is not concerned (the
        # disrupted routes are linked to other lines)
        #
        # This test doesn't work with the actual behavior
        # assert not self.has_tf_disruption(
        #     'lines/line:2/traffic_reports',
        #     'line_section_on_line_1_other_effect',
        # )

        # At least, the disruption and the line are not linked
        assert not self.has_tf_linked_disruption(
            'lines/line:2/traffic_reports',
            'line_section_on_line_1_other_effect',
            ObjGetter('lines', 'line:2')
        )

    def test_traffic_reports_on_routes(self):
        """
        for routes since we display the impacts on all the stops (but we do not display a route object)
        we display the disruption even if the route has not been directly impacted
        """
        def has_dis(q):
            r = self.default_query(q)
            return 'line_section_on_line_1' in (d['disruption_id'] for d in r['disruptions'])

        assert has_dis('routes/route:line:1:1/traffic_reports')
        assert not has_dis('routes/route:line:1:2/traffic_reports')
        # route 3 has been impacted by the line section but it has no stoppoint in the line section
        # so in this case we do not display the disruption
        assert not has_dis('routes/route:line:1:3/traffic_reports')

    def test_traffic_reports_on_vjs(self):
        """
        for /traffic_reports on vjs it's a bit the same as the lines
        we display a line section disruption if it impacts the stops of the vj
        """
        def has_dis(q):
            r = self.default_query(q)
            return 'line_section_on_line_1' in (d['disruption_id'] for d in r['disruptions'])

        assert has_dis('vehicle_journeys/vj:1:1/traffic_reports')
        assert not has_dis('vehicle_journeys/vj:1:2/traffic_reports')
        assert not has_dis('vehicle_journeys/vj:1:3/traffic_reports')

    def test_traffic_reports_on_stop_points(self):
        """
        for /traffic_reports on stop_points
        there is a line section disruption link if it impacts the stop_point
        """

        # line_section_on_line_1
        scenario = {
            'A_1': False,
            'A_2': False,
            'B_1': False,
            'B_2': False,
            'C_1': True,
            'C_2': False,
            'D_1': True,
            'D_2': False,
            'E_1': True,
            'E_2': False,
            'F_1': False,
            'F_2': False
        }

        for sp, result in scenario.iteritems():
            assert result == self.has_tf_disruption(
                'stop_points/{}/traffic_reports'.format(sp),
                'line_section_on_line_1'
            )
            assert result == self.has_tf_linked_disruption(
                'stop_points/{}/traffic_reports'.format(sp),
                'line_section_on_line_1',
                ObjGetter('stop_areas', sp[0])
            )

        # line_section_on_line_1_other_effect
        scenario = {
            'A_1': False,
            'A_2': False,
            'B_1': False,
            'B_2': False,
            'C_1': False,
            'C_2': False,
            'D_1': False,
            'D_2': False,
            'E_1': True,
            'E_2': False,
            'F_1': True,
            'F_2': False
        }

        for sp, result in scenario.iteritems():
            assert result == self.has_tf_disruption(
                'stop_points/{}/traffic_reports'.format(sp),
                'line_section_on_line_1_other_effect'
            )
            assert result == self.has_tf_linked_disruption(
                'stop_points/{}/traffic_reports'.format(sp),
                'line_section_on_line_1_other_effect',
                ObjGetter('stop_areas', sp[0])
            )

    def test_journeys_use_vj_impacted_by_line_section(self):
        """
        for /journeys, we should display a line section disruption only if we use an impacted section

        We do all the calls as base_schedule to see the disruption but not be really impacted by them

        In this test we leave at 08:00 so we'll use vj:1:1 (that is impacted) during the application
        period of the impact
        """
        date = '20170102T080000'  # with this departure time we should take 'vj:1:1' that is impacted
        current = '_current_datetime=20170101T080000'

        def journey_query(_from, to):
            return 'journeys?from={fr}&to={to}&datetime={dt}&{cur}&data_freshness=base_schedule'.\
                format(fr=_from,
                       to=to,
                       dt=date,
                       cur=current)

        def journeys(_from, to):
            q = journey_query(_from=_from, to=to)
            r = self.query_region(q)
            self.is_valid_journey_response(r, q)
            return r

        # if we do not use an impacted section we shouldn't see the impact
        r = journeys(_from='A', to='B')
        assert get_used_vj(r) == [['vj:1:1']]  # we check the used vj to be certain we took an impacted vj
        assert 'line_section_on_line_1' not in get_all_element_disruptions(r['journeys'], r)

        # 'C' is part of the impacted section, it's the first stop,
        # so A->C use an impacted section
        r = journeys(_from='A', to='C')
        assert get_used_vj(r) == [['vj:1:1']]
        disrupts = get_all_element_disruptions(r['journeys'], r)
        assert impacted_ids(disrupts) == {'vj:1:1'}

        # same for B->C
        r = journeys(_from='B', to='C')
        assert get_used_vj(r) == [['vj:1:1']]
        disrupts = get_all_element_disruptions(r['journeys'], r)
        assert 'line_section_on_line_1' in disrupts
        assert impacted_ids(disrupts) == {'vj:1:1'}

        r = journeys(_from='C', to='D')
        assert get_used_vj(r) == [['vj:1:1']]
        disrupts = get_all_element_disruptions(r['journeys'], r)
        assert 'line_section_on_line_1' in disrupts
        assert impacted_ids(disrupts) == {'vj:1:1'}

        r = journeys(_from='D', to='F')
        assert get_used_vj(r) == [['vj:1:1']]
        disrupts = get_all_element_disruptions(r['journeys'], r)
        assert 'line_section_on_line_1' in disrupts
        assert impacted_ids(disrupts) == {'vj:1:1'}

        #this journey passes over the impacted section but does not stop (or transfer) on it
        #as such it isn't impacted
        r = journeys(_from='A', to='F')
        assert get_used_vj(r) == [['vj:1:1']]
        disrupts = get_all_element_disruptions(r['journeys'], r)
        assert 'line_section_on_line_1' not in disrupts

    def test_journeys_use_vj_not_impacted_by_line_section(self):
        """
        for /journeys, we should display a line section disruption only if we use an impacted section

        We do all the calls as base_schedule to see the disruption but not be really impacted by them

        In this test we leave at 09:00 so we'll use vj:1:2 (that is not impacted because not part of an
        impacted route) during the application period of the impact
        """
        date = '20170102T090000'  # with this departure time we should take 'vj:1:2' that is impacted
        current = '_current_datetime=20170101T080000'

        def journey_query(_from, to):
            return 'journeys?from={fr}&to={to}&datetime={dt}&{cur}&data_freshness=base_schedule'.\
                format(fr=_from,
                       to=to,
                       dt=date,
                       cur=current)

        def journeys(_from, to):
            q = journey_query(_from=_from, to=to)
            r = self.query_region(q)
            self.is_valid_journey_response(r, q)
            return r

        r = journeys(_from='A', to='F')
        assert get_used_vj(r) == [['vj:1:2']]  # we check the used vj to be certain we took the right vj
        assert 'line_section_on_line_1' not in get_all_element_disruptions(r['journeys'], r)

        r = journeys(_from='C', to='D')
        assert get_used_vj(r) == [['vj:1:2']]
        assert 'line_section_on_line_1' not in get_all_element_disruptions(r['journeys'], r)

        r = journeys(_from='B', to='E')
        assert get_used_vj(r) == [['vj:1:2']]
        assert 'line_section_on_line_1' not in get_all_element_disruptions(r['journeys'], r)

        r = journeys(_from='C', to='A')
        assert get_used_vj(r) == [['vj:3']]
        assert 'line_section_on_line_1' not in get_all_element_disruptions(r['journeys'], r)

    def test_stop_schedule_impacted_by_line_section(self):
        """
        For /stop_schedules we display a line section impact if the stoppoint is part of a line section impact
        """
        cur = '_current_datetime=20170101T100000'
        dt = 'from_datetime=20170101T080000'
        fresh = 'data_freshness=base_schedule'
        r = self.query_region('stop_areas/A/stop_schedules?{cur}&{d}&{f}'.format(cur=cur, d=dt, f=fresh))
        assert 'line_section_on_line_1' not in get_all_element_disruptions(r['stop_schedules'], r)

        r = self.query_region('stop_areas/B/stop_schedules?{cur}&{d}&{f}'.format(cur=cur, d=dt, f=fresh))
        assert 'line_section_on_line_1' not in get_all_element_disruptions(r['stop_schedules'], r)

        r = self.query_region('stop_areas/C/stop_schedules?{cur}&{d}&{f}'.format(cur=cur, d=dt, f=fresh))
        d = get_all_element_disruptions(r['stop_schedules'], r)
        assert 'line_section_on_line_1' in d
        assert impacted_ids(d) == {'vj:1:1'} # the impact is linked in the response only to the stop_schedule.datetime

        r = self.query_region('stop_areas/D/stop_schedules?{cur}&{d}&{f}'.format(cur=cur, d=dt, f=fresh))
        d = get_all_element_disruptions(r['stop_schedules'], r)
        assert 'line_section_on_line_1' in d
        assert impacted_ids(d) == {'vj:1:1'}

        r = self.query_region('stop_areas/E/stop_schedules?{cur}&{d}&{f}'.format(cur=cur, d=dt, f=fresh))
        d = get_all_element_disruptions(r['stop_schedules'], r)
        assert 'line_section_on_line_1' in d
        assert impacted_ids(d) == {'vj:1:1'}

        r = self.query_region('stop_areas/F/stop_schedules?{cur}&{d}&{f}'.format(cur=cur, d=dt, f=fresh))
        d = get_all_element_disruptions(r['stop_schedules'], r)
        assert 'line_section_on_line_1' not in d

    def test_departures_impacted_by_line_section(self):
        """
        For /departures we display a line section impact if the stoppoint is part of a line section impact
        """
        cur = '_current_datetime=20170101T100000'
        dt = 'from_datetime=20170101T080000'
        fresh = 'data_freshness=base_schedule'
        r = self.query_region('stop_areas/A/departures?{cur}&{d}&{f}'.format(cur=cur, d=dt, f=fresh))
        assert 'line_section_on_line_1' not in get_all_element_disruptions(r['departures'], r)

        r = self.query_region('stop_areas/B/departures?{cur}&{d}&{f}'.format(cur=cur, d=dt, f=fresh))
        assert 'line_section_on_line_1' not in get_all_element_disruptions(r['departures'], r)

        r = self.query_region('stop_areas/C/departures?{cur}&{d}&{f}'.format(cur=cur, d=dt, f=fresh))
        d = get_all_element_disruptions(r['departures'], r)
        assert 'line_section_on_line_1' in d
        # the impact is linked in the response to the stop point and the vj
        assert impacted_ids(d) == {'vj:1:1'}

        r = self.query_region('stop_areas/D/departures?{cur}&{d}&{f}'.format(cur=cur, d=dt, f=fresh))
        d = get_all_element_disruptions(r['departures'], r)
        assert 'line_section_on_line_1' in d
        assert impacted_ids(d) == {'vj:1:1'}

        r = self.query_region('stop_areas/E/departures?{cur}&{d}&{f}'.format(cur=cur, d=dt, f=fresh))
        d = get_all_element_disruptions(r['departures'], r)
        assert 'line_section_on_line_1' in d
        assert impacted_ids(d) == {'vj:1:1'}

        r = self.query_region('stop_areas/F/departures?{cur}&{d}&{f}'.format(cur=cur, d=dt, f=fresh))
        d = get_all_element_disruptions(r['departures'], r)
        assert 'line_section_on_line_1' not in d

    def test_route_schedule_impacted_by_line_section(self):
        cur = '_current_datetime=20170101T100000'
        dt = 'from_datetime=20170101T080000'
        fresh = 'data_freshness=base_schedule'
        r = self.query_region('lines/line:1/departures?{cur}&{d}&{f}'.format(cur=cur, d=dt, f=fresh))
        d = get_all_element_disruptions(r['departures'], r)
        assert 'line_section_on_line_1' in d
        assert impacted_ids(d) == {'vj:1:1'}

    def test_line_impacted_by_line_section(self):
        assert self.has_disruption(ObjGetter('lines', 'line:1'), 'line_section_on_line_1')
        assert self.has_disruption(ObjGetter('lines', 'line:1'), 'line_section_on_line_1_other_effect')
