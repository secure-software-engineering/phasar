/*
 * DBConn.hh
 *
 *  Created on: 23.08.2016
 *      Author: pdschbrt
 */

#ifndef DATABASE_DBCONN_HH_
#define DATABASE_DBCONN_HH_

#include <string>
#include <vector>
#include <map>
#include <iostream>
#include <functional>
#include <sqlite3.h>
#include "../utils/utils.hh"

using namespace std;

struct ResultSet {
	size_t rows = 0;
	vector<string> header;
	vector<vector<string>> data;
};

class DBConn {
private:
	DBConn(const string name="db/ifds_ide_database.db");
	~DBConn();
	sqlite3 *db;
	const string dbname;
	int last_retcode;
	char *error_msg = 0;
	static int resultSetCallBack(void *data, int argc, char **argv, char **azColName);
	void createDBSchemeFromScratch();

public:
	DBConn(const DBConn& db) = delete;
	DBConn(DBConn&& db) = delete;
	DBConn& operator= (const DBConn& db) = delete;
	DBConn& operator= (DBConn&& db) = delete;
	static DBConn& getInstance();
	ResultSet execute(const string& query);
	string getDBName();
	int getLastRetCode();
	string getLastErrorMsg();
	void createDBSchemeForAnalysis(const string& analysis_name);
	bool tableExists(const string& table_name);
	bool dropTable(const string& table_name);
	vector<string> getAllTables();
};

#endif /* DATABASE_DBCONN_HH_ */
