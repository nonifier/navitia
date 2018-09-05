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

#include "ptreferential_ng.h"
#include "ptreferential_utils.h"
#include "ptreferential.h"
#include "type/pt_data.h"

#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/phoenix.hpp>

namespace qi = boost::spirit::qi;
namespace ascii = boost::spirit::ascii;
using navitia::type::Indexes;
using navitia::type::Type_e;
using navitia::type::OdtLevel_e;

namespace navitia{
namespace ptref{

namespace {

template <typename Iterator>
struct PtRefGrammar: qi::grammar<Iterator, ast::Expr(), ascii::space_type> {
    PtRefGrammar(): PtRefGrammar::base_type(expr) {
        using namespace qi::labels;
        using boost::spirit::_1;
        using boost::spirit::_2;
        namespace phx = boost::phoenix;

        pred = all | empty | fun | cmp | dwithin;
        all = qi::lit("all")[_val = phx::construct<ast::All>()];
        empty = qi::lit("empty")[_val = phx::construct<ast::Empty>()];
        fun = ident >> '.' >> ident >> '(' >> -(str % ',') >> ')';
        cmp = ident[phx::at_c<0>(_val) = _1]
            >> '.'
            >> ident[phx::at_c<1>(_val) = _1]
            >> '='
            >> str[phx::push_back(phx::at_c<2>(_val), _1)];
        dwithin = ident[phx::at_c<0>(_val) = _1]
            >> '.'
            >> "coord"
            >> qi::lit("DWITHIN")[phx::at_c<1>(_val) = "within"]
            >> '('
            >> str[phx::push_back(phx::at_c<2>(_val), ""),
                   phx::push_back(phx::at_c<2>(_val), _1)]
            >> ','
            >> str[phx::back(phx::at_c<2>(_val)) += ";",
                   phx::back(phx::at_c<2>(_val)) += _1]
            >> ','
            >> str[phx::at(phx::at_c<2>(_val), 0) = _1]
            >> ')';

        get_corresponding = (qi::lit("get") | qi::lit("GET")) >> ident >> "<-" >> expr;
        expr_leaf = '(' >> expr >> ')' | pred | get_corresponding;
        expr_diff = (expr_leaf >> '-' >> expr_diff)
            [_val = phx::construct<ast::BinaryOp<ast::Diff>>(_1, _2)]
            | expr_leaf[_val = _1];
        expr_and = (expr_diff >> (qi::lit("AND") | qi::lit("and")) >> expr_and)
            [_val = phx::construct<ast::BinaryOp<ast::And>>(_1, _2)]
            | expr_diff[_val = _1];
        expr_or = (expr_and >> (qi::lit("OR") | qi::lit("or")) >> expr_or)
            [_val = phx::construct<ast::BinaryOp<ast::Or>>(_1, _2)]
            | expr_and[_val = _1];
        expr = expr_or;

        ident = qi::lexeme[qi::alpha >> *(qi::alnum | qi::char_("_"))];
        str = qi::lexeme[+(qi::alnum | qi::char_("_.:;|-"))]
            | qi::lexeme['"' > (*(qi::char_ - qi::char_("\"\\") | ('\\' > qi::char_))) > '"'];
    }

    // Pred
    qi::rule<Iterator, ast::Expr(), ascii::space_type> pred;
    qi::rule<Iterator, ast::All(), ascii::space_type> all;
    qi::rule<Iterator, ast::Empty(), ascii::space_type> empty;
    qi::rule<Iterator, ast::Fun(), ascii::space_type> fun;
    qi::rule<Iterator, ast::Fun(), ascii::space_type> cmp;
    qi::rule<Iterator, ast::Fun(), ascii::space_type> dwithin;

    // Expr
    qi::rule<Iterator, ast::GetCorresponding(), ascii::space_type> get_corresponding;
    qi::rule<Iterator, ast::Expr(), ascii::space_type> expr_leaf;
    qi::rule<Iterator, ast::Expr(), ascii::space_type> expr_and;
    qi::rule<Iterator, ast::Expr(), ascii::space_type> expr_or;
    qi::rule<Iterator, ast::Expr(), ascii::space_type> expr_diff;
    qi::rule<Iterator, ast::Expr(), ascii::space_type> expr;

