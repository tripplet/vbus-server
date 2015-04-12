#include <iostream>
#include <cstdlib>
#include <string>
#include <algorithm>
#include <SQLiteCpp/SQLiteCpp.h>

#define DB_PATH "/srv/http/vbus-data.db"

void find_and_replace(std::string& source, std::string const& find, std::string const& replace)
{
  for(std::string::size_type i = 0; (i = source.find(find, i)) != std::string::npos;)
  {
    source.replace(i, find.length(), replace);
    i += replace.length();
  }
}


int main(int argc, char const *argv[])
{
  std::cout << "Content-type:text/comma-separated-values\r\n"
            << "Access-Control-Allow-Origin: *\r\n\r\n";

  try {
    SQLite::Database db(DB_PATH);

    std::string time_query = std::string("-1 hour");

    char *cgi_query = std::getenv("QUERY_STRING");

    if (cgi_query) {
      time_query = std::string(cgi_query);
      find_and_replace(time_query, "%20", " ");
    }

    SQLite::Statement query(db, "SELECT "
                                "datetime(time, 'localtime') as time, "
                                "temp1, temp2, temp3, temp4, pump1, pump2 "
                                "FROM data "
                                "WHERE time > (SELECT DATETIME('now', ?)) "
                                "ORDER BY id");

    query.bind(1, time_query);

    //std::cout << time_query<< std::endl;

    std::cout << "Datum,Ofen,Speicher-unten,Speicher-oben,Heizung,Ventil-Ofen,Ventil-Heizung" << std::endl;

    while (query.executeStep()) {
      std::cout << query.getColumn(0).getText()   << ","
                << query.getColumn(1).getDouble() << ","
                << query.getColumn(2).getDouble() << ","
                << query.getColumn(3).getDouble() << ","
                << query.getColumn(4).getDouble() << ","
                << query.getColumn(5).getInt()    << ","
                << query.getColumn(6).getInt()    << std::endl;
    }
  }
  catch (std::exception& e) {
      return 1;
  }

  /* code */
  return 0;
}
