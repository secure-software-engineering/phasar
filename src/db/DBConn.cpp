/*
 * DBConn.cpp
 *
 *  Created on: 23.08.2016
 *      Author: pdschbrt
 */

#include "DBConn.h"

const string DBConn::db_schema_name = "phasardb";
const string DBConn::db_user = "root";
const string DBConn::db_password = "1234";
const string DBConn::db_server_address = "tcp://127.0.0.1:3306";

DBConn::DBConn() {
  auto lg = lg::get();
  BOOST_LOG_SEV(lg, DEBUG) << "Connect to database";
  try {
    driver = get_driver_instance();
    conn = driver->connect(db_server_address, db_user, db_password);
    if (!schemeExists()) {
      buildDBScheme();
    }
    conn->setSchema(db_schema_name);
    unique_ptr<sql::PreparedStatement> pstmt(
        conn->prepareStatement("SET foreign_key_checks = 0"));
    pstmt->executeQuery();
  } catch (sql::SQLException &e) {
    SQL_STD_ERROR_HANDLING;
  }
}

DBConn::~DBConn() {
  auto lg = lg::get();
  BOOST_LOG_SEV(lg, DEBUG) << "Close database connection";
  delete conn;
}

int DBConn::getNextAvailableID(const string &TableName) {
  int id = 1;
  try {
    unique_ptr<sql::PreparedStatement> pstmt(
        conn->prepareStatement([&TableName]() {
          if (TableName == "project") {
            return "SELECT project_id FROM project ORDER BY project_id DESC "
                   "LIMIT 1";
          } else if (TableName == "module") {
            return "SELECT module_id FROM module ORDER BY module_id DESC LIMIT "
                   "1";
          } else if (TableName == "function") {
            return "SELECT function_id FROM function ORDER BY function_id DESC "
                   "LIMIT 1";
          } else if (TableName == "global_variable") {
            return "SELECT global_variable_id FROM global_variable ORDER BY "
                   "global_variable_id DESC LIMIT 1";
          } else if (TableName == "type") {
            return "SELECT type_id FROM type ORDER BY type_id DESC LIMIT 1";
          } else if (TableName == "type_hierarchy") {
            return "SELECT type_hierarchy_id FROM type_hierarchy ORDER BY "
                   "type_hierarchy_id DESC LIMIT 1";
          } else if (TableName == "callgraph") {
            return "SELECT callgraph_id FROM callgraph ORDER BY callgraph_id "
                   "DESC LIMIT 1";
          } else if (TableName == "points-to_graph") {
            return "SELECT points-to_graph_id FROM points-to_graph ORDER BY "
                   "points-to_graph_id DESC LIMIT 1";
          } else if (TableName == "ifds_ide_summary") {
            return "SELECT ifds_ide_summary_id FROM ifds_ide_summary ORDER BY "
                   "ifds_ide_summary_id DESC LIMIT 1";
          } else {
            throw invalid_argument(
                "Cannot look up next free id, because table does not exist.");
          }
        }()));
    unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
    if (res->next()) {
      id = res->getInt(1);
      id++;
    }
  } catch (sql::SQLException &e) {
    SQL_STD_ERROR_HANDLING;
  }
  return id;
}

int DBConn::getProjectID(const string &Identifier) {
  int projectID = -1;
  try {
    unique_ptr<sql::PreparedStatement> pstmt(conn->prepareStatement(
        "SELECT project_id FROM project WHERE identifier=(?)"));
    pstmt->setString(1, Identifier);
    unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
    if (res->next()) {
      projectID = res->getInt(1);
    }
    return projectID;
  } catch (sql::SQLException &e) {
    SQL_STD_ERROR_HANDLING;
  }
  return projectID;
}

int DBConn::getModuleID(const string &Identifier) {
  int moduleID = -1;
  try {
    unique_ptr<sql::PreparedStatement> pstmt(conn->prepareStatement(
        "SELECT module_id FROM module WHERE identifier=(?)"));
    pstmt->setString(1, Identifier);
    unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
    if (res->next()) {
      moduleID = res->getInt(1);
    }
    return moduleID;
  } catch (sql::SQLException &e) {
    SQL_STD_ERROR_HANDLING;
  }
  return moduleID;
}

