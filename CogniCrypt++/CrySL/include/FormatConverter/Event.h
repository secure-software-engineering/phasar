#pragma once
#include <set>
#include <string>

namespace CCPP {

class Event {
private:
  std::string eventName;
  std::set<int> factoryParamIdx;
  std::set<int> consumerParamIdx;
  bool isFactoryFunction;
  bool isConsumingFunction;

public:
  void setEventName(const std::string &eventName) { this->eventName = eventName; }
  const std::string & getEventName() { return eventName; }

  void setFactoryParamIdx(const std::set<int> &factoryParamIdx) { this->factoryParamIdx = factoryParamIdx; }
  const std::set<int> & getFactoryParamIdx() { return factoryParamIdx; }

  void setConsumerParamIdx(const std::set<int> &consumerParamIdx) { this->consumerParamIdx = consumerParamIdx; }
  const std::set<int> & getConsumerParamIdx() { return factoryParamIdx; }

  void setIsFactoryFunction(const bool &isFactoryFunction) { this->isFactoryFunction = isFactoryFunction; }
  const std::string & getIsFactoryFunction() { return isFactoryFunction; }

  void setIsConsumingFunction(const bool &isConsumingFunction) { this->isConsumingFunction = isConsumingFunction; }
  const std::string &getIsConsumingFunction() { return isConsumingFunction; }

};

} // namespace CCPP