#include <boost/algorithm/string/trim.hpp>

#include <scope/localization.h>
#include <scope/query.h>

#include <unity/scopes/Annotation.h>
#include <unity/scopes/CategorisedResult.h>
#include <unity/scopes/CategoryRenderer.h>
#include <unity/scopes/QueryBase.h>
#include <unity/scopes/SearchReply.h>
#include <unity/scopes/SearchMetadata.h>

#include <unity/scopes/Location.h>

#include <iomanip>
#include <sstream>

namespace sc = unity::scopes;
namespace alg = boost::algorithm;

using namespace std;
using namespace api;
using namespace scope;


/**
 * Define the layout for the forecast results
 *
 * The icon size is small, and ask for the card layout
 * itself to be horizontal. I.e. the text will be placed
 * next to the image.
 */

const static string VENUES_TEMPLATE =
    R"(
        {
            "schema-version": 1,
            "template": {
                "category-layout": "grid",
                "card-layout": "vertical",
                "card-size": "medium"
            },
            "components": {
                "title": "title",
                "art" : {
                    "field": "art"
                },
                "subtitle": "start_time"
            }
        }
    )";


Query::Query(const sc::CannedQuery &query, const sc::SearchMetadata &metadata,
             Config::Ptr config) :
    sc::SearchQueryBase(query, metadata), client_(config) {
}

void Query::cancelled() {
    client_.cancel();
}


void Query::run(sc::SearchReplyProxy const& reply) {
    try {
        // Start by getting information about the query
        const sc::CannedQuery &query(sc::SearchQueryBase::query());

        // A string to store the location name for the openweathermap query
        std::string ll;

        // Access search metadata
        auto metadata = search_metadata();

        // Check for location data
        if(metadata.has_location()) {
            auto location = metadata.location();

            // Check for city and country
            if(location.latitude() && location.longitude()) {

                // Create the "city country" string
                auto lat = location.latitude();
                auto lng = location.longitude();

                ll = std::to_string(lat) + "," + std::to_string(lng);
            }
        }

        // Fallback to a hardcoded location
        if(ll.empty()) {
            ll = "51.528,-0.101";
        }

        // Trim the query string of whitespace
        string query_string = alg::trim_copy(query.query_string());

        Client::TrackRes trackslist;
        if (query_string.empty()) {
            // If the string is empty, provide a specific one
            trackslist = client_.tracks("", ll);
        } else {
            // otherwise, use the query string
            trackslist = client_.tracks(query_string, ll);
        }


        // Register a category for tracks
        auto tracks_cat = reply->register_category("tracks", "", "",
            sc::CategoryRenderer(VENUES_TEMPLATE));
        // register_category(arbitrary category id, header title, header icon, template)
        // In this case, since this is the only category used by our scope,
        // it doesn’t need to display a header title, we leave it as a blank string.


        for (const auto &track : trackslist.tracks) {

            // Use the tracks category
            sc::CategorisedResult res(tracks_cat);

            // We must have a URI
            res.set_uri(track.uri);

            // Our result also needs a track title
            res.set_title(track.title);

            // Set the rest of the attributes, art, artist, etc.
            res.set_art(track.image);
            res["start_time"] = track.start_time;
            res["description"] = track.city_name + ", " + track.country_name + "\n" + track.venue_name + "\n\n" + track.description;
            res["city_name"] = track.city_name;
            res["country_name"] = track.country_name;
            res["venue_name"] = track.venue_name;


            // Push the result
            if (!reply->push(res)) {
                // If we fail to push, it means the query has been cancelled.
                return;
            }
        }

    } catch (domain_error &e) {
        // Handle exceptions being thrown by the client API
        cerr << e.what() << endl;
        reply->error(current_exception());

    }
}