int DBConn::getFunctionID(const string &Identifier) {
  int functionID = -1;
  try {
    unique_ptr<sql::PreparedStatement> pstmt(conn->prepareStatement(
        "SELECT function_id FROM function WHERE identifier=(?)"));
    pstmt->setString(1, Identifier);
    unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
    if (res->next()) {
      functionID = res->getInt(1);
    }
    return functionID;
  } catch (sql::SQLException &e) {
    SQL_STD_ERROR_HANDLING;
  }
  return functionID;
}

int DBConn::getGlobalVariableID(const string &Identifier) {
  int globalVariableID = 0;
  try {
    unique_ptr<sql::PreparedStatement> pstmt(conn->prepareStatement(
        "SELECT global_variable_id FROM global_variable WHERE identifier=(?)"));
    pstmt->setString(1, Identifier);
    unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
    if (res->next()) {
      globalVariableID = res->getInt(1);
    }
    return globalVariableID;
  } catch (sql::SQLException &e) {
    SQL_STD_ERROR_HANDLING;
  }
  return globalVariableID;
}

int DBConn::getTypeID(const string &Identifier) {
  int typeID = 0;
  try {
    unique_ptr<sql::PreparedStatement> pstmt(conn->prepareStatement(
        "SELECT type_id FROM type WHERE identifier=(?)"));
    pstmt->setString(1, Identifier);
    unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
    if (res->next()) {
      typeID = res->getInt(1);
    }
    return typeID;
  } catch (sql::SQLException &e) {
    SQL_STD_ERROR_HANDLING;
  }
  return typeID;
}

DBConn &DBConn::getInstance() {
  static DBConn instance;
  return instance;
}

string DBConn::getDBName() { return db_schema_name; }

bool DBConn::insertModule(const string &ProjectName,
                          const llvm::Module *module) {
  try {
    // Check if the project already exists, otherwise add a new entry
    int projectID = getProjectID(ProjectName);
    if (projectID == -1) {
      projectID = getNextAvailableID("project");
      unique_ptr<sql::PreparedStatement> ppstmt(conn->prepareStatement(
          "INSERT INTO project (project_id,identifier) VALUES(?,?)"));
      ppstmt->setInt(1, projectID);
      ppstmt->setString(2, ProjectName);
      ppstmt->executeUpdate();
    }
    // Write module
    string identifier(module->getModuleIdentifier());
    string ir_mod_buffer;
    llvm::raw_string_ostream rso(ir_mod_buffer);
    llvm::WriteBitcodeToFile(module, rso);
    rso.flush();
    istringstream ist(ir_mod_buffer);
    size_t hash_value = hash<string>()(ir_mod_buffer);
    unique_ptr<sql::PreparedStatement> mpstmt(conn->prepareStatement(
        "INSERT INTO module (module_id,identifier,hash,code) VALUES(?,?,?,?)"));
    int moduleID = getNextAvailableID("module");
    mpstmt->setInt(1, moduleID);
    mpstmt->setString(2, identifier);
    mpstmt->setString(3, to_string(hash_value));
    mpstmt->setBlob(4, &ist);
    mpstmt->executeUpdate();
    // Fill project - module relation
    cout << "PROJECT ID IS: " << projectID << endl;
    unique_ptr<sql::PreparedStatement> pmrstmt(conn->prepareStatement(
        "INSERT INTO project_has_module(project_id,module_id) VALUES(?,?)"));
    pmrstmt->setInt(1, projectID);
    pmrstmt->setInt(2, moduleID);
    pmrstmt->executeUpdate();
    // Write globals
    for (auto &G : module->globals()) {
      int globalID = getNextAvailableID("global_variable");
      unique_ptr<sql::PreparedStatement> gpstmt(conn->prepareStatement(
          "INSERT INTO global_variable "
          "(global_variable_id,identifier,declaration) VALUES (?,?,?)"));
      gpstmt->setInt(1, globalID);
      gpstmt->setString(2, G.getName().str());
      gpstmt->setBoolean(3, G.isDeclaration());
      gpstmt->executeUpdate();
      // Fill module - global relation
      unique_ptr<sql::PreparedStatement> grpstmt(conn->prepareStatement(
          "INSERT INTO module_has_global_variable "
          "(module_id,global_variable_id) VALUES (?,?)"));
      grpstmt->setInt(1, moduleID);
      grpstmt->setInt(2, globalID);
      grpstmt->executeUpdate();
    }
    // Write functions
    for (auto &F : *module) {
      int functionID = getNextAvailableID("function");
      string ir_fun_buffer = llvmIRToString(&F);
      hash_value = hash<string>()(ir_fun_buffer);
      unique_ptr<sql::PreparedStatement> fpstmt(conn->prepareStatement(
          "INSERT INTO function (identifier,declaration,hash) VALUES (?,?,?)"));
      fpstmt->setString(1, F.getName().str());
      fpstmt->setBoolean(2, F.isDeclaration());
      fpstmt->setString(3, to_string(hash_value));
      fpstmt->executeUpdate();
      // Fill module - function relation
      unique_ptr<sql::PreparedStatement> frpstmt(
          conn->prepareStatement("INSERT INTO module_has_function "
                                 "(module_id,function_id) VALUES (?,?)"));
      frpstmt->setInt(1, moduleID);
      frpstmt->setInt(2, functionID);
      frpstmt->executeUpdate();
    }
    // // Write types
    for (auto &ST : module->getIdentifiedStructTypes()) {
      int typeID = getNextAvailableID("type");
      unique_ptr<sql::PreparedStatement> stpstmt(
          conn->prepareStatement("INSERT INTO type (identifier) VALUES (?)"));
      stpstmt->setString(1, ST->getName().str());
      stpstmt->executeUpdate();
      // Fill module - type relation
      unique_ptr<sql::PreparedStatement> trpstmt(
          conn->prepareStatement("INSERT INTO module_has_type "
                                 "(module_id,type_id) VALUES (?,?)"));
      trpstmt->setInt(1, moduleID);
      trpstmt->setInt(2, typeID);
      trpstmt->executeUpdate();
    }
  } catch (sql::SQLException &e) {
    SQL_STD_ERROR_HANDLING;
  }
  return false;
}

