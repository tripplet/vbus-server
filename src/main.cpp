#include <iostream>
#include <locale>
#include <cstdlib>
#include <string>
#include <algorithm>
#include <unordered_map>

#include <uriparser/Uri.h>
#include <SQLiteCpp/SQLiteCpp.h>

#define DB_PATH "/srv/http/data/vbus.sqlite"

typedef std::unordered_map<std::string, std::string> parameterMap;

parameterMap* parseURL(const std::string& url);

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

  try
  {
    SQLite::Database db(DB_PATH);
    std::unique_ptr<parameterMap> requestParameter(parseURL(std::getenv("REQUEST_URI")));

    if (requestParameter->count("timespan") == 0)
    {
      requestParameter->at("timespan") = "-1 hour";
    }

    SQLite::Statement query(db, "SELECT "
                                "datetime(time, 'localtime') as time, "
                                "temp1, temp2, temp3, temp4, pump1, pump2 "
                                "FROM data "
                                "WHERE time > (SELECT DATETIME('now', ?)) "
                                "ORDER BY id");

    query.bind(1, (*requestParameter)["timespan"]);

    std::cout << "Datum,Ofen,Speicher-unten,Speicher-oben,Heizung,Ventil-Ofen,Ventil-Heizung" << std::endl;

    while (query.executeStep())
    {
      std::cout << query.getColumn(0).getText()   << ","
                << query.getColumn(1).getDouble() << ","
                << query.getColumn(2).getDouble() << ","
                << query.getColumn(3).getDouble() << ","
                << query.getColumn(4).getDouble() << ","
                << query.getColumn(5).getInt()    << ","
                << query.getColumn(6).getInt()    << std::endl;
    }
  }
  catch (std::exception& e)
  {
      std::cerr << e.what() << std::endl;
      return 1;
  }

  /* code */
  return 0;
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
      if (queryList->value != NULL) {
        (*parameter)[queryList->key] = queryList->value;
      }
      else {
        (*parameter)[queryList->key] = std::string("");
      }
      
      current = queryList->next;
    } while (current != NULL);
      
    uriFreeQueryListA(queryList);
    uriFreeUriMembersA(&uri);
  
    return parameter;
  }
  
  return new std::unordered_map<std::string, std::string>();
}
