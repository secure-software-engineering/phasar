/*
 * DBConn.cpp
 *
 *  Created on: 23.08.2016
 *      Author: pdschbrt
 */

#include "DBConn.hh"


DBConn::DBConn(const string name) : dbname(name)
{
	if ((last_retcode = sqlite3_open(dbname.c_str(), &db)) != SQLITE_OK)
		cerr << "could not open sqlite3 database!" << endl;
}

DBConn::~DBConn()
{
	if ((last_retcode = sqlite3_close(db)) != SQLITE_OK)
		cerr << "could not close sqlite3 database properly!" << endl;
}

DBConn& DBConn::getInstance()
{
	static DBConn instance;
	return instance;
}

int DBConn::resultSetCallBack(void *data, int argc, char **argv, char **azColName)
{
	ResultSet *rs = (ResultSet*) data;
	rs->rows++;
	if (rs->rows == 1) {
		rs->header.resize(argc);
		for (int i = 0; i < argc; ++i) {
			rs->header.push_back(azColName[i]);
		}
	}
	vector<string> row(argc);
	for (int i = 0; i < argc; ++i) {
		row.push_back((argv[i] ? argv[i] : "NULL"));
	}
	rs->data.push_back(row);
	return 0;
}

ResultSet DBConn::execute(const string& query)
{
	ResultSet result;
	if ((last_retcode = sqlite3_exec(db, query.c_str(), DBConn::resultSetCallBack, (void*) &result, &error_msg)) != SQLITE_OK) {
		cerr << "could not execute sqlite3 query!" << endl;
		cerr << getLastErrorMsg() << endl;
		sqlite3_free(error_msg);
	}
	return result;
}

string DBConn::getDBName() { return dbname; }

int DBConn::getLastRetCode() { return last_retcode; }

string DBConn::getLastErrorMsg() { return (error_msg != 0) ? string(error_msg) : "none"; }

void DBConn::createDBSchemeForAnalysis(const string& analysis_name)
{
	const static string analysis_initialization_command = "";
	execute(analysis_initialization_command);
}

void DBConn::createDBSchemeFromScratch()
{
	const static string database_initialization_command = "";
	execute(database_initialization_command);
}

vector<string> DBConn::getAllTables()
{
	const static string list_tables = "SELECT name FROM sqlite_master WHERE type='table';";
	ResultSet rs = execute(list_tables);
	vector<string> tables;
	for (auto row : rs.data) {
		for (auto entry : row) {
			tables.push_back(entry);
		}
	}
	return tables;
}

bool DBConn::tableExists(const string& table_name)
{
	vector<string> tables = getAllTables();
	for (auto table : tables) {
		if (table == table_name) {
			return true;
		}
	}
	return false;
}

bool DBConn::dropTable(const string& table_name)
{
	if (!tableExists(table_name)) return false;
	const static string drop_table = "DROP TABLE "+table_name+";";
	execute(drop_table);
	return true;
}