unique_ptr<llvm::Module> DBConn::getModule(const string &identifier,
                                           llvm::LLVMContext &Context) {
  try {
    unique_ptr<sql::PreparedStatement> pstmt(
        conn->prepareStatement("SELECT code FROM module WHERE identifier=?"));
    pstmt->setString(1, identifier);
    unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
    if (res->next()) {
      istream *ist = res->getBlob("code");
      string ir_mod_buffer(istreambuf_iterator<char>(*ist), {});
      // parse the freshly retrieved byte sequence into an llvm::Module
      llvm::SMDiagnostic ErrorDiagnostics;
      unique_ptr<llvm::MemoryBuffer> MemBuffer =
          llvm::MemoryBuffer::getMemBuffer(ir_mod_buffer);
      unique_ptr<llvm::Module> Mod =
          llvm::parseIR(*MemBuffer, ErrorDiagnostics, Context);
      // restore module identifier
      Mod->setModuleIdentifier(identifier);
      // check if everything has worked-out
      bool broken_debug_info = false;
      if (llvm::verifyModule(*Mod, &llvm::errs(), &broken_debug_info)) {
        cout << "verifying module failed!" << endl;
      }
      if (broken_debug_info) {
        cout << "debug info is broken!" << endl;
      }
      return Mod;
    } else {
      return nullptr;
    }
  } catch (sql::SQLException &e) {
    SQL_STD_ERROR_HANDLING;
    return nullptr;
  }
}

// bool insertLLVMStructHierarchyGraph(LLVMTypeHierarchy::digraph_t g) {
//   typename boost::graph_traits<LLVMTypeHierarchy::digraph_t>::edge_iterator
//   ei_start, e_end;
// 	for (tie(ei_start, e_end) = boost::edges(g); ei_start != e_end;
// ++ei_start) {
// 		auto source = boost::source(*ei_start, g);
// 		auto target = boost::target(*ei_start, g);
// 		cout << g[source].name << " --> " << g[target].name << "\n";
// 	}
//   return false;
// }

void operator>>(DBConn &db, const LLVMTypeHierarchy &STH) {
  cout << "READ STH FROM DB\n";
}

