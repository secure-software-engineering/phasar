#pagma once
#include <vector>
#include <set>
#include <string>
#include <FormatConverter/ObjectWithOutLLVM.h>


namespace CCPP {
class Predicate {
  private:
  bool operator == ( const Predicate &pc );
  std::string getFunctionName();
  //std::vector<Object> params;
  std::vector<ObjectWithOutLLVM> params;

  public:
  void setParams(const std::vector<ObjectWithOutLLVM> &params) {
    this->params = params;
  }
  const std::vector<ObjectWithOutLLVM> &getparams() const { return params; }
};

} // namespace CCPP