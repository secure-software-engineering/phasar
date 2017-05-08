/*
 * IO.hh
 *
 *  Created on: 02.05.2017
 *      Author: philipp
 */

#ifndef SRC_UTILS_IO_HH_
#define SRC_UTILS_IO_HH_

#include <string>
#include <fstream>
#include <boost/filesystem.hpp>
using namespace std;

string readFile(const string& path);

void writeFile(const string& path, const string& content);

#endif /* SRC_UTILS_IO_HH_ */