void operator<<(DBConn &db, const PointsToGraph &PTG) {
  // UNRECOVERABLE_CXX_ERROR_COND(db.isSynchronized(), "DBConn not synchronized
  // with an ProjectIRCompiledDB object!");
  // static PHSStringConverter converter(*db.IRDB);
  // hexastore::Hexastore h("ptg_hexastore.db");
  // cout << "writing points-to graph of function ";
  // for (const auto& fname : PTG.ContainedFunctions) {
  //   cout << fname << " ";
  // }
  // cout << "into hexastore!\n";
  // typename boost::graph_traits<PointsToGraph::graph_t>::edge_iterator
  // ei_start, e_end;
  // // iterate over all edges and store the vertices and edges (i.e. string
  // rep. of the vertices/edges) in the hexastore
  // for (tie(ei_start, e_end) = boost::edges(PTG.ptg); ei_start != e_end;
  // ++ei_start) {
  //   auto source = boost::source(*ei_start, PTG.ptg);
  //   auto target = boost::target(*ei_start, PTG.ptg);
  //   string hs_source_rep =
  //   converter.PToHStoreStringRep(PTG.ptg[source].value);
  //   string hs_target_rep =
  //   converter.PToHStoreStringRep(PTG.ptg[target].value);
  //   cout << "source: " << PTG.ptg[source].ir_code << " | hex: " <<
  //   hs_source_rep << '\n'
  //        << "target: " << PTG.ptg[target].ir_code << " | hex: " <<
  //        hs_target_rep << '\n';
  //   if (PTG.ptg[*ei_start].value) {
  //     string hs_edge_rep =
  //     converter.PToHStoreStringRep(PTG.ptg[*ei_start].value);
  //     cout << "edge ir_code: " << PTG.ptg[*ei_start].ir_code << " | edge id:
  //     " << PTG.ptg[*ei_start].id
  //          << " | edge hex: " << hs_edge_rep << '\n';
  //     h.put({{hs_source_rep, hs_edge_rep, hs_target_rep}});
  //   } else {
  //     h.put({{hs_source_rep, "---", hs_target_rep}});
  //   }
  //   cout << '\n';
  // }
  // cout << "vertices without edges:\n";
  // typedef boost::graph_traits<PointsToGraph::graph_t>::vertex_iterator
  // vertex_iterator_t;
  // typename boost::graph_traits<PointsToGraph::graph_t>::out_edge_iterator ei,
  // ei_end;
  // // check for vertices without any adjacent edges and store them in the
  // hexastore
  // for (pair<vertex_iterator_t, vertex_iterator_t> vp =
  // boost::vertices(PTG.ptg); vp.first != vp.second; ++vp.first) {
  //   boost::tie(ei, ei_end) = boost::out_edges(*vp.first, PTG.ptg);
  //   if(ei == ei_end) {
  //     string hs_vertex_rep =
  //     converter.PToHStoreStringRep(PTG.ptg[*vp.first].value);
  //     cout << "id: " << PTG.ptg[*vp.first].id
  //          << " | ir_code: " << PTG.ptg[*vp.first].ir_code
  //          << " | hex: " << hs_vertex_rep << "\n\n";
  //     h.put({{hs_vertex_rep,"---","---"}});
  //   }
  // }
  // cout << "print the points-to graph:\n";
  // PTG.print();
  // cout << "\n\n\n";
}

