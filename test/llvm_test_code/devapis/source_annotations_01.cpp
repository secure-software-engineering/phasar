
#define PHASAR_ENABLE_TAINT_CONFIGURATION_API
#include "../../../devapis/taint/phasar_taint_config_api.h"

int main() {
    int x = 13;
    PHASAR_DECLARE_VAR_AS_SOURCE(x);
    return 0;
}
