#include <iostream>
#include <locale>
#include <cstdlib>
#include <sstream>
#include <algorithm>
#include <memory>
#include <limits>

#include <SQLiteCpp/SQLiteCpp.h>

#include "httphandler.hpp"

#if !defined(DB_PATH)
    #define DB_PATH "/srv/http/data/vbus.sqlite"
#endif

#define CSV_HEADER "Datum,Ofen,Speicher-unten,Speicher-oben,Heizung,Ventil-Ofen,Ventil-Heizung"

#define SELECT_TIMESPAN "SELECT" \
                        " datetime(time, 'localtime') as time," \
                        " temp1, temp2, temp3, temp4, pump1, pump2" \
                        " FROM data" \
                        " WHERE time > (SELECT DATETIME('now', ?))" \
                        " ORDER BY time"

#define SELECT_TIMESPAN_RANGE "SELECT" \
                              " datetime(time, 'localtime') as time," \
                              " temp1, temp2, temp3, temp4, pump1, pump2" \
                              " FROM data" \
                              " WHERE time > (SELECT DATETIME(?, 'utc')) AND time < (SELECT DATETIME(?, 'utc', ?))" \
                              " ORDER BY time"

#define SELECT_SINGLE "SELECT" \
                      " datetime(time, 'localtime') as time," \
                      " temp1, temp2, temp3, temp4, pump1, pump2" \
                      " FROM data" \
                      " ORDER BY id DESC LIMIT 1"

#define SELECT_CURRENT "SELECT" \
                       " datetime(time, 'localtime') as time," \
                       " temp1, temp2, temp3, temp4, pump1, pump2" \
                       " FROM data" \
                       " WHERE time > (SELECT DATETIME('now', '-5 minutes'))" \
                       " ORDER BY id DESC LIMIT 1"


void printCsvResult(SQLite::Statement& query, std::ostream& stream, bool isFirstRow, bool clip);
void printJsonResult(SQLite::Statement& query, std::ostream& stream, bool isFirstRow);

// See: http://stackoverflow.com/questions/15220861/how-can-i-set-the-comma-to-be-a-decimal-point
class punct_facet: public std::numpunct<char>
{
protected:
    char do_decimal_point() const {
        return '.';
    }
};

int main(int argc, char const *argv[])
{
    // Set decimal seperator to '.'
    std::cout.imbue(std::locale(std::cout.getloc(), new punct_facet()));

    double minTemp[4];
    double maxTemp[4];
    std::fill_n(minTemp, 4,  std::numeric_limits<double>::infinity());
    std::fill_n(maxTemp, 4, -std::numeric_limits<double>::infinity());

    // Get HTTP handler and set default paramter
    HttpHandler http(std::getenv("HTTP_ACCEPT_ENCODING"), std::getenv("REQUEST_URI"), std::cout);
    http.setDefault("timespan", "-1 hour");
    http.setDefault("format", "csv");
    http.setDefault("start", "now");
    http.setDefault("clip", "0");

    // Print HTTP header
    if (http.param("format") == "csv")
    {
        http.setContentType("text/comma-separated-values");
    }
    else if (http.param("format") == "json")
    {
        http.setContentType("application/json");
    }

    http.sendHeader();
    auto outputStream = http.getStream();

    try
    {
        SQLite::Database db(DB_PATH);
        db.setBusyTimeout(3000);

        // Build sqlite query
        std::unique_ptr<SQLite::Statement> query;

        if (http.param("timespan") == "single")
        {
            query.reset(new SQLite::Statement(db, SELECT_SINGLE));
        }
        else if (http.param("timespan") == "current")
        {
            query.reset(new SQLite::Statement(db, SELECT_CURRENT));

        }
        else
        {
            if (http.param("start") != "now")
            {
                query.reset(new SQLite::Statement(db, SELECT_TIMESPAN_RANGE));
                query->bind(1, http.param("start"));
                query->bind(2, http.param("start"));
                query->bind(3, http.param("timespan"));
            }
            else
            {
                query.reset(new SQLite::Statement(db, SELECT_TIMESPAN));
                query->bind(1, http.param("timespan"));
            }
        }

        if (http.param("format") == "csv")
        {
            *outputStream << CSV_HEADER << std::endl;
        }
        else if (http.param("format") == "json")
        {
            *outputStream << "{\"data\":[" << std::endl;
        }

        bool isFirstRow = true;

        while (query->executeStep())
        {
            if (http.param("format") == "csv")
            {
                printCsvResult(*query, *outputStream, isFirstRow, http.param("clip") == "1");
            }
            else if (http.param("format") == "json")
            {
                printJsonResult(*query, *outputStream, isFirstRow);
            }

            // Update min/max stats
            for (int idx=0; idx<4; idx++)
            {
                minTemp[idx] = std::min(minTemp[idx], query->getColumn(idx+1).getDouble());
                maxTemp[idx] = std::max(maxTemp[idx], query->getColumn(idx+1).getDouble());
            }

            http.process();

            isFirstRow = false;
        }

        // Add min/max stats to json response
        if (http.param("format") == "json")
        {
            if (http.param("timespan") == "single" || http.param("timespan") == "current")
            {
                *outputStream << std::endl << "]}" << std::endl;
            }
            else
            {
                *outputStream << std::endl << "]";

                for (int idx=0; idx<4; idx++)
                {
                    *outputStream << ",\"temp" << idx+1 << "\": {\"min\": " << minTemp[idx] << ", \"max\": " << maxTemp[idx] << "}" << std::endl;
                }

                *outputStream << "}";
            }

            http.process();
        }

        http.flush();
    }
    catch (std::exception& e)
    {
        std::cerr << e.what() << std::endl;
        return 1;
    }

    return 0;
}

void printCsvResult(SQLite::Statement& query, std::ostream& stream, bool isFirstRow, bool clip)
{
    stream << query.getColumn(0).getText()   << ","
           << query.getColumn(1).getDouble() << ","
           << query.getColumn(2).getDouble() << ","
           << query.getColumn(3).getDouble() << ","
           << query.getColumn(4).getDouble() << ",";

    if (clip && query.getColumn(5).getDouble() > query.getColumn(1).getDouble())
    {
        stream << query.getColumn(1).getDouble() << ",";
    }
    else if (clip && query.getColumn(5).getInt() == 0)
    {
        stream << "NaN,";
    }
    else
    {
        stream << query.getColumn(5).getDouble() << ",";
    }

    if (clip && query.getColumn(6).getDouble() > query.getColumn(3).getDouble())
    {
        stream << query.getColumn(3).getDouble() << std::endl;
    }
    else if (clip && query.getColumn(6).getInt() == 0)
    {
        stream << "NaN" << std::endl;
    }
    else
    {
        stream << query.getColumn(6).getDouble() << std::endl;
    }
}

void printJsonResult(SQLite::Statement& query, std::ostream& stream, bool isFirstRow)
{
    if (!isFirstRow) {
        stream << "," << std::endl;
    }

    stream << "{\"timestamp\":\"" << query.getColumn(0).getText()   << "\","
           << "\"temp1\":"        << query.getColumn(1).getDouble() << ","
           << "\"temp2\":"        << query.getColumn(2).getDouble() << ","
           << "\"temp3\":"        << query.getColumn(3).getDouble() << ","
           << "\"temp4\":"        << query.getColumn(4).getDouble() << ","
           << "\"valve1\":"       << query.getColumn(5).getInt()    << ","
           << "\"valve2\":"       << query.getColumn(6).getInt()    << "}";
}
