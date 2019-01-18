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

#pragma once

#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/serialization/weak_ptr.hpp>
#include <boost/serialization/serialization.hpp>
#include <boost/date_time/gregorian/greg_serialize.hpp>
#include <boost/date_time/posix_time/time_serialize.hpp>
#include <boost/serialization/bitset.hpp>
#include "utils/serialization_vector.h"
#include <boost/serialization/map.hpp>
#include <boost/serialization/set.hpp>
#include <boost/variant.hpp>
#include <boost/serialization/variant.hpp>
#include <boost/range/iterator_range.hpp>

#include <atomic>
#include <map>
#include <vector>
#include <string>

#include "utils/exception.h"
#include "utils/serialization_unique_ptr.h"
#include "utils/serialization_unique_ptr_container.h"

#include "type/type.h"

namespace navitia {
namespace type {
namespace disruption {

enum class Effect {
  NO_SERVICE = 0,
  REDUCED_SERVICE,
  SIGNIFICANT_DELAYS,
  DETOUR,
  ADDITIONAL_SERVICE,
  MODIFIED_SERVICE,
  OTHER_EFFECT,
  UNKNOWN_EFFECT,
  STOP_MOVED
};

enum class ChannelType {
    web = 0,
    sms,
    email,
    mobile,
    notification,
    twitter,
    facebook,
    unknown_type,
    title,
    beacon
};

inline std::string to_string(Effect effect) {
    switch (effect) {
    case Effect::NO_SERVICE: return "NO_SERVICE";
    case Effect::REDUCED_SERVICE: return "REDUCED_SERVICE";
    case Effect::SIGNIFICANT_DELAYS: return "SIGNIFICANT_DELAYS";
    case Effect::DETOUR: return "DETOUR";
    case Effect::ADDITIONAL_SERVICE: return "ADDITIONAL_SERVICE";
    case Effect::MODIFIED_SERVICE: return "MODIFIED_SERVICE";
    case Effect::OTHER_EFFECT: return "OTHER_EFFECT";
    case Effect::UNKNOWN_EFFECT: return "UNKNOWN_EFFECT";
    case Effect::STOP_MOVED: return "STOP_MOVED";
    default:
        throw navitia::exception("unhandled effect case");
    }
}

inline Effect from_string(const std::string& str) {
    if (str == "NO_SERVICE") { return Effect::NO_SERVICE; }
    if (str == "REDUCED_SERVICE") { return Effect::REDUCED_SERVICE; }
    if (str == "SIGNIFICANT_DELAYS") { return Effect::SIGNIFICANT_DELAYS; }
    if (str == "DETOUR") { return Effect::DETOUR; }
    if (str == "ADDITIONAL_SERVICE") { return Effect::ADDITIONAL_SERVICE; }
    if (str == "MODIFIED_SERVICE") { return Effect::MODIFIED_SERVICE; }
    if (str == "OTHER_EFFECT") { return Effect::OTHER_EFFECT; }
    if (str == "UNKNOWN_EFFECT") { return Effect::UNKNOWN_EFFECT; }
    if (str == "STOP_MOVED") { return Effect::STOP_MOVED; }
    throw navitia::exception("unhandled effect case");
}

inline std::string to_string(ChannelType ct) {
    switch (ct) {
    case ChannelType::web: return "web";
    case ChannelType::sms: return "sms";
    case ChannelType::email: return "email";
    case ChannelType::mobile: return "mobile";
    case ChannelType::notification: return "notification";
    case ChannelType::twitter: return "twitter";
    case ChannelType::facebook: return "facebook";
    case ChannelType::unknown_type: return "unknown_type";
    case ChannelType::title: return "title";
    case ChannelType::beacon: return "beacon";
    default:
        throw navitia::exception("unhandled channeltype case");
    }
}

struct Property {
    std::string key;
    std::string type;
    std::string value;

    bool operator< (const Property &right) const {
        if (key != right.key) { return key < right.key; }
        if (type != right.type) { return type < right.type; }
        return value < right.value;
    }

    template<class Archive>
    void serialize(Archive& ar, const unsigned int) {
        ar & key & type & value;
    }
};

struct Cause {
    std::string uri;
    std::string wording;
    std::string category;
    boost::posix_time::ptime created_at;
    boost::posix_time::ptime updated_at;

    template<class Archive>
    void serialize(Archive& ar, const unsigned int) {
        ar & uri & wording & created_at & updated_at & category;
    }
};

struct Severity {
    std::string uri;
    std::string wording;
    boost::posix_time::ptime created_at;
    boost::posix_time::ptime updated_at;
    std::string color;

    int priority;

    Effect effect = Effect::UNKNOWN_EFFECT;