    // lexemes
    qi::rule<Iterator, std::string(), ascii::space_type> ident;
    qi::rule<Iterator, std::string(), ascii::space_type> str;
};

const char* to_string(OdtLevel_e level) {
    switch (level) {
    case type::OdtLevel_e::scheduled: return "scheduled";
    case type::OdtLevel_e::with_stops: return "with_stops";
    case type::OdtLevel_e::zonal: return "zonal";
    default: return "all";
    }
}

OdtLevel_e odt_level_from_string(const std::string& s) {
    if (s == "scheduled") {
        return OdtLevel_e::scheduled;
    } else if (s == "with_stops") {
        return OdtLevel_e::with_stops;
    } else if (s == "zonal") {
        return OdtLevel_e::zonal;
    } else {
        return OdtLevel_e::all;
    }
}

boost::posix_time::ptime from_datetime(const std::string& s) {
    if (s.empty()) {
        throw parsing_error(parsing_error::partial_error, "empty datetime is illegal");
    }
    if (s.back() != 'Z') {
        throw parsing_error(parsing_error::partial_error,
                            "only UTC datetime are allowed (must finish with `Z`): " + s);
    }
    try {
        return boost::posix_time::from_iso_string(s.substr(0, s.size() - 1));
    } catch (...) {
        throw parsing_error(parsing_error::partial_error, "failed to parse datetime: " + s);
    }
}

struct Eval: boost::static_visitor<Indexes> {
    const Type_e target;
    const type::Data& data;
    Eval(Type_e t, const type::Data& d): target(t), data(d) {}

