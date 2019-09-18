#pragma once
#include <set>
#include <string>
#include <vector>
#include <FormatConverter/ObjectWithOutLLVM.h>

namespace CCPP {

class Event {
private:
  std::string eventName;
  std::set<int> factoryParamIdx;
  std::set<int> consumerParamIdx;
  bool isFactoryFunction;
  bool isConsumingFunction;
  std::vector<ObjectWithOutLLVM> params;

public:
  void setEventName(const std::string &eventName) { this->eventName = eventName; }
  const std::string &getEventName() const { return eventName; }

  void setFactoryParamIdx(const std::set<int> &factoryParamIdx) { this->factoryParamIdx = factoryParamIdx; }
  const std::set<int> &getFactoryParamIdx() const { return factoryParamIdx; }

  void setConsumerParamIdx(const std::set<int> &consumerParamIdx) { this->consumerParamIdx = consumerParamIdx; }
  const std::set<int> &getConsumerParamIdx() const { return factoryParamIdx; }

  void setIsFactoryFunction(bool isFactoryFunction) { this->isFactoryFunction = isFactoryFunction; }
  bool getIsFactoryFunction() const { return isFactoryFunction; }

  void setIsConsumingFunction(bool isConsumingFunction) { this->isConsumingFunction = isConsumingFunction; }
  bool getIsConsumingFunction() const { return isConsumingFunction; }

  void setParams(const std::vector<ObjectWithOutLLVM> &params) { this->params = params; }
  const std::vector<ObjectWithOutLLVM> &getParams() const { return params; }

};

} // namespace CCPP