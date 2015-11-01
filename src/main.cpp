#include <iostream>
#include <locale>
#include <cstdlib>
#include <string>
#include <algorithm>
#include <unordered_map>

#include <uriparser/Uri.h>
#include <SQLiteCpp/SQLiteCpp.h>

#define DB_PATH "/srv/http/data/vbus.sqlite"

#define SELECT_TIMESPAN "SELECT " \
                        "datetime(time, 'localtime') as time, " \
                        "temp1, temp2, temp3, temp4, pump1, pump2 " \
                        "FROM data " \
                        "WHERE time > (SELECT DATETIME('now', ?)) " \
                        "ORDER BY id"

#define SELECT_SINGLE "SELECT " \
                      "datetime(time, 'localtime') as time, " \
                      "temp1, temp2, temp3, temp4, pump1, pump2 " \
                      "FROM data " \
                      "ORDER BY id DESC LIMIT 1"

typedef std::unordered_map<std::string, std::string> parameterMap;

parameterMap* parseURL(const std::string& url);

void printCsvResult(SQLite::Statement& query, std::ostream& stream, bool isFirstRow);
void printJsonResult(SQLite::Statement& query, std::ostream& stream, bool isFirstRow);

void find_and_replace(std::string& source, std::string const& find, std::string const& replace)
{
  for(std::string::size_type i = 0; (i = source.find(find, i)) != std::string::npos;)
  {
    source.replace(i, find.length(), replace);
    i += replace.length();
  }
}

// See: http://stackoverflow.com/questions/15220861/how-can-i-set-the-comma-to-be-a-decimal-point
class punct_facet: public std::numpunct<char>
{
  protected: char do_decimal_point() const { return '.'; }
};

int main(int argc, char const *argv[])
{
  // Set decimal seperator to '.'
  std::cout.imbue(std::locale(std::cout.getloc(), new punct_facet()));

  // Print HTTP header
  std::cout << "Content-type: text/comma-separated-values\r\n"
            << "Cache-Control: no-cache, no-store, must-revalidate\r\n"
            << "Pragma: no-cache\r\n"
            << "Expires: 0\r\n"
            << "Access-Control-Allow-Origin: *\r\n\r\n";

  std::unique_ptr<parameterMap> requestParameter;

  try
  {
    SQLite::Database db(DB_PATH);
    db.setBusyTimeout(3000);

    auto request_uri = std::getenv("REQUEST_URI");

    if (request_uri != nullptr)
    {
      requestParameter.reset(parseURL(std::string(request_uri)));
    }
    else {
      requestParameter.reset(new parameterMap());
    }

    // Set default parameter
    if (requestParameter->count("timespan") == 0) { (*requestParameter)["timespan"] = "-1 hour"; }
    if (requestParameter->count("format") == 0)   { (*requestParameter)["format"] = "csv"; }

    std::unique_ptr<SQLite::Statement> query;

    if ((*requestParameter)["timespan"] == "single")
    {
      query.reset(new SQLite::Statement(db, SELECT_SINGLE));
    }
    else {
      query.reset(new SQLite::Statement(db, SELECT_TIMESPAN));
      query->bind(1, (*requestParameter)["timespan"]);
    }

    if ((*requestParameter)["format"] == "csv")
    {
      std::cout << "Datum,Ofen,Speicher-unten,Speicher-oben,Heizung,Ventil-Ofen,Ventil-Heizung" << std::endl;
    }
    else if ((*requestParameter)["format"] == "json")
    {
      std::cout << "{ \"data\": [" << std::endl;
    }

    bool isFirstRow = true;

    while (query->executeStep())
    {
      if ((*requestParameter)["format"] == "csv")
      {
        printCsvResult(*query, std::cout, isFirstRow);
      }
      else if ((*requestParameter)["format"] == "json")
      {
        printJsonResult(*query, std::cout, isFirstRow);
      }

      isFirstRow = false;
    }

    if ((*requestParameter)["format"] == "json")
    {
      std::cout << std::endl<< "] }" << std::endl;
    }

  }
  catch (std::exception& e)
  {
      std::cerr << e.what() << std::endl;
      return 1;
  }

  return 0;
}


void printCsvResult(SQLite::Statement& query, std::ostream& stream, bool isFirstRow)
{
  stream << query.getColumn(0).getText()   << ","
         << query.getColumn(1).getDouble() << ","
         << query.getColumn(2).getDouble() << ","
         << query.getColumn(3).getDouble() << ","
         << query.getColumn(4).getDouble() << ","
         << query.getColumn(5).getInt()    << ","
         << query.getColumn(6).getInt()    << std::endl;
}

void printJsonResult(SQLite::Statement& query, std::ostream& stream, bool isFirstRow)
{
  if (!isFirstRow) {
    stream << "," << std::endl;
  }

  stream << "{ \"timestamp\": \"" << query.getColumn(0).getText() << "\" ,"
         << "\"temp1\": "         << query.getColumn(1).getDouble()     << ","
         << "\"temp2\": "         << query.getColumn(2).getDouble()     << ","
         << "\"temp3\": "         << query.getColumn(3).getDouble()     << ","
         << "\"temp4\": "         << query.getColumn(4).getDouble()     << ","
         << "\"valve1\": "        << query.getColumn(5).getInt()        << ","
         << "\"valve2\": "        << query.getColumn(6).getInt()        << "}";
}

parameterMap* parseURL(const std::string& url)
{
  UriParserStateA state;
  UriUriA uri;

  state.uri = &uri;
  if (uriParseUriA(&state, url.c_str()) != URI_SUCCESS)
  {
    uriFreeUriMembersA(&uri);
    return new std::unordered_map<std::string, std::string>();
  }

  UriQueryListA* queryList;
  int itemCount;

  if (uriDissectQueryMallocA(&queryList, &itemCount, uri.query.first, uri.query.afterLast) == URI_SUCCESS)
  {
    auto parameter = new parameterMap();
    auto& current = queryList;

    do
    {
      if (queryList->value != nullptr) {
        (*parameter)[queryList->key] = queryList->value;
      }
      else {
        (*parameter)[queryList->key] = std::string("");
      }

      current = queryList->next;
    } while (current != nullptr);

    uriFreeQueryListA(queryList);
    uriFreeUriMembersA(&uri);

    return parameter;
  }

  return new std::unordered_map<std::string, std::string>();
}