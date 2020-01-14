// name-mangled configuration options represented as external booleans
extern int _CfIf3K_CONFIG_A;
// the "definedness" of a configuration option is modeled as a separate boolean
extern int _Djkifd_CONFIG_A_defined;
extern int _Ckc8DF_CONFIG_B;
extern int _D8dDjJ_CONFIG_B_defined;
extern int _CJFi8c_CONFIG_C;
extern int _DIdf38_CONFIG_C_defined;

int main ( ) {
  if (_D8dDjJ_CONFIG_B_defined) { /* from static conditional around function definition */ {
      // conditionally-defined C variables are name-mangled and lifted outside the conditional scope
      int _Vkdfj3_x; /* metadata: renamed from x */ ; 
      int _Vf3DF7_y; /* metadata: renamed from y */ ;

      // the static conditional is propagated to the use of variables, i.e., an "implicit static conditional"
      if (_D8dDjJ_CONFIG_B_defined && _DIdf38_CONFIG_C_defined) { /* metadata: from static conditional around statement */
        _Vf3DF7_y = 4 ; 
      }
      if (_D8dDjJ_CONFIG_B_defined && _Djkifd_CONFIG_A_defined && _DIdf38_CONFIG_C_defined) { /* metadata: from static conditional around statement */
        _Vkdfj3_x = _Vf3DF7_y; 
      }
      if (_D8dDjJ_CONFIG_B_defined && _Djkifd_CONFIG_A_defined) { /* metadata: from static conditional around statement */
        _Vkdfj3_x++; 
      }
      if (_D8dDjJ_CONFIG_B_defined && _Djkifd_CONFIG_A_defined) { /* metadata: from static conditional around statement */
        return _Vkdfj3_x; 
      }
    } 
  }
}
