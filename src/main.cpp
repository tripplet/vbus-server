#include <iostream>

#include <json/json.h>
#include <SQLiteCpp/SQLiteCpp.h>

#define DB_PATH "/srv/http/vbus-data.db"

int main(int argc, char const *argv[])
{
  try {
    SQLite::Database db(DB_PATH);

    SQLite::Statement query(db, "SELECT datetime(time, 'localtime'),temp1,temp2,temp3,temp4,pump1,pump2 FROM data ORDER BY id DESC LIMIT 1;");
    if (query.executeStep()) {
        std::cout << query.getColumn(0).getText() << std::endl;
    }
  }
  catch (std::exception& e) {
      std::cout << "SQLite exception: " << e.what() << std::endl;
      return 1;
  }

  /* code */
  return 0;
}