    template<class Archive>
    void serialize(Archive& ar, const unsigned int) {
        ar & uri & wording & created_at & updated_at & color & priority & effect;
    }
};

struct UnknownPtObj {
    template<class Archive>
    void serialize(Archive&, const unsigned int) {}
};
struct LineSection {
    Line* line = nullptr;
    StopArea* start_point = nullptr;
    StopArea* end_point = nullptr;
    std::vector<Route*> routes;
    template<class Archive>
    void serialize(Archive& ar, const unsigned int) {
        ar & line & start_point & end_point & routes;
    }

    std::set<StopPoint*> get_stop_points_section() const {
        std::set<StopPoint*> res;
        for(const auto* route: routes) {
            route->for_each_vehicle_journey([&](const VehicleJourney& vj) {
                res = vj.get_sections_stop_points(start_point, end_point);
                if(res.empty()) {
                    return true;
                }
                return false;
            });
        }
        return res;
    }
};

typedef boost::variant<
    UnknownPtObj,
    Network*,
    StopArea*,
    StopPoint*,
    LineSection,
    Line*,
    Route*,
    MetaVehicleJourney*
    > PtObj;

PtObj make_pt_obj(Type_e type,
                  const std::string &uri,
                  PT_Data& pt_data);

struct Disruption;

struct Message {
    std::string text;
    std::string channel_id;
    std::string channel_name;
    std::string channel_content_type;

    boost::posix_time::ptime created_at;
    boost::posix_time::ptime updated_at;

    std::set<ChannelType> channel_types;

    template<class Archive>
    void serialize(Archive& ar, const unsigned int) {
        ar & text & created_at & updated_at & channel_id & channel_name & channel_content_type& channel_types;
    }
};

struct StopTimeUpdate {
    StopTime stop_time;
    std::string cause; //TODO factorize this cause with a pool
    enum class Status: uint8_t {
        // Note: status are ordered, from least to most important
        UNCHANGED = 0,
        ADDED,
        ADDED_FOR_DETOUR,
        DELETED,
        DELETED_FOR_DETOUR,
        DELAYED
    };
    Status departure_status{Status::UNCHANGED};
    Status arrival_status{Status::UNCHANGED};
    StopTimeUpdate() {}
    StopTimeUpdate(const StopTime& st, const std::string& c, Status dep_status, Status arr_status):
        stop_time(st), cause(c), departure_status(dep_status), arrival_status(arr_status) {}

    template<class Archive>
    void serialize(Archive& ar, const unsigned int) {
        ar & stop_time & cause & departure_status & arrival_status;
    }
};

inline bool operator<(StopTimeUpdate::Status a, StopTimeUpdate::Status b) {
    using T = std::underlying_type<StopTimeUpdate::Status>::type;
    return static_cast<T>(a) < static_cast<T>(b);
}

namespace detail {
struct AuxInfoForMetaVJ {
    std::vector<StopTimeUpdate> stop_times;
    // get the corresponding StopTime in the VJ associated to given StopTimeUpdate
    // returns null if nothing found
    const StopTime* get_vj_stop_time(const StopTimeUpdate&) const;

    template<class Archive>
    void serialize(Archive& ar, const unsigned int) {
        ar & stop_times;
    }
};
}

struct Impact {
    using SharedImpact = boost::shared_ptr<Impact>;
    std::string uri;
    std::string company_id;
    boost::posix_time::ptime created_at;
    boost::posix_time::ptime updated_at;

    // the application period define when the impact happen
    // i.e. the canceled base schedule period for vj
    std::vector<boost::posix_time::time_period> application_periods;

    boost::shared_ptr<Severity> severity;

    std::vector<Message> messages;

    detail::AuxInfoForMetaVJ aux_info;

    //link to the parent disruption
    //Note: it is a raw pointer because an Impact is owned by it's disruption
    //(even if the impact is stored as a share_ptr in the disruption to allow for weak_ptr towards it)
    Disruption* disruption;

    template<class Archive>
    void serialize(Archive& ar, const unsigned int) {
        ar & uri & company_id & created_at & updated_at & application_periods
           & severity & _informed_entities & messages & disruption
           & aux_info;
    }

    boost::iterator_range<std::vector<PtObj>::const_iterator> informed_entities() const {
        return boost::make_iterator_range(_informed_entities.begin(), _informed_entities.end());
    }
    boost::iterator_range<std::vector<PtObj>::iterator> mut_informed_entities() {
        return boost::make_iterator_range(_informed_entities.begin(), _informed_entities.end());
    }

    // add the ptobj to the enformed entities and make all the needed backref
    // Note: it's a static method because we need the shared_ptr to the impact
    static void link_informed_entity(PtObj ptobj, SharedImpact& impact, const boost::gregorian::date_period&, type::RTLevel);

