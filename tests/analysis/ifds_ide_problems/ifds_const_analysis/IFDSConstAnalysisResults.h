/* ============== GROUND TRUTH FOR BASIC TESTS ============== */
const std::map<unsigned, std::set<std::string>> basic_01_result = {};

const std::map<unsigned, std::set<std::string>> basic_02_result = {
  {5, {"%2 = alloca i32, align 4, !phasar.instruction.id !2, ID: 1"}}};

const std::map<unsigned, std::set<std::string>> basic_03_result = {
  {8, {"%2 = alloca i32, align 4, !phasar.instruction.id !2, ID: 1"}}};

const std::map<unsigned, std::set<std::string>> basic_04_result = {
  {7, {"%2 = alloca i32, align 4, !phasar.instruction.id !2, ID: 1"}},
  {8, {"%2 = alloca i32, align 4, !phasar.instruction.id !2, ID: 1"}},
  {9, {"%2 = alloca i32, align 4, !phasar.instruction.id !2, ID: 1"}},
  {10, {"%2 = alloca i32, align 4, !phasar.instruction.id !2, ID: 1"}}};

const std::map<unsigned, std::set<std::string>> basic_05_result = {
  {7, {"%2 = alloca i32, align 4, !phasar.instruction.id !2, ID: 1"}},
  {8, {"%2 = alloca i32, align 4, !phasar.instruction.id !2, ID: 1"}},
  {9, {"%2 = alloca i32, align 4, !phasar.instruction.id !2, ID: 1"}},
  {10, {"%2 = alloca i32, align 4, !phasar.instruction.id !2, ID: 1"}},
  {11, {"%2 = alloca i32, align 4, !phasar.instruction.id !2, ID: 1"}},
  {12, {"%2 = alloca i32, align 4, !phasar.instruction.id !2, ID: 1"}},
  {13, {"%2 = alloca i32, align 4, !phasar.instruction.id !2, ID: 1"}},
  {14, {"%2 = alloca i32, align 4, !phasar.instruction.id !2, ID: 1"}},
  {15, {"%2 = alloca i32, align 4, !phasar.instruction.id !2, ID: 1"}},
  {16, {"%2 = alloca i32, align 4, !phasar.instruction.id !2, ID: 1"}}};

const std::map<unsigned, std::set<std::string>> basic_06_result = {
  {21, {"%2 = alloca i32, align 4, !phasar.instruction.id !2, ID: 1"}}};


/* ============== GROUND TRUTH FOR CALL TESTS ============== */
const std::map<unsigned, std::set<std::string>> call_01_result = {
  {5, {"%1 = alloca i32, align 4, !phasar.instruction.id !1, ID: 0"}}};

const std::map<unsigned, std::set<std::string>> call_02_result = {
  {10, {"%2 = alloca i32, align 4, !phasar.instruction.id !2, ID: 4"}},
  {11, {"%2 = alloca i32, align 4, !phasar.instruction.id !2, ID: 4"}}};

const std::map<unsigned, std::set<std::string>> call_03_result = {
  {5, {"%1 = alloca i32, align 4, !phasar.instruction.id !1, ID: 0"}},
  {13, {"%2 = alloca i32, align 4, !phasar.instruction.id !2, ID: 7"}},
  {14, {"%2 = alloca i32, align 4, !phasar.instruction.id !2, ID: 7"}}};

const std::map<unsigned, std::set<std::string>> call_param_01_result = {
  {8, {"%3 = alloca i32, align 4, !phasar.instruction.id !2, ID: 1"}}};

const std::map<unsigned, std::set<std::string>> call_param_02_result = {
  {5, {"%2 = alloca i32, align 4, !phasar.instruction.id !1, ID: 0"}}};

const std::map<unsigned, std::set<std::string>> call_param_03_result = {
  {6,
       {"i32* %0",
         "%3 = load i32*, i32** %2, align 8, !phasar.instruction.id !3, ID: 2"}},
  {12, {"%2 = alloca i32, align 4, !phasar.instruction.id !2, ID: 8"}}};

const std::map<unsigned, std::set<std::string>> call_param_04_result = {
  {6,
       {"i32* %0",
         "%3 = load i32*, i32** %2, align 8, !phasar.instruction.id !3, ID: 2"}},
  {12, {"%2 = alloca i32, align 4, !phasar.instruction.id !2, ID: 8"}}};

const std::map<unsigned, std::set<std::string>> call_param_05_result = {
  {6,
    {"%3 = load i32*, i32** %2, align 8, !phasar.instruction.id !3, ID: 2",
      "i32* %0"}},
  {15,
    {"%4 = load i32*, i32** %3, align 8, !phasar.instruction.id !7, ID: 13",
      "%2 = alloca i32, align 4, !phasar.instruction.id !2, ID: 8"}}};

