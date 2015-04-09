#include <iostream>
#include <SQLiteCpp/SQLiteCpp.h>

#define DB_PATH "/srv/http/vbus-data.db"

int main(int argc, char const *argv[])
{
  std::cout << "Content-type:application/json\r\n\r\n";

  try {
    SQLite::Database db(DB_PATH);

    SQLite::Statement query(db, "SELECT "
                                 "datetime(time, 'localtime') as time, "
                                 "temp1, temp2, temp3, temp4, pump1, pump2 "
                                 "FROM data "
                                 "WHERE time > (SELECT DATETIME('now', '-2 hour')) "
                                 "ORDER BY id DESC");

    std::cout << "{\"data\":[";

    bool first = true;

    while (query.executeStep()) {
      if (!first) {
        std::cout << ",";
      }
      else {
        first = false;
      }

      std::cout << "["
                << "\"" << query.getColumn(0).getText() << "\","
                << query.getColumn(1).getDouble() << ","
                << query.getColumn(2).getDouble() << ","
                << query.getColumn(3).getDouble() << ","
                << query.getColumn(4).getDouble() << ","
                << query.getColumn(5).getInt()    << ","
                << query.getColumn(6).getInt()    << "]";
    }

    std::cout << "]}";
  }
  catch (std::exception& e) {
      std::cout << "{ \"error\": \"SQLite exception: " << e.what() << "\" }" << std::endl;
      return 1;
  }

  /* code */
  return 0;
}
