#pragma once
#include "type/pb_converter.h"
namespace navitia { namespace timetables {


pbnavitia::Response next_departures(const std::string &request,
        const std::vector<std::string>& forbidden_uris,
        const std::string &str_dt, uint32_t duration, uint32_t nb_stoptimes,
        const int depth, const type::AccessibiliteParams & accessibilite_params,
        type::Data & data, bool disruption_active, uint32_t count, uint32_t start_page,
        const bool show_codes);
pbnavitia::Response next_arrivals(const std::string &request,
        const std::vector<std::string>& forbidden_uris,
        const std::string &str_dt, uint32_t duration, uint32_t nb_stoptimes,
        const int depth, const type::AccessibiliteParams & accessibilite_params,
        type::Data & data, bool disruption_active, uint32_t count, uint32_t start_page,
        const bool show_codes);

}}