const std::map<unsigned, std::set<std::string>> call_param_06_result = {
  {9,
       {"%6 = load i32*, i32** %3, align 8, !phasar.instruction.id !6, ID: 5",
         "i32* %0"}},
  {18, {"%2 = alloca i32, align 4, !phasar.instruction.id !2, ID: 11"}}};

const std::map<unsigned, std::set<std::string>> call_ret_01_result = {
  {7, {"%2 = alloca i32, align 4, !phasar.instruction.id !2, ID: 2"}}};

const std::map<unsigned, std::set<std::string>> call_ret_02_result = {
  {10, {"%2 = alloca i32, align 4, !phasar.instruction.id !2, ID: 5"}}};

const std::map<unsigned, std::set<std::string>> call_ret_03_result = {
  {10, {"%2 = alloca i32, align 4, !phasar.instruction.id !2, ID: 5"}}};

const std::map<unsigned, std::set<std::string>> call_ret_04_result = {};


/* ============== GROUND TRUTH FOR CONTROL FLOW TESTS ============== */
const std::map<unsigned, std::set<std::string>> cf_for_01_result = {
  {7,
    {"%2 = alloca i32, align 4, !phasar.instruction.id !2, ID: 1",
      "%3 = alloca i32, align 4, !phasar.instruction.id !3, ID: 2"}},
  {8,
    {"%2 = alloca i32, align 4, !phasar.instruction.id !2, ID: 1",
      "%3 = alloca i32, align 4, !phasar.instruction.id !3, ID: 2"}},
  {9,
    {"%2 = alloca i32, align 4, !phasar.instruction.id !2, ID: 1",
      "%3 = alloca i32, align 4, !phasar.instruction.id !3, ID: 2"}},
  {10,
    {"%2 = alloca i32, align 4, !phasar.instruction.id !2, ID: 1",
      "%3 = alloca i32, align 4, !phasar.instruction.id !3, ID: 2"}},
  {11,
    {"%2 = alloca i32, align 4, !phasar.instruction.id !2, ID: 1",
      "%3 = alloca i32, align 4, !phasar.instruction.id !3, ID: 2"}},
  {12,
    {"%2 = alloca i32, align 4, !phasar.instruction.id !2, ID: 1",
      "%3 = alloca i32, align 4, !phasar.instruction.id !3, ID: 2"}},
  {13,
    {"%2 = alloca i32, align 4, !phasar.instruction.id !2, ID: 1",
      "%3 = alloca i32, align 4, !phasar.instruction.id !3, ID: 2"}},
  {14,
    {"%2 = alloca i32, align 4, !phasar.instruction.id !2, ID: 1",
      "%3 = alloca i32, align 4, !phasar.instruction.id !3, ID: 2"}},
  {15,
    {"%2 = alloca i32, align 4, !phasar.instruction.id !2, ID: 1",
      "%3 = alloca i32, align 4, !phasar.instruction.id !3, ID: 2"}},
  {16,
    {"%3 = alloca i32, align 4, !phasar.instruction.id !3, ID: 2",
      "%2 = alloca i32, align 4, !phasar.instruction.id !2, ID: 1"}},
  {17,
    {"%2 = alloca i32, align 4, !phasar.instruction.id !2, ID: 1",
      "%3 = alloca i32, align 4, !phasar.instruction.id !3, ID: 2"}},
  {18,
    {"%2 = alloca i32, align 4, !phasar.instruction.id !2, ID: 1",
      "%3 = alloca i32, align 4, !phasar.instruction.id !3, ID: 2"}},
  {19,
    {"%3 = alloca i32, align 4, !phasar.instruction.id !3, ID: 2",
      "%2 = alloca i32, align 4, !phasar.instruction.id !2, ID: 1"}},
  {20,
    {"%2 = alloca i32, align 4, !phasar.instruction.id !2, ID: 1",
      "%3 = alloca i32, align 4, !phasar.instruction.id !3, ID: 2"}},
  {21,
    {"%2 = alloca i32, align 4, !phasar.instruction.id !2, ID: 1",
      "%3 = alloca i32, align 4, !phasar.instruction.id !3, ID: 2"}}};

const std::map<unsigned, std::set<std::string>> cf_for_02_result = {};

const std::map<unsigned, std::set<std::string>> cf_if_01_result = {
  {9, {"%2 = alloca i32, align 4, !phasar.instruction.id !3, ID: 2"}},
  {10, {"%2 = alloca i32, align 4, !phasar.instruction.id !3, ID: 2"}}};

