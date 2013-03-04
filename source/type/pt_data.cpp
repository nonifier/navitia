#include "pt_data.h"

namespace navitia{namespace type {


PT_Data& PT_Data::operator=(PT_Data&& other){
    validity_patterns = other.validity_patterns;
    lines = other.lines;
    journey_patterns = other.journey_patterns;
    routes = other.routes;
    vehicle_journeys = other.vehicle_journeys;
    stop_points = other.stop_points;
    stop_areas = other.stop_areas;
    stop_times = other.stop_times;

    networks = other.networks;
    physical_modes = other.physical_modes;
    commercial_modes = other.commercial_modes;
    cities = other.cities;
    connections = other.connections;
    journey_pattern_points = other.journey_pattern_points;

    districts = other.districts;
    departments = other.departments;
    companies = other.companies;
    countries = other.countries;

    stop_point_connections = other.stop_point_connections;

    // First letter
    stop_area_autocomplete = other.stop_area_autocomplete;
    city_autocomplete = other.city_autocomplete;
    stop_point_autocomplete = other.stop_point_autocomplete;

    // Proximity list
    stop_area_proximity_list = other.stop_area_proximity_list;
    stop_point_proximity_list = other.stop_point_proximity_list;
    city_proximity_list = other.city_proximity_list;

    line_map = other.line_map;
    journey_pattern_map = other.journey_pattern_map;
    vehicle_journey_map = other.vehicle_journey_map;
    stop_area_map = other.stop_area_map;
    stop_point_map = other.stop_point_map;
    network_map = other.network_map;
    physical_mode_map = other.physical_mode_map;
    commercial_mode_map = other.commercial_mode_map;
    city_map = other.city_map;
    district_map = other.district_map;
    department_map = other.department_map;
    company_map = other.company_map;
    country_map = other.country_map;

    return *this;
}


std::vector<idx_t> PT_Data::get_target_by_source(Type_e source, Type_e target, std::vector<idx_t> source_idx) const {
    std::vector<idx_t> result;
    result.reserve(source_idx.size());
    for(idx_t idx : source_idx) {
        std::vector<idx_t> tmp;
        tmp = get_target_by_one_source(source, target, idx);
        result.insert(result.end(), tmp.begin(), tmp.end());

    }
    return result;
}

std::vector<idx_t> PT_Data::get_target_by_one_source(Type_e source, Type_e target, idx_t source_idx) const {
    std::vector<idx_t> result;
    if(source_idx == invalid_idx)
        return result;
    if(source == target){
        result.push_back(source_idx);
        return result;
    }
    switch(source) {
        case Type_e::eLine: result = lines[source_idx].get(target, *this); break;
        case Type_e::eJourneyPattern: result = journey_patterns[source_idx].get(target, *this); break;
        case Type_e::eVehicleJourney: result = vehicle_journeys[source_idx].get(target, *this); break;
        case Type_e::eStopPoint: result = stop_points[source_idx].get(target, *this); break;
        case Type_e::eStopArea: result = stop_areas[source_idx].get(target, *this); break;
        case Type_e::eNetwork: result = networks[source_idx].get(target, *this); break;
        case Type_e::ePhysicalMode: result = physical_modes[source_idx].get(target, *this); break;
        case Type_e::eCommercialMode: result = commercial_modes[source_idx].get(target, *this); break;
        case Type_e::eCity: result = cities[source_idx].get(target, *this); break;
        case Type_e::eDistrict: result = districts[source_idx].get(target, *this); break;
        case Type_e::eDepartment: result = departments[source_idx].get(target, *this); break;
        case Type_e::eCompany: result = companies[source_idx].get(target, *this); break;
        case Type_e::eValidityPattern: result = validity_patterns[source_idx].get(target, *this); break;
        case Type_e::eConnection: result = connections[source_idx].get(target, *this); break;
        case Type_e::eCountry: result = countries[source_idx].get(target, *this); break;
        case Type_e::eJourneyPatternPoint: result = journey_pattern_points[source_idx].get(target, *this); break;
        case Type_e::eRoute: result = routes[source_idx].get(target, *this); break;
        default: break;
    }
    return result;
}

std::vector<idx_t> PT_Data::get_all_index(Type_e type) const {
    size_t num_elements = 0;
    switch(type){
    case Type_e::eLine: num_elements = lines.size(); break;
    case Type_e::eValidityPattern: num_elements = validity_patterns.size(); break;
    case Type_e::eJourneyPattern: num_elements = journey_patterns.size(); break;
    case Type_e::eVehicleJourney: num_elements = vehicle_journeys.size(); break;
    case Type_e::eStopPoint: num_elements = stop_points.size(); break;
    case Type_e::eStopArea: num_elements = stop_areas.size(); break;
    case Type_e::eStopTime: num_elements = stop_times.size(); break;
    case Type_e::eNetwork: num_elements = networks.size(); break;
    case Type_e::ePhysicalMode: num_elements = physical_modes.size(); break;
    case Type_e::eCommercialMode: num_elements = commercial_modes.size(); break;
    case Type_e::eCity: num_elements = cities.size(); break;
    case Type_e::eConnection: num_elements = connections.size(); break;
    case Type_e::eJourneyPatternPoint: num_elements = journey_pattern_points.size(); break;
    case Type_e::eDistrict: num_elements = districts.size(); break;
    case Type_e::eDepartment: num_elements = departments.size(); break;
    case Type_e::eCompany: num_elements = companies.size(); break;
    case Type_e::eCountry: num_elements = countries.size(); break;
    case Type_e::eRoute: num_elements = routes.size(); break;
    default:  break;
    }
    std::vector<idx_t> indexes(num_elements);
    for(size_t i=0; i < num_elements; i++)
        indexes[i] = i;
    return indexes;
}

template<> std::vector<Line> & PT_Data::get_data<Line>() {return lines;}
template<> std::vector<ValidityPattern> & PT_Data::get_data<ValidityPattern>() {return validity_patterns;}
template<> std::vector<JourneyPattern> & PT_Data::get_data<JourneyPattern>() {return journey_patterns;}
template<> std::vector<VehicleJourney> & PT_Data::get_data<VehicleJourney>() {return vehicle_journeys;}
template<> std::vector<StopPoint> & PT_Data::get_data<StopPoint>() {return stop_points;}
template<> std::vector<StopArea> & PT_Data::get_data<StopArea>() {return stop_areas;}
template<> std::vector<StopTime> & PT_Data::get_data<StopTime>() {return stop_times;}
template<> std::vector<Network> & PT_Data::get_data<Network>() {return networks;}
template<> std::vector<PhysicalMode> & PT_Data::get_data<PhysicalMode>() {return physical_modes;}
template<> std::vector<CommercialMode> & PT_Data::get_data<CommercialMode>() {return commercial_modes;}
template<> std::vector<City> & PT_Data::get_data<City>() {return cities;}
template<> std::vector<Connection> & PT_Data::get_data<Connection>() {return connections;}
template<> std::vector<JourneyPatternPoint> & PT_Data::get_data<JourneyPatternPoint>() {return journey_pattern_points;}
template<> std::vector<District> & PT_Data::get_data<District>() {return districts;}
template<> std::vector<Department> & PT_Data::get_data<Department>() {return departments;}
template<> std::vector<Company> & PT_Data::get_data<Company>() {return companies;}
template<> std::vector<Country> & PT_Data::get_data<Country>() {return countries;}
template<> std::vector<Route> & PT_Data::get_data<Route>() {return routes;}

template<> std::vector<Line> const & PT_Data::get_data<Line>() const {return lines;}
template<> std::vector<ValidityPattern> const & PT_Data::get_data<ValidityPattern>() const {return validity_patterns;}
template<> std::vector<JourneyPattern> const & PT_Data::get_data<JourneyPattern>() const {return journey_patterns;}
template<> std::vector<VehicleJourney> const & PT_Data::get_data<VehicleJourney>() const {return vehicle_journeys;}
template<> std::vector<StopPoint> const & PT_Data::get_data<StopPoint>() const {return stop_points;}
template<> std::vector<StopArea> const & PT_Data::get_data<StopArea>() const {return stop_areas;}
template<> std::vector<StopTime> const & PT_Data::get_data<StopTime>() const {return stop_times;}
template<> std::vector<Network> const & PT_Data::get_data<Network>() const {return networks;}
template<> std::vector<PhysicalMode> const & PT_Data::get_data<PhysicalMode>() const {return physical_modes;}
template<> std::vector<CommercialMode> const & PT_Data::get_data<CommercialMode>() const {return commercial_modes;}
template<> std::vector<City> const & PT_Data::get_data<City>() const {return cities;}
template<> std::vector<Connection> const & PT_Data::get_data<Connection>() const {return connections;}
template<> std::vector<JourneyPatternPoint> const & PT_Data::get_data<JourneyPatternPoint>() const {return journey_pattern_points;}
template<> std::vector<District> const & PT_Data::get_data<District>() const {return districts;}
template<> std::vector<Department> const & PT_Data::get_data<Department>() const {return departments;}
template<> std::vector<Company> const & PT_Data::get_data<Company>() const {return companies;}
template<> std::vector<Country> const & PT_Data::get_data<Country>() const {return countries;}
template<> std::vector<Route> const & PT_Data::get_data<Route>() const {return routes;}


void PT_Data::build_autocomplete(){
    for(const StopArea & sa : this->stop_areas){
        if(sa.city_idx < this->cities.size())
            this->stop_area_autocomplete.add_string(sa.name + " " + cities[sa.city_idx].name, sa.idx);
        else
            this->stop_area_autocomplete.add_string(sa.name, sa.idx);
    }

    this->stop_area_autocomplete.build();

    for(const StopPoint & sp : this->stop_points){
        if(sp.city_idx < this->cities.size())
            this->stop_point_autocomplete.add_string(sp.name + " " + cities[sp.city_idx].name, sp.idx);
        else
            this->stop_point_autocomplete.add_string(sp.name, sp.idx);
    }

    this->stop_point_autocomplete.build();


    for(const City & city : cities){
        this->city_autocomplete.add_string(city.name, city.idx);
    }
    this->city_autocomplete.build();
}

void PT_Data::build_proximity_list() {
    for(const City & city : this->cities){
        this->city_proximity_list.add(city.coord, city.idx);
    }
    this->city_proximity_list.build();

    for(const StopArea &stop_area : this->stop_areas){
        this->stop_area_proximity_list.add(stop_area.coord, stop_area.idx);
    }
    this->stop_area_proximity_list.build();

    for(const StopPoint & stop_point : this->stop_points){
        this->stop_point_proximity_list.add(stop_point.coord, stop_point.idx);
    }
    this->stop_point_proximity_list.build();
}

void PT_Data::build_uri() {
    normalize_extcode<Line>(line_map);
    normalize_extcode<JourneyPattern>(journey_pattern_map);
    normalize_extcode<VehicleJourney>(vehicle_journey_map);
    normalize_extcode<StopArea>(stop_area_map);
    normalize_extcode<StopPoint>(stop_point_map);
    normalize_extcode<Network>(network_map);
    normalize_extcode<PhysicalMode>(physical_mode_map);
    normalize_extcode<CommercialMode>(commercial_mode_map);
    normalize_extcode<City>(city_map);
    normalize_extcode<District>(district_map);
    normalize_extcode<Department>(department_map);
    normalize_extcode<Company>(company_map);
    normalize_extcode<Country>(country_map);
    normalize_extcode<Route>(routes_map);
}


void PT_Data::build_connections() {
    stop_point_connections.resize(stop_points.size());
    for(Connection &conn : this->connections){
        const StopPoint & dep = this->stop_points[conn.departure_stop_point_idx];
        const StopPoint & arr = this->stop_points[conn.destination_stop_point_idx];
        if(dep.stop_area_idx == arr.stop_area_idx)
            conn.connection_type = eStopAreaConnection;
        else
            conn.connection_type = eWalkingConnection;
        stop_point_connections[dep.idx].push_back(conn);
    }
}

}}