void operator>>(DBConn &db, PointsToGraph &PTG) {
  // 	UNRECOVERABLE_CXX_ERROR_COND(db.isSynchronized(), "DBConn not
  // synchronized with an ProjectIRCompiledDB object!");
  // 	static PHSStringConverter converter(*db.IRDB);
  //   hexastore::Hexastore h("ptg_hexastore.db");
  //   // using set instead of vector because searching in a set is easier
  // 	set<string> fnames(PTG.ContainedFunctions);
  //   cout << "reading points-to graph ";
  //   for (const auto& fname : PTG.ContainedFunctions) {
  //     cout << fname << " ";
  //   }
  //   cout << "from hexastore!\n";
  //   auto result = h.get({{"?", "?", "?"}});
  //   for_each(result.begin(), result.end(), [fnames,&PTG](hexastore::hs_result
  //   r){
  // //    cout << r << endl;
  //     // holds the function name, if it's not a global variable; otherwise
  //     the name of
  //     // the global variable
  //     string curr_subject_fname = r.subject.substr(0, r.subject.find("."));
  //     string curr_object_fname = r.object.substr(0, r.object.find("."));
  //     bool mergedPTG = PTG.ContainedFunctions.size() > 1;
  //     // check if the current result is part the PTG, i.e. the function name
  //     of the
  //     // function name matches one of the functions from the PTG
  //     if (fnames.find(curr_subject_fname) != fnames.end()) {
  //       // if the PTG is not a merged points-to graph, one of the following
  //       is true:
  //       // (1) subject fname and object fname are equal
  //       // (2) object is global variable
  //       // if the PTG is a merged points-to graph, then there are no single
  //       nodes
  //       if ((mergedPTG && r.object != "---") ||
  //         (!mergedPTG && ((curr_subject_fname == curr_object_fname) ||
  //                         (curr_object_fname.find(".") == string::npos)))) {
  //         const llvm::Value *source =
  //         converter.HStoreStringRepToP(r.subject);
  //         // check if the node was already created
  //         if (PTG.value_vertex_map.find(source) ==
  //         PTG.value_vertex_map.end()) {
  //           PTG.value_vertex_map[source] = boost::add_vertex(PTG.ptg);
  //           PTG.ptg[PTG.value_vertex_map[source]] =
  //           PointsToGraph::VertexProperties(source);
  //         }
  //         // check if the target node (object) exists
  //         if (r.object != "---") {
  //           const llvm::Value *target =
  //           converter.HStoreStringRepToP(r.object);
  //           if (PTG.value_vertex_map.find(target) ==
  //           PTG.value_vertex_map.end()) {
  //             PTG.value_vertex_map[target] = boost::add_vertex(PTG.ptg);
  //             PTG.ptg[PTG.value_vertex_map[target]] =
  //             PointsToGraph::VertexProperties(target);
  //           }
  //           // create an (labeled) edge
  //           if (r.predicate != "---") {
  //             const llvm::Value *edge =
  //             converter.HStoreStringRepToP(r.predicate);
  //             boost::add_edge(PTG.value_vertex_map[source],
  //                             PTG.value_vertex_map[target],
  //                             PointsToGraph::EdgeProperties(edge),
  //                             PTG.ptg);
  //           } else {
  //             boost::add_edge(PTG.value_vertex_map[source],
  //             PTG.value_vertex_map[target],PTG.ptg);
  //           }
  //         }
  //       }
  //     }
  //     // check if subject is a global variable
  //     else if (r.subject.find(".") == string::npos) {
  //       cout << "WE FOUND THE GLOBAL VARIABLE " << r.subject << '\n';
  //       const llvm::Value *GV = converter.HStoreStringRepToP(r.subject);
  //       // check if the global variable has no adjacent edges
  //       if (r.object == "---") {
  //         // check if the global variable is part of the PTG by looking at
  //         user of the global variable
  //         for (auto User: GV->users()) {
  //           if (const llvm::Instruction *I =
  //           llvm::dyn_cast<llvm::Instruction>(User)) {
  //             string user_full_fname = I->getFunction()->getName().str();
  //             string user_fname = user_full_fname.substr(0,
  //             user_full_fname.find("."));
  //             cout << "GV user: " << user_fname << '\n';
  //             if (fnames.find(user_fname) != fnames.end()) {
  //               // create vertex without adjacent edges for the global
  //               variable
  //               if (PTG.value_vertex_map.find(GV) ==
  //               PTG.value_vertex_map.end()) {
  //                 PTG.value_vertex_map[GV] = boost::add_vertex(PTG.ptg);
  //                 PTG.ptg[PTG.value_vertex_map[GV]] =
  //                 PointsToGraph::VertexProperties(GV);
  //               }
  //               break;
  //             }
  //           }
  //         }
  //       }
  //       // check if the target node of the global variable (i.e. object) is
  //       not a global variable itself
  //       else if (fnames.find(curr_object_fname) != fnames.end()) {
  //         curr_object_fname = r.object.substr(0, r.object.find("."));
  //         // check if the target node of the global variable is part the PTG
  //         if (fnames.find(curr_object_fname) != fnames.end()) {
  //           if (PTG.value_vertex_map.find(GV) == PTG.value_vertex_map.end())
  //           {
  //             PTG.value_vertex_map[GV] = boost::add_vertex(PTG.ptg);
  //             PTG.ptg[PTG.value_vertex_map[GV]] =
  //             PointsToGraph::VertexProperties(GV);
  //           }
  //           const llvm::Value *target =
  //           converter.HStoreStringRepToP(r.object);
  //           if (PTG.value_vertex_map.find(target) ==
  //           PTG.value_vertex_map.end()) {
  //             PTG.value_vertex_map[target] = boost::add_vertex(PTG.ptg);
  //             PTG.ptg[PTG.value_vertex_map[target]] =
  //             PointsToGraph::VertexProperties(target);
  //           }
  //           // create an (labeled) edge
  //           if (r.predicate != "---") {
  //             const llvm::Value *edge =
  //             converter.HStoreStringRepToP(r.predicate);
  //             boost::add_edge(PTG.value_vertex_map[GV],
  //                             PTG.value_vertex_map[target],
  //                             PointsToGraph::EdgeProperties(edge),
  //                             PTG.ptg);
  //           } else {
  //             boost::add_edge(PTG.value_vertex_map[GV],
  //             PTG.value_vertex_map[target],PTG.ptg);
  //           }
  //         }
  //       }
  // //      else if (curr_object_fname.find(".") == string::npos) {
  // //        cout << "subject and object are both global variables - is this
  // possible??\n"
  // //             << "if so, to what points-to graph does it belong??\n";
  // //      }
  //     }
  //   });
  //   cout << "print the restored points-to graph:\n";
  //   PTG.print();
  //   stringstream ss;
  //   for (auto f : PTG.ContainedFunctions)
  //     ss << f;
  //   ss << "_restored.dot";
  //   cout << ss.str() << '\n';
  //   PTG.printAsDot(ss.str());
  //   cout << "\n\n\n";
}

