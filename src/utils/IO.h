/*
 * IO.h
 *
 *  Created on: 02.05.2017
 *      Author: philipp
 */

#ifndef SRC_UTILS_IO_H_
#define SRC_UTILS_IO_H_

#include <boost/filesystem.hpp>
#include <fstream>
#include <string>
using namespace std;

string readFile(const string &path);

void writeFile(const string &path, const string &content);

#endif /* SRC_UTILS_IO_HH_ */
