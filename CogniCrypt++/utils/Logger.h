// used phasar/Utils/Logger.h
#pragma once
#include <string>

#include <boost/log/sinks.hpp>
#include <boost/log/sources/global_logger_storage.hpp>
#include <boost/log/sources/severity_logger.hpp>
#include <boost/log/support/date_time.hpp>
// Not useful here but enable all logging macros in files that include Logger.h
#include <boost/log/sources/record_ostream.hpp>



namespace bl = boost::log;

