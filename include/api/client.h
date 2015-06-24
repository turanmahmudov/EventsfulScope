#ifndef API_CLIENT_H_
#define API_CLIENT_H_

#include <api/config.h>

#include <atomic>
#include <deque>
#include <map>
#include <string>
#include <core/net/http/request.h>
#include <core/net/uri.h>

#include <QJsonDocument>

namespace api {

/**
 * Provide a nice way to access the HTTP API.
 *
 * We don't want our scope's code to be mixed together with HTTP and JSON handling.
 */

class Client {
    public:

    /**
    * Track info, including the artist.
    */
    struct Track {
        std::string title;
        std::string venue_name;
        std::string city_name;
        std::string country_name;
        std::string description;
        std::string latitude;
        std::string longitude;
        std::string start_time;
        std::string image;
        std::string uri;
    };

    /**
    * A list of Track objects.
    */
    typedef std::deque<Track> TrackList;

    /**
    * Track results.
    */
    struct TrackRes {
        TrackList tracks;
    };

    Client(Config::Ptr config);

    virtual ~Client() = default;

    /**
     * Get the track list for a query
     */
    virtual TrackRes tracks(const std::string &query, const std::string &ll);

    /**
     * Cancel any pending queries (this method can be called from a different thread)
     */
    virtual void cancel();

    virtual Config::Ptr config();

protected:
    void get(const core::net::Uri::Path &path,
             const core::net::Uri::QueryParameters &parameters,
             QJsonDocument &root);
    /**
     * Progress callback that allows the query to cancel pending HTTP requests.
     */
    core::net::http::Request::Progress::Next progress_report(
            const core::net::http::Request::Progress& progress);

    /**
     * Hang onto the configuration information
     */
    Config::Ptr config_;

    /**
     * Thread-safe cancelled flag
     */
    std::atomic<bool> cancelled_;
};

}

#endif // API_CLIENT_H_