const std::map<unsigned, std::set<std::string>> cf_if_02_result = {
  {11, {"%2 = alloca i32, align 4, !phasar.instruction.id !3, ID: 2"}},
  {13, {"%3 = alloca i32, align 4, !phasar.instruction.id !4, ID: 3"}},
  {14,
       {"%2 = alloca i32, align 4, !phasar.instruction.id !3, ID: 2",
         "%3 = alloca i32, align 4, !phasar.instruction.id !4, ID: 3"}}};

const std::map<unsigned, std::set<std::string>> cf_while_01_result = {
  {7, {"%3 = alloca i32, align 4, !phasar.instruction.id !3, ID: 2"}},
  {8, {"%3 = alloca i32, align 4, !phasar.instruction.id !3, ID: 2"}},
  {9, {"%3 = alloca i32, align 4, !phasar.instruction.id !3, ID: 2"}},
  {10, {"%3 = alloca i32, align 4, !phasar.instruction.id !3, ID: 2"}},
  {11, {"%3 = alloca i32, align 4, !phasar.instruction.id !3, ID: 2"}},
  {12, {"%3 = alloca i32, align 4, !phasar.instruction.id !3, ID: 2"}}};


/* ============== GROUND TRUTH FOR GLOBAL TESTS ============== */
const std::map<unsigned, std::set<std::string>> global_01_result = {};

const std::map<unsigned, std::set<std::string>> global_02_result = {
  {7, {"%2 = alloca i32, align 4, !phasar.instruction.id !3, ID: 2"}},
  {8,
      {"@gint = global i32 10, align 4, !phasar.instruction.id !0",
        "%2 = alloca i32, align 4, !phasar.instruction.id !3, ID: 2"}},
  {9,
      {"@gint = global i32 10, align 4, !phasar.instruction.id !0",
        "%2 = alloca i32, align 4, !phasar.instruction.id !3, ID: 2"}},
  {10,
      {"%2 = alloca i32, align 4, !phasar.instruction.id !3, ID: 2",
        "@gint = global i32 10, align 4, !phasar.instruction.id !0"}},
  {11,
      {"@gint = global i32 10, align 4, !phasar.instruction.id !0",
        "%2 = alloca i32, align 4, !phasar.instruction.id !3, ID: 2"}},
  {12,
      {"@gint = global i32 10, align 4, !phasar.instruction.id !0",
        "%2 = alloca i32, align 4, !phasar.instruction.id !3, ID: 2"}},
  {13,
      {"%2 = alloca i32, align 4, !phasar.instruction.id !3, ID: 2",
        "@gint = global i32 10, align 4, !phasar.instruction.id !0"}},
  {14,
      {"@gint = global i32 10, align 4, !phasar.instruction.id !0",
        "%2 = alloca i32, align 4, !phasar.instruction.id !3, ID: 2"}}};

const std::map<unsigned, std::set<std::string>> global_03_result = {
  {7,
    {"@gint = global i32 10, align 4, !phasar.instruction.id !0",
      "%3 = load i32*, i32** %2, align 8, !phasar.instruction.id !6, ID: 5"}}};

const std::map<unsigned, std::set<std::string>> global_04_result = {
  {4, {"@gint = global i32 10, align 4, !phasar.instruction.id !0"}}};

const std::map<unsigned, std::set<std::string>> global_05_result = {
  {5,
       {"@gint = global i32* null, align 8, !phasar.instruction.id !0",
         "%1 = load i32*, i32** @gint, align 8, !phasar.instruction.id !2, ID: "
           "1"}},
  {11, {"@gint = global i32* null, align 8, !phasar.instruction.id !0"}},
  {12, {"@gint = global i32* null, align 8, !phasar.instruction.id !0"}}};


/* ============== GROUND TRUTH FOR POINTER TESTS ============== */
const std::map<unsigned, std::set<std::string>> pointer_01_result = {
  {8,
    {"%2 = alloca i32, align 4, !phasar.instruction.id !2, ID: 1",
      "%4 = load i32*, i32** %3, align 8, !phasar.instruction.id !7, ID: 6"}}};

const std::map<unsigned, std::set<std::string>> pointer_02_result = {
  {9, {"%4 = alloca i32*, align 8, !phasar.instruction.id !4, ID: 3"}}};

const std::map<unsigned, std::set<std::string>> pointer_03_result = {
  {11,
    {"%6 = load i32*, i32** %5, align 8, !phasar.instruction.id !10, ID: 9",
      "%2 = alloca i32, align 4, !phasar.instruction.id !2, ID: 1"}}};

