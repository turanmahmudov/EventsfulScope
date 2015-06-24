#include <api/client.h>

#include <core/net/error.h>
#include <core/net/http/client.h>
#include <core/net/http/content_type.h>
#include <core/net/http/response.h>
#include <QVariantMap>

namespace http = core::net::http;
namespace net = core::net;

using namespace api;
using namespace std;

Client::Client(Config::Ptr config) :
    config_(config), cancelled_(false) {
}


void Client::get(const net::Uri::Path &path,
                 const net::Uri::QueryParameters &parameters, QJsonDocument &root) {
    // Create a new HTTP client
    auto client = http::make_client();

    // Start building the request configuration
    http::Request::Configuration configuration;

    // Build the URI from its components
    net::Uri uri = net::make_uri(config_->apiroot, path, parameters);
    configuration.uri = client->uri_to_string(uri);

    // Give out a user agent string
    configuration.header.add("User-Agent", config_->user_agent);

    // Build a HTTP request object from our configuration
    auto request = client->head(configuration);

    try {
        // Synchronously make the HTTP request
        // We bind the cancellable callback to #progress_report
        auto response = request->execute(
                    bind(&Client::progress_report, this, placeholders::_1));

        // Check that we got a sensible HTTP status code
        if (response.status != http::Status::ok) {
            throw domain_error(response.body);
        }
        // Parse the JSON from the response
        root = QJsonDocument::fromJson(response.body.c_str());

        // Open weather map API error code can either be a string or int
        QVariant cod = root.toVariant().toMap()["cod"];
        if ((cod.canConvert<QString>() && cod.toString() != "200")
                || (cod.canConvert<unsigned int>() && cod.toUInt() != 200)) {
            throw domain_error(root.toVariant().toMap()["message"].toString().toStdString());
        }
    } catch (net::Error &) {
    }
}

Client::TrackRes Client::tracks(const string& query, const string& location) {
    QJsonDocument root;

    // Build a URI and get the contents.
    // The fist parameter forms the path part of the URI.
    // The second parameter forms the CGI parameters.

    get( { "json", "events", "search" }, { { "location", location }, { "within", "2000" }, { "keywords", query }, { "app_key", "XhNvhQ5xzb8mhhbt" }, { "image_sizes", "large" } }, root);

    // My “list of tracks” object (as seen in the corresponding header file)
    TrackRes result;

    QVariantMap variant = root.toVariant().toMap();
    QVariantMap rData = variant["events"].toMap();
    QVariantList eData = rData["event"].toList();
    for (const QVariant &i : eData) {
        QVariantMap item = i.toMap();

        QVariantMap image = item["image"].toMap();
        QVariantMap limage = image["large"].toMap();
        string img = limage["url"].toString().toStdString();
        if (img.empty()) {
            img = "file:///usr/share/icons/suru/apps/scalable/calendar-app-symbolic.svg";
        }

        // We add each result to our list
        result.tracks.emplace_back(
            Track {
                item["title"].toString().toStdString(),
                item["venue_name"].toString().toStdString(),
                item["city_name"].toString().toStdString(),
                item["country_name"].toString().toStdString(),
                item["description"].toString().toStdString(),
                item["latitude"].toString().toStdString(),
                item["longitude"].toString().toStdString(),
                item["start_time"].toString().toStdString(),
                img,
                "uri"
            }
        );
    }

    return result;
}

http::Request::Progress::Next Client::progress_report(
        const http::Request::Progress&) {

    return cancelled_ ?
                http::Request::Progress::Next::abort_operation :
                http::Request::Progress::Next::continue_operation;
}

void Client::cancel() {
    cancelled_ = true;
}

Config::Ptr Client::config() {
    return config_;
}

