#pagma once
#include <vector>
#include <set>
#include <string>
#include <FormatConverter/ObjectWithOutLLVM.h>


namespace CCPP {
class Predicate {
  private:
  //std::vector<Object> params;
  std::vector<ObjectWithOutLLVM> params;

  public:
  bool operator == (const Predicate &pc);
  std::string getFunctionName();
  void setParams(const std::vector<ObjectWithOutLLVM> &params) {
    this->params = params;
  }
  const std::vector<ObjectWithOutLLVM> &getparams() const { return params; }
};

} // namespace CCPP