const std::map<unsigned, std::set<std::string>> pointer_04_result = {
  {11,
    {"%6 = load i32*, i32** %4, align 8, !phasar.instruction.id !10, ID: 9",
      "%5 = load i32*, i32** %3, align 8, !phasar.instruction.id !8, ID: 7",
      "%2 = alloca i32, align 4, !phasar.instruction.id !2, ID: 1"}}};

const std::map<unsigned, std::set<std::string>> pointer_heap_01_result = {};

const std::map<unsigned, std::set<std::string>> pointer_heap_02_result = {};

const std::map<unsigned, std::set<std::string>> pointer_heap_03_result = {
  {9,
    {"%4 = bitcast i8* %3 to i32*, !phasar.instruction.id !5, ID: 4",
      "%5 = load i32*, i32** %2, align 8, !phasar.instruction.id !8, ID: 7",
      "%6 = load i32*, i32** %2, align 8, !phasar.instruction.id !10, ID: 9",
      "%9 = bitcast i32* %6 to i8*, !phasar.instruction.id !13, ID: 12",
      "%3 = call i8* @_Znwm(i64 4) #3, !phasar.instruction.id !4, ID: 3"}},
  {10,
    {"%4 = bitcast i8* %3 to i32*, !phasar.instruction.id !5, ID: 4",
      "%5 = load i32*, i32** %2, align 8, !phasar.instruction.id !8, ID: 7",
      "%6 = load i32*, i32** %2, align 8, !phasar.instruction.id !10, ID: 9",
      "%9 = bitcast i32* %6 to i8*, !phasar.instruction.id !13, ID: 12",
      "%3 = call i8* @_Znwm(i64 4) #3, !phasar.instruction.id !4, ID: 3"}},
  {11,
    {"%5 = load i32*, i32** %2, align 8, !phasar.instruction.id !8, ID: 7",
      "%3 = call i8* @_Znwm(i64 4) #3, !phasar.instruction.id !4, ID: 3",
      "%9 = bitcast i32* %6 to i8*, !phasar.instruction.id !13, ID: 12",
      "%6 = load i32*, i32** %2, align 8, !phasar.instruction.id !10, ID: 9",
      "%4 = bitcast i8* %3 to i32*, !phasar.instruction.id !5, ID: 4"}},
  {12,
    {"%4 = bitcast i8* %3 to i32*, !phasar.instruction.id !5, ID: 4",
      "%5 = load i32*, i32** %2, align 8, !phasar.instruction.id !8, ID: 7",
      "%6 = load i32*, i32** %2, align 8, !phasar.instruction.id !10, ID: 9",
      "%9 = bitcast i32* %6 to i8*, !phasar.instruction.id !13, ID: 12",
      "%3 = call i8* @_Znwm(i64 4) #3, !phasar.instruction.id !4, ID: 3"}},
  {13,
    {"%5 = load i32*, i32** %2, align 8, !phasar.instruction.id !8, ID: 7",
      "%3 = call i8* @_Znwm(i64 4) #3, !phasar.instruction.id !4, ID: 3",
      "%6 = load i32*, i32** %2, align 8, !phasar.instruction.id !10, ID: 9",
      "%4 = bitcast i8* %3 to i32*, !phasar.instruction.id !5, ID: 4",
      "%9 = bitcast i32* %6 to i8*, !phasar.instruction.id !13, ID: 12"}},
  {14,
    {"%4 = bitcast i8* %3 to i32*, !phasar.instruction.id !5, ID: 4",
      "%5 = load i32*, i32** %2, align 8, !phasar.instruction.id !8, ID: 7",
      "%6 = load i32*, i32** %2, align 8, !phasar.instruction.id !10, ID: 9",
      "%9 = bitcast i32* %6 to i8*, !phasar.instruction.id !13, ID: 12",
      "%3 = call i8* @_Znwm(i64 4) #3, !phasar.instruction.id !4, ID: 3"}},
  {15,
    {"%9 = bitcast i32* %6 to i8*, !phasar.instruction.id !13, ID: 12",
      "%3 = call i8* @_Znwm(i64 4) #3, !phasar.instruction.id !4, ID: 3",
      "%6 = load i32*, i32** %2, align 8, !phasar.instruction.id !10, ID: 9",
      "%5 = load i32*, i32** %2, align 8, !phasar.instruction.id !8, ID: 7",
      "%4 = bitcast i8* %3 to i32*, !phasar.instruction.id !5, ID: 4"}}};

const std::map<unsigned, std::set<std::string>> pointer_heap_04_result = {};

/* ============== STRUCTS TESTS ============== */
const std::map<unsigned, std::set<std::string>> structs_01_result = {};

const std::map<unsigned, std::set<std::string>> structs_02_result = {};