// void DBConn::storeLLVMBasedICFG(const LLVMBasedICFG &ICFG,
//                                 bool use_hs = false) {}

// LLVMBasedICFG
// DBConn::loadLLVMBasedICFGfromModule(const string &ModuleName,
//                                     bool use_hs = false) {}

// LLVMBasedICFG
// DBConn::loadLLVMBasedICFGfromModules(initializer_list<string> ModuleNames,
//                                      bool use_hs = false) {}

// LLVMBasedICFG
// DBConn::loadLLVMBasedICFGfromProject(const string &ProjectName,
//                                      bool use_hs = false) {}

// void DBConn::storePointsToGraph(const PointsToGraph &PTG, bool use_hs =
// false) {

// }

// PointsToGraph
// DBConn::loadPointsToGraphFromFunction(const string &FunctionName,
//                                       bool use_hs = false) {}

// void DBConn::storeLLVMTypeHierarchy(const LLVMTypeHierarchy &TH,
//                                     bool use_hs = false) {}

// LLVMTypeHierarchy
// DBConn::loadLLVMTypeHierarchyFromModule(const string &ModuleName,
//                                         bool use_hs = false) {}

// LLVMTypeHierarchy
// DBConn::loadLLVMTypeHierarchyFromModules(initializer_list<string>
// ModuleNames,
//                                          bool use_hs = false) {}

// LLVMTypeHierarchy
// DBConn::loadLLVMTypeHierarchyFromProject(const string &ProjectName,
//                                          bool use_hs = false) {}

// void DBConn::storeIDESummary(const IDESummary &S) {}

// IDESummary DBConn::loadIDESummary(const string &FunctionName,
//                                          const string &AnalysisName) {}

void DBConn::storeProjectIRDB(const string &ProjectName,
                              const ProjectIRDB &IRDB) {
  for (auto M : IRDB.getAllModules()) {
    insertModule(ProjectName, M);
  }
}

ProjectIRDB DBConn::loadProjectIRDB(const string &ProjectName) {
  unique_ptr<sql::PreparedStatement> pstmt(conn->prepareStatement(
      "SELECT identifier "
      "FROM project_has_module NATURAL JOIN module WHERE project_id=(SELECT "
      "project_id FROM project WHERE identifier=?)"));
  pstmt->setString(1, ProjectName);
  unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
  ProjectIRDB IRDB;
  while (res->next()) {
    string module_identifier = res->getString("identifier");
    cout << "module identifier: " << module_identifier << endl;
    llvm::LLVMContext *C = new llvm::LLVMContext;
    unique_ptr<llvm::Module> M = getModule(module_identifier, *C);
    IRDB.insertModule(move(M));
  }
  return IRDB;
}

bool DBConn::schemeExists() {
  unique_ptr<sql::PreparedStatement> pstmt(conn->prepareStatement(
      "SELECT SCHEMA_NAME FROM INFORMATION_SCHEMA.SCHEMATA WHERE "
      "SCHEMA_NAME=?"));
  pstmt->setString(1, db_schema_name);
  unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
  return res->next();
}

void DBConn::buildDBScheme() {
//   const static string Scheme(
// #include "createScheme.sql"
//       );
//       cout << Scheme << endl;
//   unique_ptr<sql::Statement> stmt(conn->createStatement());
//   stmt->execute(Scheme);
}

void DBConn::dropDBAndRebuildScheme() {}