    bool is_valid(const boost::posix_time::ptime& current_time, const boost::posix_time::time_period& action_period) const;
    bool is_relevant(const std::vector<const StopTime*>& stop_times) const;
    bool is_only_line_section() const;
    bool is_line_section_of(const Line&) const;
    Indexes get(Type_e target, const PT_Data & pt_data) const;
    const type::ValidityPattern get_impact_vp(const boost::gregorian::date_period& production_date) const;

    bool operator<(const Impact& other);

private:
    std::vector<PtObj> _informed_entities;
};

struct Tag {
    std::string uri;
    std::string name;
    boost::posix_time::ptime created_at;
    boost::posix_time::ptime updated_at;

    template<class Archive>
    void serialize(Archive& ar, const unsigned int) {
        ar & uri & name & created_at & updated_at;
    }
};

class DisruptionHolder;

struct Disruption {
    Disruption() {}
    Disruption(const std::string u, RTLevel lvl): uri(u), rt_level(lvl) {}
    Disruption& operator=(const Disruption&) = delete;
    Disruption(const Disruption&) = delete;

    std::string uri;
    // Provider of the disruption
    std::string contributor;
    // it's the title of the disruption as shown in the backoffice
    std::string reference;
    RTLevel rt_level = RTLevel::Adapted;

    // the publication period specify when an information can be displayed to
    // the customer, if a request is made before or after this period the
    // disruption must not be shown
    boost::posix_time::time_period publication_period {
        boost::posix_time::not_a_date_time, boost::posix_time::seconds(1)
    };//no default constructor for time_period, we must provide a value

    boost::posix_time::ptime created_at;
    boost::posix_time::ptime updated_at;

    boost::shared_ptr<Cause> cause;

    // the place where the disruption happen, the impacts can be in anothers places
    std::vector<PtObj> localization;

    //additional informations on the disruption
    std::vector<boost::shared_ptr<Tag>> tags;

    //properties linked to the disruption
    std::set<Property> properties;

    std::string note;

    template<class Archive>
    void serialize(Archive& ar, const unsigned int) {
        ar & uri & reference & rt_level & publication_period
           & created_at & updated_at & cause & impacts & localization & tags & note & contributor & properties;
    }

    void add_impact(const Impact::SharedImpact& impact, DisruptionHolder& holder);
    const std::vector<Impact::SharedImpact>& get_impacts() const {
        return impacts;
    }

    bool is_publishable(const boost::posix_time::ptime& current_time) const;

private:
    //Disruption have the ownership of the Impacts.  Impacts are
    //shared_ptr and not unique_ptr because there are weak_ptr
    //pointing to them in the impacted objects
    std::vector<Impact::SharedImpact> impacts;
};

class DisruptionHolder {
    std::map<std::string, std::unique_ptr<Disruption>> disruptions_by_uri;
    std::vector<boost::weak_ptr<Impact>> weak_impacts;
public:
    Disruption& make_disruption(const std::string& uri, type::RTLevel lvl);
    std::unique_ptr<Disruption> pop_disruption(const std::string& uri);
    const Disruption* get_disruption(const std::string& uri) const;
    size_t nb_disruptions() const { return disruptions_by_uri.size(); }
    void add_weak_impact(boost::weak_ptr<Impact>);
    void clean_weak_impacts();
    void forget_vj(const VehicleJourney*);
    const std::vector<boost::weak_ptr<Impact>>&
        get_weak_impacts() const{ return weak_impacts;}
    boost::weak_ptr<Impact> get_weak_impact(size_t id) const { return weak_impacts[id]; }
    boost::shared_ptr<Impact> get_impact(size_t id) const { return weak_impacts[id].lock(); }
    // causes, severities and tags are a pool (weak_ptr because the owner ship
    // is in the linked disruption or impact)
    std::map<std::string, boost::weak_ptr<Cause>> causes; //to be wrapped
    std::map<std::string, boost::weak_ptr<Severity>> severities; //to be wrapped too
    std::map<std::string, boost::weak_ptr<Tag>> tags; //to be wrapped too

    template<class Archive>
    void serialize(Archive& ar, const unsigned int) {
        ar & disruptions_by_uri & causes & severities & tags & weak_impacts;
    }
};

struct ImpactedVJ {
    const VehicleJourney* vj; // vj before impact
    ValidityPattern new_vp;
    std::set<StopPoint*> impacted_stops;
    ImpactedVJ(const VehicleJourney* vj,
               ValidityPattern vp,
               std::set<StopPoint*> r):
        vj(vj), new_vp(vp), impacted_stops(std::move(r)) {}
};
/*
 * return the list of vehicle journey that are impacted by the linesection
 */
std::vector<ImpactedVJ> get_impacted_vehicle_journeys(const LineSection&,
                                                      const Impact&,
                                                      const boost::gregorian::date_period&,
                                                      type::RTLevel);

}

}}//namespace
