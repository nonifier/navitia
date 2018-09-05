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
import serpy
from .base import LiteralField, NestedPropertyField, IntNestedPropertyField, value_by_path, \
    BetaEndpointsSerializer
import logging
from jormungandr.interfaces.v1.serializer import jsonschema
from jormungandr.interfaces.v1.fields import raw_feed_publisher_bano, raw_feed_publisher_osm
from jormungandr.interfaces.v1.serializer.base import NestedDictGenericField, NestedDictCodeField, NestedPropertiesField, NestedDictCommentField
from jormungandr.utils import get_house_number
from jormungandr.autocomplete.geocodejson import create_address_field, get_lon_lat


class CoordField(jsonschema.Field):
    def __init__(self, schema_type=None, schema_metadata={}, **kwargs):
        schema_metadata.update({
            "type": "object",
            "properties": {
                "lat": { "type": ["string", "null"] },
                "lon": { "type": ["string", "null"] }
            },
            "required": ["lat", "lon"]
        })
        super(CoordField, self).__init__(schema_type, schema_metadata, **kwargs)

    def as_getter(self, serializer_field_name, serializer_cls):
        return lambda obj: self.generate_coord_field(obj)

    def generate_coord_field(self, obj):
        coords = value_by_path(obj, 'geometry.coordinates')
        res = {'lat': None, 'lon': None}
        if coords and len(coords) >= 2:
            res.update({'lat': str(coords[1]), 'lon': str(coords[0])})
        return res


class CoordId(jsonschema.Field):
    def __init__(self, schema_type=None, schema_metadata={}, **kwargs):
        schema_metadata.update({
            "type": ["string", "null"]
        })
        super(CoordId, self).__init__(schema_type, schema_metadata, **kwargs)

    def as_getter(self, serializer_field_name, serializer_cls):
        return lambda obj: self.generate_coord_id(obj)

    def generate_coord_id(self, obj):
        coords = value_by_path(obj, 'geometry.coordinates')
        if coords and len(coords) >= 2:
            return '{};{}'.format(coords[0], coords[1])
        return None


class AdministrativeRegionsSerializer(serpy.Field):
    def as_getter(self, serializer_field_name, serializer_cls):
        return lambda obj: self.make(obj)

    def make(self, obj):
        admins = value_by_path(obj, 'properties.geocoding.administrative_regions', [])
        if admins:
            def make_admin(admin):
                res = {
                    'id': admin['id'],
                    'insee': admin['insee'],
                    'name': admin['name'],
                    'label': admin['label'],
                    'level': admin['level'],
                    'coord': {
                        'lon': str(admin['coord']['lon']),
                        'lat': str(admin['coord']['lat']),
                    },
                }
                zip_codes = admin.get('zip_codes', [])
                if all(zip_code == "" for zip_code in zip_codes):
                    pass
                elif len(zip_codes) == 1:
                    res['zip_code'] = zip_codes[0]
                else:
                    res['zip_code'] = '{}-{}'.format(min(zip_codes), max(zip_codes))
                return res
            return [make_admin(admin) for admin in admins]
        admins = obj.get('properties', {}).get('geocoding', {}).get('admin', {})
        return [
            {
                "insee": None,
                "name": name,
                "level": int(level.replace('level', '')),
                "coord": {"lat": None, "lon": None},
                "label": None,
                "id": None,
                "zip_code": None
            }
            for level, name in admins.items()
        ]


class AdministrativeRegionSerializer(serpy.DictSerializer):
    id = NestedPropertyField(attr='properties.geocoding.id', display_none=True)
    name = NestedPropertyField(attr='properties.geocoding.name', display_none=True)
    label = NestedPropertyField(attr='properties.geocoding.label', display_none=True)
    zip_code = NestedPropertyField(attr='properties.geocoding.postcode')
    coord = CoordField()
    insee = NestedPropertyField(attr='properties.geocoding.citycode')
    level = IntNestedPropertyField(attr='properties.geocoding.level')
    administrative_regions = AdministrativeRegionsSerializer(display_none=False)


class GeocodeAdminSerializer(serpy.DictSerializer):
    id = NestedPropertyField(attr='properties.geocoding.id', display_none=True)
    name = NestedPropertyField(attr='properties.geocoding.name', display_none=True)
    quality = LiteralField(0, deprecated=True)
    embedded_type = LiteralField("administrative_region", display_none=True)
    administrative_region = jsonschema.MethodField()
    distance = IntNestedPropertyField(attr='distance', display_none=False)

    def get_administrative_region(self, obj):
        return AdministrativeRegionSerializer(obj).data


class PoiTypeSerializer(serpy.DictSerializer):
    id = serpy.StrField(display_none=True)
    name = serpy.StrField(display_none=True)


class PoiSerializer(serpy.DictSerializer):
    id = NestedPropertyField(attr='properties.geocoding.id', display_none=True)
    coord = CoordField()
    label = NestedPropertyField(attr='properties.geocoding.label', display_none=True)
    name = NestedPropertyField(attr='properties.geocoding.name', display_none=True)
    administrative_regions = AdministrativeRegionsSerializer(display_none=False)
    poi_type = jsonschema.MethodField(display_none=False)
    properties = jsonschema.MethodField(display_none=False)
    address = jsonschema.MethodField(display_none=False)

    def get_poi_type(self, obj):
        poi_types = obj.get('properties', {}).get('geocoding', {}).get('poi_types', [])
        return PoiTypeSerializer(poi_types[0]).data if isinstance(poi_types, list) and poi_types else None

    def get_properties(self, obj):
        return {p.get("key"): p.get("value") 
                for p in obj.get('properties', {}).get('geocoding', {}).get('properties', [])}

    def get_address(self, obj):
        address = obj.get('properties', {}).get('geocoding', {}).get('address', None)
        if not address:
            return None
        poi_lon, poi_lat = get_lon_lat(obj)
        return create_address_field(address, poi_lat=poi_lat, poi_lon=poi_lon)


