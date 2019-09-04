#pragma once
#include <set>
#include <string>

namespace CCPP {

class Event {
public:
  std::string eventName;
  std::set<int> factoryParamIdx;
  std::set<int> consumerParamIdx;
  bool isFactoryFunction;
  bool isConsumingFunction;
};

} // namespace CCPP