    Indexes operator()(const ast::All&) const {
        return data.get_all_index(target);
    }
    Indexes operator()(const ast::Empty&) const {
        return Indexes();
    }
    Indexes operator()(const ast::Fun& f) const {
        Indexes indexes;
        if (f.type == "vehicle_journey" && f.method == "has_headsign" && f.args.size() == 1) {
            for (auto vj: data.pt_data->headsign_handler.get_vj_from_headsign(f.args.at(0))) {
                indexes.insert(vj->idx);
            }
        } else if (f.type == "vehicle_journey" && f.method == "has_disruption" && f.args.size() == 0) {
            indexes = get_indexes_by_impacts(type::Type_e::VehicleJourney, data);
        } else if (f.type == "line" && f.method == "code" && f.args.size() == 1) {
            for (auto l: data.pt_data->lines) {
                if (l->code != f.args[0]) { continue; }
                indexes.insert(l->idx);
            }
        } else if (f.type == "line" && f.method == "odt_level" && f.args.size() == 1) {
            const auto level = odt_level_from_string(f.args.at(0));
            for (auto l: data.pt_data->lines) {
                const auto properties = l->get_odt_properties();
                switch (level) {
                case OdtLevel_e::scheduled:
                    if (properties.is_scheduled()) { indexes.insert(l->idx); }
                    break;
                case OdtLevel_e::with_stops:
                    if (properties.is_with_stops()) { indexes.insert(l->idx); }
                    break;
                case OdtLevel_e::zonal:
                    if (properties.is_zonal()) { indexes.insert(l->idx); }
                    break;
                default:
                    indexes.insert(l->idx);
                }
            }
        } else if (f.type == "disruption"
                   && ((f.method == "tag"  && f.args.size() == 1)
                    || (f.method == "tags" && f.args.size() >= 1))) {
            indexes = get_impacts_by_tags(f.args, data);
        } else if ((f.method == "since" && f.args.size() == 1)
                   || (f.method == "until" && f.args.size() == 1)
                   || (f.method == "between" && f.args.size() == 2)) {
            boost::optional<boost::posix_time::ptime> since, until;
            if (f.method == "since") {
                since = from_datetime(f.args.at(0));
            } else if (f.method == "until") {
                until = from_datetime(f.args.at(0));
            } else if (f.method == "between") {
                since = from_datetime(f.args.at(0));
                until = from_datetime(f.args.at(1));
            }
            const auto type = type_by_caption(f.type);
            indexes = filter_on_period(data.get_all_index(type), type, since, until, data);
        } else if (f.method == "within" && f.args.size() == 2) {
            double distance = 0.;
            try {
                distance = boost::lexical_cast<double>(f.args.at(0));
            } catch (...) {
                throw parsing_error(parsing_error::partial_error, "invalid distance " + f.args.at(0));
            }
            type::GeographicalCoord coord;
            try {
                std::vector<std::string> splited;
                boost::algorithm::split(splited, f.args.at(1), boost::algorithm::is_any_of(";"));
                if (splited.size() != 2) { throw 0; }
                coord = type::GeographicalCoord(boost::lexical_cast<double>(splited[0]),
                                                boost::lexical_cast<double>(splited[1]));
            } catch (...) {
                throw parsing_error(parsing_error::partial_error, "invalid coord " + f.args.at(1));
            }
            indexes = get_within(type_by_caption(f.type), coord, distance, data);
        } else if ((f.method == "id" || f.method == "uri") && f.args.size() == 1) {
            indexes = get_indexes_from_id(type_by_caption(f.type), f.args.at(0), data);
        } else if (f.method == "name" && f.args.size() == 1) {
            indexes = get_indexes_from_name(type_by_caption(f.type), f.args.at(0), data);
        } else if (f.method == "has_code" && f.args.size() == 2) {
            indexes = get_indexes_from_code(type_by_caption(f.type), f.args.at(0), f.args.at(1), data);
        } else {
            std::stringstream ss;
            ss << "Unknown function: " << f;
            throw parsing_error(parsing_error::partial_error, ss.str());
        }
        return get_corresponding(indexes, type_by_caption(f.type), target, data);
    }
    Indexes operator()(const ast::GetCorresponding& expr) const {
        const auto from = type_by_caption(expr.type);
        auto indexes = boost::apply_visitor(Eval(from, data), expr.expr);
        return get_corresponding(indexes, from, target, data);
    }
    Indexes operator()(const ast::BinaryOp<ast::And>& expr) const {
        return get_intersection(boost::apply_visitor(*this, expr.lhs),
                                boost::apply_visitor(*this, expr.rhs));
    }
    Indexes operator()(const ast::BinaryOp<ast::Diff>& expr) const {
        return get_difference(boost::apply_visitor(*this, expr.lhs),
                              boost::apply_visitor(*this, expr.rhs));
    }
    Indexes operator()(const ast::BinaryOp<ast::Or>& expr) const {
        auto res = boost::apply_visitor(*this, expr.lhs);
        const auto other = boost::apply_visitor(*this, expr.rhs);
        res.insert(boost::container::ordered_unique_range_t(), other.begin(), other.end());
        return res;
    }
};

} // anonymous namespace

ast::Expr parse(const std::string& request) {
    using boost::spirit::ascii::space;
    static const PtRefGrammar<std::string::const_iterator> grammar;

    ast::Expr expr;
    auto iter = request.begin(), end = request.end();
    if (phrase_parse(iter, end, grammar, space, expr)) {
        if(iter != end) {
            const std::string unparsed(iter, end);
            throw parsing_error(parsing_error::partial_error, "Filter: Unable to parse the whole string. Not parsed: >>" + unparsed + "<<");
        }
    } else {
        throw parsing_error(parsing_error::global_error, "Filter: unable to parse " + request);
    }
    return expr;
}

std::string make_request(const Type_e requested_type,
                         const std::string& request,
                         const std::vector<std::string>& forbidden_uris,
                         const OdtLevel_e odt_level,
                         const boost::optional<boost::posix_time::ptime>& since,
                         const boost::optional<boost::posix_time::ptime>& until,
                         const type::Data& data) {
    type::static_data* static_data = type::static_data::get();
    std::string res = request.empty() ? std::string("all") : request;

    switch (requested_type) {
    case Type_e::Line:
        if (odt_level != OdtLevel_e::all) {
            res = "(" + res + ") AND line.odt_level = " + to_string(odt_level);
        }
        break;
    case Type_e::VehicleJourney:
    case Type_e::Impact:
        if (since && until) {
            res = "(" + res + ") AND " +
                static_data->captionByType(requested_type) + ".between(" +
                to_iso_string(*since) + "Z, " + to_iso_string(*until) + "Z)";
        } else if (since) {
            res = "(" + res + ") AND " +
                static_data->captionByType(requested_type) + ".since(" +
                to_iso_string(*since) + "Z)";
        } else if (until) {
            res = "(" + res + ") AND " +
                static_data->captionByType(requested_type) + ".until(" +
                to_iso_string(*until) + "Z)";
        }
        break;
    default: break;
    }

    std::vector<std::string> forbidden;
    for (const auto& id: forbidden_uris) {
        const auto type = data.get_type_of_id(id);
        if (type == Type_e::Unknown) { continue; }
        std::stringstream ss;
        try {
            ss << static_data->captionByType(type);
        } catch(std::out_of_range&) {
            throw parsing_error(parsing_error::error_type::unknown_object,
                                "Filter Unknown object type: " + id);
        }
        ss << ".id = ";
        ast::print_quoted_string(ss, id);
        forbidden.push_back(ss.str());
    }
    if (!forbidden.empty()) {
        std::stringstream ss;
        ss << '(' << res << ") - (";
        auto it = forbidden.begin(), end = forbidden.end();
        ss << *it;
        for (++it; it != end; ++it) {
            ss << " OR " << *it;
        }
        ss << ')';
        res = ss.str();
    }

    return res;
}

Indexes make_query_ng(const Type_e requested_type,
                      const std::string& request,
                      const std::vector<std::string>& forbidden_uris,
                      const OdtLevel_e odt_level,
                      const boost::optional<boost::posix_time::ptime>& since,
                      const boost::optional<boost::posix_time::ptime>& until,
                      const type::Data& data) {
    auto logger = log4cplus::Logger::getInstance("ptref");
    const auto request_ng = make_request(requested_type, request, forbidden_uris, odt_level, since, until, data);
    const auto expr = parse(request_ng);
    LOG4CPLUS_DEBUG(logger, "ptref_ng parsed: " << expr
            << " [requesting: " << navitia::type::static_data::captionByType(requested_type) << "]");
    return boost::apply_visitor(Eval(requested_type, data), expr);
}

}} // namespace navitia::ptref