class GeocodePoiSerializer(serpy.DictSerializer):
    embedded_type = LiteralField("poi", display_none=True)
    quality = LiteralField(0, deprecated=True)
    id = NestedPropertyField(attr='properties.geocoding.id', display_none=True)
    name = NestedPropertyField(attr='properties.geocoding.label', display_none=True)
    poi = jsonschema.MethodField()
    distance = IntNestedPropertyField(attr='distance', display_none=False)

    def get_poi(self, obj):
        return PoiSerializer(obj).data


class AddressSerializer(serpy.DictSerializer):
    id = CoordId(display_none=True)
    coord = CoordField()
    house_number = jsonschema.MethodField(display_none=True)
    label = NestedPropertyField(attr='properties.geocoding.label', display_none=True)
    name = NestedPropertyField(attr='properties.geocoding.name', display_none=True)
    administrative_regions = AdministrativeRegionsSerializer(display_none=False)

    def get_house_number(self, obj):
        geocoding = obj.get('properties', {}).get('geocoding', {})
        return get_house_number(geocoding.get('housenumber'))


class GeocodeAddressSerializer(serpy.DictSerializer):
    embedded_type = LiteralField("address", display_none=True)
    quality = LiteralField(0, deprecated=True)
    id = CoordId(display_none=True)
    name = NestedPropertyField(attr='properties.geocoding.label', display_none=True)
    address = jsonschema.MethodField()
    distance = IntNestedPropertyField(attr='distance', display_none=False)

    def get_address(self, obj):
        return AddressSerializer(obj).data


class StopAreaSerializer(serpy.DictSerializer):
    id = NestedPropertyField(attr='properties.geocoding.id', display_none=True)
    coord = CoordField()
    label = NestedPropertyField(attr='properties.geocoding.label', display_none=True)
    name = NestedPropertyField(attr='properties.geocoding.name', display_none=True)
    administrative_regions = AdministrativeRegionsSerializer(display_none=False)
    timezone = NestedPropertyField(attr='properties.geocoding.timezone')
    commercial_modes = NestedDictGenericField(attr='properties.geocoding.commercial_modes', many=True)
    physical_modes = NestedDictGenericField(attr='properties.geocoding.physical_modes', many=True)
    comments = NestedDictCommentField(attr='properties.geocoding.comments', many=True)
    comment = jsonschema.MethodField(display_none=True)
    codes = NestedDictCodeField(attr='properties.geocoding.codes', many=True)
    properties = NestedPropertiesField(attr='properties.geocoding.properties', display_none=False)

    def get_comment(self, obj):
        # To be compatible with old version, we create the "comment" field in addition.
        # This field is a simple string, so we take only one comment (In our case, the first
        # element of the list).
        comments = obj.get('properties', {}).get('geocoding', {}).get('comments')
        if comments:
            return next(iter(comments or []), None).get('name')


class GeocodeStopAreaSerializer(serpy.DictSerializer):
    embedded_type = LiteralField("stop_area", display_none=True)
    quality = LiteralField(0, deprecated=True)
    id = NestedPropertyField(attr='properties.geocoding.id', display_none=True)
    name = NestedPropertyField(attr='properties.geocoding.label', display_none=True)
    stop_area = jsonschema.MethodField()
    distance = IntNestedPropertyField(attr='distance', display_none=False)

    def get_stop_area(self, obj):
        return StopAreaSerializer(obj).data


class GeocodePlacesSerializer(serpy.DictSerializer):
    places = jsonschema.MethodField(display_none=True)
    warnings = BetaEndpointsSerializer()
    feed_publishers = jsonschema.MethodField()

    def get_places(self, obj):
        map_serializer = {
            'city': GeocodeAdminSerializer,
            'administrative_region': GeocodeAdminSerializer,
            'street': GeocodeAddressSerializer,
            'house': GeocodeAddressSerializer,
            'poi': GeocodePoiSerializer,
            'public_transport:stop_area': GeocodeStopAreaSerializer
        }
        res = []
        for feature in obj.get('features', []):
            type_ = feature.get('properties', {}).get('geocoding', {}).get('type')
            if not type_ or type_ not in map_serializer:
                logging.getLogger(__name__).error('Place not serialized (unknown type): {}'.format(feature))
                continue
            res.append(map_serializer[type_](feature).data)
        return res

    def get_feed_publishers(self, obj):
        fp = []
        for feature in obj.get('features', []):
            feed_pubs = feature.get('properties', {}).get('geocoding', {}).get('feed_publishers')
            if feed_pubs:
                [fp.append(x) for x in feed_pubs if x not in fp]
        # By default, keep BANO & OSM as feed publishers
        fp.extend([raw_feed_publisher_bano, raw_feed_publisher_osm])
        return fp
