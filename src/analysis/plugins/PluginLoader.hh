/*
 * PluginLoader.hh
 *
 *  Created on: 14.06.2017
 *      Author: philipp
 */

#ifndef SRC_ANALYSIS_PLUGINS_PLUGINLOADER_HH_
#define SRC_ANALYSIS_PLUGINS_PLUGINLOADER_HH_

#include <iostream>
#include <memory>
#include <dlfcn.h>
#include <cstdlib>
#include <cstdio>
using namespace std;

class PluginLoader {
public:
	PluginLoader();
	virtual ~PluginLoader();
};

#endif /* SRC_ANALYSIS_PLUGINS_PLUGINLOADER_HH_ */
