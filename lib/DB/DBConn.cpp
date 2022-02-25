/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

/*
 * DBConn.cpp
 *
 *  Created on: 23.08.2016
 *      Author: pdschbrt
 */
/*
#include <thread>

#include "llvm/Bitcode/BitcodeWriter.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Verifier.h"
#include "llvm/IRReader/IRReader.h"
#include "llvm/Support/SourceMgr.h"

#include "cppconn/driver.h"
#include "cppconn/exception.h"
#include "cppconn/prepared_statement.h"
#include "cppconn/resultset.h"
#include "cppconn/statement.h"

#include "phasar/DB/DBConn.h"
#include "phasar/DB/Hexastore.h"
#include "phasar/PhasarLLVM/Pointer/VTable.h"

#include "phasar/Utils/IO.h"
#include "phasar/Utils/LLVMShorthands.h"
#include "phasar/Utils/Logger.h"
#include "phasar/Utils/Macros.h"

using namespace psr;
using namespace std;

namespace psr {

const string DBConn::db_schema_name = "phasardb";
const string DBConn::db_user = "root";
const string DBConn::db_password = "1234";
const string DBConn::db_server_address = "tcp://127.0.0.1:3306";

DBConn::DBConn() {
  auto lg = lg::get();
  LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG) << "Connect to database");
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
  LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG) << "Close database connection");
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

set<int> DBConn::getFunctionID(const string &Identifier) {
  set<int> functionIDs;
  try {
    unique_ptr<sql::PreparedStatement> pstmt(conn->prepareStatement(
        "SELECT function_id FROM function WHERE identifier=(?)"));
    pstmt->setString(1, Identifier);
    unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
    while (res->next()) {
      functionIDs.insert(res->getInt(1));
    }
    return functionIDs;
  } catch (sql::SQLException &e) {
    SQL_STD_ERROR_HANDLING;
  }
  return functionIDs;
}

size_t DBConn::getFunctionHash(const unsigned functionID) {
  size_t hash_value = 0;
  try {
    unique_ptr<sql::PreparedStatement> pstmt(conn->prepareStatement(
        "SELECT hash FROM function WHERE function_id=(?)"));
    pstmt->setInt(1, functionID);
    unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
    if (res->next()) {
      string hash_string = res->getString(1);
      stringstream sstream(hash_string);
      sstream >> hash_value;
    }
    return hash_value;
  } catch (sql::SQLException &e) {
    SQL_STD_ERROR_HANDLING;
  }
  return hash_value;
}

size_t DBConn::getModuleHash(const unsigned moduleID) {
  size_t hash_value = 0;
  try {
    unique_ptr<sql::PreparedStatement> pstmt(
        conn->prepareStatement("SELECT hash FROM module WHERE module_id=(?)"));
    pstmt->setInt(1, moduleID);
    unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
    if (res->next()) {
      string hash_string = res->getString(1);
      stringstream sstream(hash_string);
      sstream >> hash_value;
    }
    return hash_value;
  } catch (sql::SQLException &e) {
    SQL_STD_ERROR_HANDLING;
  }
  return hash_value;
}

set<int> DBConn::getGlobalVariableID(const string &Identifier) {
  set<int> globalVariableIDs;
  try {
    unique_ptr<sql::PreparedStatement> pstmt(conn->prepareStatement(
        "SELECT global_variable_id FROM global_variable WHERE identifier=(?)"));
    pstmt->setString(1, Identifier);
    unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
    while (res->next()) {
      globalVariableIDs.insert(res->getInt(1));
    }
    return globalVariableIDs;
  } catch (sql::SQLException &e) {
    SQL_STD_ERROR_HANDLING;
  }
  return globalVariableIDs;
}

int DBConn::getTypeID(const string &Identifier) {
  int typeID = -1;
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

set<int> DBConn::getModuleIDsFromProject(const string &Identifier) {
  set<int> moduleIDs;
  try {
    unique_ptr<sql::PreparedStatement> pstmt(conn->prepareStatement(
        "SELECT module_id "
        "FROM project_has_module WHERE project_id=(SELECT "
        "project_id FROM phasardb.project WHERE identifier=?)"));
    pstmt->setString(1, Identifier);
    unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
    while (res->next()) {
      moduleIDs.insert(res->getInt(1));
    }
    return moduleIDs;
  } catch (sql::SQLException &e) {
    SQL_STD_ERROR_HANDLING;
  }
  return moduleIDs;
}

int DBConn::getModuleIDFromTypeID(const unsigned typeID) {
  int moduleID = -1;
  try {
    unique_ptr<sql::PreparedStatement> pstmt(conn->prepareStatement(
        "SELECT module_id FROM module_has_type WHERE type_id=?)"));
    pstmt->setInt(1, typeID);
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

int DBConn::getModuleIDFromFunctionID(const unsigned functionID) {
  int moduleID = -1;
  try {
    unique_ptr<sql::PreparedStatement> pstmt(conn->prepareStatement(
        "SELECT module_id FROM module_has_function WHERE type_id=?)"));
    pstmt->setInt(1, functionID);
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

set<int> DBConn::getAllTypeHierarchyIDs() {
  set<int> THIDs;
  try {
    unique_ptr<sql::PreparedStatement> pstmt(
        conn->prepareStatement("SELECT type_hierarchy_id FROM type_hierarchy"));
    unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
    while (res->next()) {
      THIDs.insert(res->getInt("type_hierarchy_id"));
    }
    return THIDs;
  } catch (sql::SQLException &e) {
    SQL_STD_ERROR_HANDLING;
  }
  return THIDs;
}

set<int> DBConn::getAllModuleIDsFromTH(const unsigned typeHierarchyID) {
  set<int> moduleIDs;
  try {
    unique_ptr<sql::PreparedStatement> pstmt(conn->prepareStatement(
        "SELECT module_id FROM moduel_has_type_hierarchy WHERE "
        "type_hierarchy_id=?"));
    pstmt->setInt(1, typeHierarchyID);
    unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
    while (res->next()) {
      moduleIDs.insert(res->getInt("module_id"));
    }
    return moduleIDs;
  } catch (sql::SQLException &e) {
    SQL_STD_ERROR_HANDLING;
  }
  return moduleIDs;
}

QueryReturnCode DBConn::moduleHasTypeHierarchy(const unsigned moduleID) {
  try {
    unique_ptr<sql::PreparedStatement> pstmt(conn->prepareStatement(
        "SELECT module_id FROM module_has_type_hierarchy WHERE module_id=?"));
    pstmt->setInt(1, moduleID);
    unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
    return res->next() ? QueryReturnCode::DBTrue : QueryReturnCode::DBFalse;
  } catch (sql::SQLException &e) {
    SQL_STD_ERROR_HANDLING;
  }
  return QueryReturnCode::DBError;
}

QueryReturnCode
DBConn::globalVariableIsDeclaration(const unsigned globalVariableID) {
  try {
    unique_ptr<sql::PreparedStatement> pstmt(conn->prepareStatement(
        "SELECT declaration FROM global_variable WHERE global_variable_id=?"));
    pstmt->setInt(1, globalVariableID);
    unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
    if (res->next()) {
      return res->getBoolean("declaration") ? QueryReturnCode::DBTrue
                                            : QueryReturnCode::DBFalse;
    }
    return QueryReturnCode::DBError;
  } catch (sql::SQLException &e) {
    SQL_STD_ERROR_HANDLING;
  }
  return QueryReturnCode::DBError;
}

string DBConn::getDBName() { return db_schema_name; }

bool DBConn::insertGlobalVariable(const llvm::GlobalVariable &G,
                                  const unsigned moduleID) {
  try {
    set<int> globalVariableIDs = getGlobalVariableID(G.getGlobalIdentifier());
    cout << "TRYING TO STORE "
         << "[" << G.getGlobalIdentifier() << "," << G.isDeclaration()
         << " FROM MODULE " << moduleID << " GLOBAL IDS: ";
    for (auto id : globalVariableIDs)
      cout << id << " ";
    int newGlobalID = getNextAvailableID("global_variable");
    QueryReturnCode GIsDeclarataion =
        G.isDeclaration() ? QueryReturnCode::DBTrue : QueryReturnCode::DBFalse;
    bool newGlobVar = false;
    // Do not write duplicate global variables
    if (globalVariableIDs.size() == 0 ||
        (globalVariableIDs.size() == 1 &&
         globalVariableIsDeclaration(*globalVariableIDs.begin()) !=
             GIsDeclarataion)) {
      unique_ptr<sql::PreparedStatement> gpstmt(conn->prepareStatement(
          "INSERT INTO global_variable "
          "(global_variable_id,identifier,declaration) VALUES (?,?,?)"));
      gpstmt->setInt(1, newGlobalID);
      gpstmt->setString(2, G.getName().str());
      gpstmt->setBoolean(3, G.isDeclaration());
      gpstmt->executeUpdate();
      newGlobVar = true;
    }
    // Fill module - global relation
    unique_ptr<sql::PreparedStatement> grpstmt(
        conn->prepareStatement("INSERT INTO module_has_global_variable "
                               "(module_id,global_variable_id) VALUES (?,?)"));
    grpstmt->setInt(1, moduleID);
    // Find the ID of the global variable
    if (newGlobVar) {
      cout << "USING NEW ID: " << newGlobalID << '\n';
      grpstmt->setInt(2, newGlobalID);
    } else if (globalVariableIDs.size() == 1) {
      cout << "USING ALREADY AVAILABLE ID: " << *globalVariableIDs.begin()
           << '\n';
      grpstmt->setInt(2, *globalVariableIDs.begin());
    } else {
      if (GIsDeclarataion ==
          globalVariableIsDeclaration(*globalVariableIDs.begin())) {
        cout << "USING: " << *globalVariableIDs.begin() << '\n';
        grpstmt->setInt(2, *globalVariableIDs.begin());
      } else {
        cout << "USING: " << *(++globalVariableIDs.begin()) << '\n';
        grpstmt->setInt(2, *(++globalVariableIDs.begin()));
      }
    }
    grpstmt->executeUpdate();
  } catch (sql::SQLException &e) {
    SQL_STD_ERROR_HANDLING;
  }
  return false;
}

bool DBConn::insertFunction(const llvm::Function &F, const unsigned moduleID) {
  try {
    set<int> functionIDs = getFunctionID(F.getName().str());
    int newFunctionID = getNextAvailableID("function");
    int matchingFID = -1;
    bool newFunc = true;
    string ir_fun_buffer = llvmIRToString(&F);
    size_t hash_value = hash<string>()(ir_fun_buffer);
    // Check if there is already a function with the same identifier
    // AND hash value
    for (auto fid : functionIDs) {
      if (hash_value == getFunctionHash(fid)) {
        newFunc = false;
        matchingFID = fid;
        break;
      }
    }
    // Do not write duplicate functions
    if (newFunc) {
      unique_ptr<sql::PreparedStatement> fpstmt(conn->prepareStatement(
          "INSERT INTO function (function_id,identifier,declaration,hash) "
          "VALUES (?,?,?,?)"));
      fpstmt->setInt(1, newFunctionID);
      fpstmt->setString(2, F.getName().str());
      fpstmt->setBoolean(3, F.isDeclaration());
      fpstmt->setString(4, to_string(hash_value));
      fpstmt->executeUpdate();
    }
    // Fill module - function relation
    unique_ptr<sql::PreparedStatement> frpstmt(
        conn->prepareStatement("INSERT INTO module_has_function "
                               "(module_id,function_id) VALUES (?,?)"));
    frpstmt->setInt(1, moduleID);
    if (newFunc) {
      frpstmt->setInt(2, newFunctionID);
    } else {
      frpstmt->setInt(2, matchingFID);
    }
    frpstmt->executeUpdate();
  } catch (sql::SQLException &e) {
    SQL_STD_ERROR_HANDLING;
  }
  return false;
}

bool DBConn::insertType(const llvm::StructType &ST, const unsigned moduleID) {
  try {
    int newTypeID = getNextAvailableID("type");
    int typeID = getTypeID(ST.getName().str());
    bool newType = false;
    // Do not write duplicate types
    if (typeID == -1) {
      unique_ptr<sql::PreparedStatement> stpstmt(conn->prepareStatement(
          "INSERT INTO type (type_id, identifier) VALUES (?,?)"));
      stpstmt->setInt(1, newTypeID);
      stpstmt->setString(2, ST.getName().str());
      stpstmt->executeUpdate();
      newType = true;
    }
    // Fill module - type relation
    unique_ptr<sql::PreparedStatement> trpstmt(
        conn->prepareStatement("INSERT INTO module_has_type "
                               "(module_id,type_id) VALUES (?,?)"));
    trpstmt->setInt(1, moduleID);
    if (newType) {
      trpstmt->setInt(2, newTypeID);
    } else {
      trpstmt->setInt(2, typeID);
    }
    trpstmt->executeUpdate();
  } catch (sql::SQLException &e) {
    SQL_STD_ERROR_HANDLING;
  }
  return false;
}

bool DBConn::insertVTable(const VTable &VTBL, const string &TypeName,
                          const string &ProjectName) {
  try {
    int typeID = getTypeID(TypeName);
    // module ID of the module that contains the current type
    for (auto fname : VTBL) {
      // Identify the corresponding function id
      unique_ptr<sql::PreparedStatement> pstmt(conn->prepareStatement(
          "SELECT DISTINCT function_id, declaration FROM function "
          "NATURAL JOIN module_has_function NATURAL JOIN project_has_module "
          "WHERE function.identifier=? AND project_id=( "
          "SELECT project_id FROM project WHERE project.identifier=?)"));
      pstmt->setString(1, fname);
      pstmt->setString(2, ProjectName);
      unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
      int fid = -1;
      while (res->next()) {
        fid = res->getInt("function_id");
        if (!res->getBoolean("declaration"))
          break;
      }
      unique_ptr<sql::PreparedStatement> tvpstmt(conn->prepareStatement(
          "INSERT INTO type_has_virtual_function "
          "(type_id,function_id,vtable_index) VALUES(?,?,?)"));
      tvpstmt->setInt(1, typeID);
      tvpstmt->setInt(2, fid);
      tvpstmt->setInt(3, VTBL.getEntryByFunctionName(fname));
      tvpstmt->executeUpdate();
    }
  } catch (sql::SQLException &e) {
    SQL_STD_ERROR_HANDLING;
  }
  return false;
}

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
    for (const llvm::GlobalVariable &G : module->globals()) {
      insertGlobalVariable(G, moduleID);
    }
    // Write functions
    for (const llvm::Function &F : *module) {
      insertFunction(F, moduleID);
    }
    // Write types
    for (const llvm::StructType *ST : module->getIdentifiedStructTypes()) {
      insertType(*ST, moduleID);
    }
  } catch (sql::SQLException &e) {
    SQL_STD_ERROR_HANDLING;
  }
  return false;
}

// TODO use module id instead of the module identifier to avoid ambiguity
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
      if (Mod.get() == nullptr ||
          llvm::verifyModule(*Mod, &llvm::errs(), &broken_debug_info)) {
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
  // Hexastore h("ptg_hexastore.db");
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
  //   Hexastore h("ptg_hexastore.db");
  //   // using set instead of vector because searching in a set is easier
  // 	set<string> fnames(PTG.ContainedFunctions);
  //   cout << "reading points-to graph ";
  //   for (const auto& fname : PTG.ContainedFunctions) {
  //     cout << fname << " ";
  //   }
  //   cout << "from hexastore!\n";
  //   auto result = h.get({{"?", "?", "?"}});
  //   for_each(result.begin(), result.end(), [fnames,&PTG](hs_result
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

void DBConn::storeLLVMBasedICFG(const LLVMBasedICFG &ICFG,
                                const string &ProjectName, bool use_hs) {
  try {
  } catch (sql::SQLException &e) {
    SQL_STD_ERROR_HANDLING;
  }
}

LLVMBasedICFG DBConn::loadLLVMBasedICFGfromModule(const string &ModuleName,
                                                  bool use_hs) {
  try {
  } catch (sql::SQLException &e) {
    SQL_STD_ERROR_HANDLING;
  }
  ProjectIRDB IRDB(IRDBOptions::NONE);
  LLVMTypeHierarchy TH;
  return LLVMBasedICFG(TH, IRDB);
}

LLVMBasedICFG
DBConn::loadLLVMBasedICFGfromModules(initializer_list<string> ModuleNames,
                                     bool use_hs) {
  try {
  } catch (sql::SQLException &e) {
    SQL_STD_ERROR_HANDLING;
  }
  ProjectIRDB IRDB(IRDBOptions::NONE);
  LLVMTypeHierarchy TH;
  return LLVMBasedICFG(TH, IRDB);
}

LLVMBasedICFG DBConn::loadLLVMBasedICFGfromProject(const string &ProjectName,
                                                   bool use_hs) {
  try {
  } catch (sql::SQLException &e) {
    SQL_STD_ERROR_HANDLING;
  }
  ProjectIRDB IRDB(IRDBOptions::NONE);
  LLVMTypeHierarchy TH;
  return LLVMBasedICFG(TH, IRDB);
}

void DBConn::storePointsToGraph(const PointsToGraph &PTG,
                                const string &ProjectName, bool use_hs) {
  try {
  } catch (sql::SQLException &e) {
    SQL_STD_ERROR_HANDLING;
  }
}

PointsToGraph DBConn::loadPointsToGraphFromFunction(const string &FunctionName,
                                                    bool use_hs) {
  try {
  } catch (sql::SQLException &e) {
    SQL_STD_ERROR_HANDLING;
  }
  return PointsToGraph();
}

void DBConn::storeLTHGraphToHex(const LLVMTypeHierarchy::bidigraph_t &G,
                                const string hex_id) {
  Hexastore h(hex_id);
  typename boost::graph_traits<LLVMTypeHierarchy::bidigraph_t>::edge_iterator
      ei_start,
      e_end;
  for (tie(ei_start, e_end) = boost::edges(G); ei_start != e_end; ++ei_start) {
    auto source = boost::source(*ei_start, G);
    auto target = boost::target(*ei_start, G);
    h.put({{G[source].name, "-->", G[target].name}});
  }
  typedef boost::graph_traits<LLVMTypeHierarchy::bidigraph_t>::vertex_iterator
      vertex_iterator_t;
  typename boost::graph_traits<
      LLVMTypeHierarchy::bidigraph_t>::out_edge_iterator ei,
      ei_end;
  for (pair<vertex_iterator_t, vertex_iterator_t> vp = boost::vertices(G);
       vp.first != vp.second; ++vp.first) {
    boost::tie(ei, ei_end) = boost::out_edges(*vp.first, G);
    if (ei == ei_end) {
      string hs_vertex_rep = G[*vp.first].name;
      h.put({{hs_vertex_rep, "---", "---"}});
    }
  }
  auto result = h.get({{"?", "?", "?"}});
  for_each(result.begin(), result.end(),
           [](hs_result r) { cout << r << endl; });
}

void DBConn::storeLLVMTypeHierarchy(LLVMTypeHierarchy &TH,
                                    const string &ProjectName, bool use_hs) {
  // try {
  //   // TH contains new information if one of the following is true:
  //   // (1) one or more modules are not stored in the database
  //   // (2) one or more modules are not listed in the
  //   //     module - type_hierarchy relation
  //   // If one of the above is true, the TH will be stored and all missing
  //   // or new information (e.g. modules, VTables) will be stored as well.
  //   bool THContainsNewInformation = false;
  //   // Initialize the transaction
  //   unique_ptr<sql::Statement> stmt(conn->createStatement());
  //   stmt->execute("START TRANSACTION");
  //   for (auto M : TH.contained_modules) {
  //     // Check if all modules contained in the TH are already stored in the
  //     // database. At the same time, all types contained in the modules will
  //     // be stored (if not already stored).
  //     int moduleID = getModuleID(M->getModuleIdentifier());
  //     if (moduleID == -1 || getModuleHash(moduleID) != computeModuleHash(M))
  //     {
  //       insertModule(ProjectName, M);
  //       THContainsNewInformation = true;
  //     } else if (!THContainsNewInformation &&
  //                moduleHasTypeHierarchy(moduleID) ==
  //                QueryReturnCode::DBFalse) {
  //       THContainsNewInformation = true;
  //     }
  //   }
  //   if (THContainsNewInformation) {
  //     // Write type hierarchy
  //     unique_ptr<sql::PreparedStatement> thpstmt(
  //         conn->prepareStatement("INSERT INTO type_hierarchy "
  //                                "(type_hierarchy_id,representation,"
  //                                "representation_ref) VALUES(?,?,?)"));
  //     int THID = getNextAvailableID("type_hierarchy");
  //     thpstmt->setInt(1, THID);
  //     if (use_hs) {
  //       // Write type hierarchy graph to hexastore
  //       string hex_id("LTH_" + to_string(THID) + "_hex.db");
  //       storeLTHGraphToHex(TH.g, hex_id);
  //       thpstmt->setNull(2, 0);
  //       thpstmt->setString(3, hex_id);
  //     } else {
  //       // Write type hierarchy graph as dot
  //       stringstream sst;
  //       TH.printGraphAsDot(sst);
  //       sst.flush();
  //       cout << sst.str() << endl;
  //       thpstmt->setBlob(2, &sst);
  //       thpstmt->setNull(3, 0);
  //       // Write type hiearchy graph as dot file
  //       // ofstream myfile("LTHGraph.dot");
  //       // TH.printGraphAsDot(myfile);
  //     }
  //     thpstmt->executeUpdate();

  //     // Fill type hierarchy - type relation
  //     for (auto type_identifier : TH.recognized_struct_types) {
  //       unique_ptr<sql::PreparedStatement> ttpstmt(
  //           conn->prepareStatement("INSERT INTO type_hierarchy_has_type "
  //                                  "(type_hierarchy_id,type_id)
  //                                  VALUES(?,?)"));
  //       int typeID = getTypeID(type_identifier);
  //       ttpstmt->setInt(1, THID);
  //       ttpstmt->setInt(2, typeID);
  //       ttpstmt->executeUpdate();
  //     }

  //     // Fill module - type hierarchy relation
  //     for (auto M : TH.contained_modules) {
  //       unique_ptr<sql::PreparedStatement> mtpstmt(conn->prepareStatement(
  //           "INSERT INTO module_has_type_hierarchy "
  //           "(module_id,type_hierarchy_id) VALUES(?,?)"));
  //       int moduleID = getModuleID(M->getModuleIdentifier());
  //       mtpstmt->setInt(1, moduleID);
  //       mtpstmt->setInt(2, THID);
  //       mtpstmt->executeUpdate();
  //     }

  //     // Store VTables
  //     for (auto entry : TH.vtable_map) {
  //       insertVTable(entry.second, entry.first, ProjectName);
  //     }
  //   }
  //   // Perform the commit
  //   conn->commit();
  // } catch (sql::SQLException &e) {
  //   SQL_STD_ERROR_HANDLING;
  // }
}

LLVMTypeHierarchy
DBConn::loadLLVMTypeHierarchyFromModule(const string &ModuleName, bool use_hs) {
  try {
    int THID = -1;
    unique_ptr<sql::PreparedStatement> pstmt(conn->prepareStatement(
        "SELECT type_hierarchy_id FROM module_has_type_hierarchy "
        "NATURAL JOIN module WHERE module.identifier=?"));
    pstmt->setString(1, ModuleName);
    unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
    while (res->next()) {
      if (THID == -1)
        THID = res->getInt("type_hierarchy_id");
      else
        throw logic_error(
            "Given module is contained in several type hierarchies!");
    }
    if (THID == -1)
      throw logic_error(
          "No Type Hierarchy found that contains the given module!");
  } catch (sql::SQLException &e) {
    SQL_STD_ERROR_HANDLING;
  } catch (logic_error &e) {
    cout << e.what() << endl;
  }
  return LLVMTypeHierarchy();
}

LLVMTypeHierarchy
DBConn::loadLLVMTypeHierarchyFromModules(initializer_list<string> ModuleNames,
                                         bool use_hs) {
  try {
    set<int> moduleIDs;
    for (auto modName : ModuleNames) {
      moduleIDs.insert(getModuleID(modName));
    }
    int THID = -1;
    for (auto typeHieraryID : getAllTypeHierarchyIDs()) {
      set<int> moduleIDsFromTH(getAllModuleIDsFromTH(typeHieraryID));
      if (moduleIDs == moduleIDsFromTH) {
        THID = typeHieraryID;
        break;
      }
    }
    if (THID == -1)
      throw logic_error(
          "No Type Hierarchy found that contains all given modules!");

  } catch (sql::SQLException &e) {
    SQL_STD_ERROR_HANDLING;
  } catch (logic_error &e) {
    cout << e.what() << endl;
  }
  return LLVMTypeHierarchy();
}

LLVMTypeHierarchy
DBConn::loadLLVMTypeHierarchyFromProject(const string &ProjectName,
                                         bool use_hs) {
  try {
  } catch (sql::SQLException &e) {
    SQL_STD_ERROR_HANDLING;
  }
  return LLVMTypeHierarchy();
}

void DBConn::storeIDESummary(const IDESummary &S) {
  try {
  } catch (sql::SQLException &e) {
    SQL_STD_ERROR_HANDLING;
  }
}

IDESummary DBConn::loadIDESummary(const string &FunctionName,
                                  const string &AnalysisName) {
  try {
  } catch (sql::SQLException &e) {
    SQL_STD_ERROR_HANDLING;
  }
  return IDESummary();
}

void DBConn::storeProjectIRDB(const string &ProjectName,
                              const ProjectIRDB &IRDB) {
  // Initialize the transaction
  unique_ptr<sql::Statement> stmt(conn->createStatement());
  stmt->execute("START TRANSACTION");
  for (auto M : IRDB.getAllModules()) {
    int moduleID = getModuleID(M->getModuleIdentifier());
    if (moduleID == -1 ||
        getModuleHash(moduleID) != computeModuleHash(M, true)) {
      insertModule(ProjectName, M);
    }
  }
  // Perform the commit
  conn->commit();
}

ProjectIRDB DBConn::loadProjectIRDB(const string &ProjectName) {
  unique_ptr<sql::PreparedStatement> pstmt(conn->prepareStatement(
      "SELECT identifier "
      "FROM project_has_module NATURAL JOIN module WHERE project_id=(SELECT "
      "project_id FROM project WHERE identifier=?)"));
  pstmt->setString(1, ProjectName);
  unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
  ProjectIRDB IRDB(IRDBOptions::NONE);
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
  auto lg = lg::get();
  LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG) << "Building database schema");

  unique_ptr<sql::Statement> stmt(conn->createStatement());
  static const string old_unique_checks(
      "SET @OLD_UNIQUE_CHECKS=@@UNIQUE_CHECKS, UNIQUE_CHECKS=0; ");
  stmt->execute(old_unique_checks);

  static const string old_foreign_key_checks(
      "SET @OLD_FOREIGN_KEY_CHECKS=@@FOREIGN_KEY_CHECKS, "
      "FOREIGN_KEY_CHECKS=0; ");
  stmt->execute(old_foreign_key_checks);

  static const string old_sql_mode(
      "SET @OLD_SQL_MODE=@@SQL_MODE, "
      "SQL_MODE='TRADITIONAL,ALLOW_INVALID_DATES';");
  stmt->execute(old_sql_mode);

  static const string create_schema(
      "CREATE SCHEMA IF NOT EXISTS `phasardb` DEFAULT CHARACTER SET utf8 ; ");
  stmt->execute(create_schema);

  static const string create_function(
      "CREATE TABLE IF NOT EXISTS `phasardb`.`function` ( "
      "`function_id` INT(11) NOT NULL AUTO_INCREMENT, "
      "`identifier` VARCHAR(512) NULL DEFAULT NULL, "
      "`declaration` TINYINT(1) NULL DEFAULT NULL, "
      "`hash` VARCHAR(512) NULL DEFAULT NULL, "
      "PRIMARY KEY (`function_id`)) "
      "ENGINE = InnoDB "
      "DEFAULT CHARACTER SET = utf8 "
      "COLLATE = utf8_unicode_ci; ");
  stmt->execute(create_function);

  static const string create_ifds_ide_summary(
      "CREATE TABLE IF NOT EXISTS `phasardb`.`ifds_ide_summary` ( "
      "`ifds_ide_summary_id` INT(11) NOT NULL AUTO_INCREMENT, "
      "`analysis` VARCHAR(512) NULL DEFAULT NULL, "
      "`representation` BLOB NULL DEFAULT NULL, "
      "PRIMARY KEY (`ifds_ide_summary_id`)) "
      "ENGINE = InnoDB "
      "DEFAULT CHARACTER SET = utf8 "
      "COLLATE = utf8_unicode_ci; ");
  stmt->execute(create_ifds_ide_summary);

  static const string create_module(
      "CREATE TABLE IF NOT EXISTS `phasardb`.`module` ( "
      "`module_id` INT(11) NOT NULL AUTO_INCREMENT, "
      "`identifier` VARCHAR(512) NULL DEFAULT NULL, "
      "`hash` VARCHAR(512) NULL DEFAULT NULL, "
      "`code` LONGBLOB NULL DEFAULT NULL, "
      "PRIMARY KEY (`module_id`)) "
      "ENGINE = InnoDB "
      "DEFAULT CHARACTER SET = utf8 "
      "COLLATE = utf8_unicode_ci; ");
  stmt->execute(create_module);

  static const string create_type(
      "CREATE TABLE IF NOT EXISTS `phasardb`.`type` ( "
      "`type_id` INT(11) NOT NULL AUTO_INCREMENT, "
      "`identifier` VARCHAR(512) NULL DEFAULT NULL, "
      "PRIMARY KEY (`type_id`)) "
      "ENGINE = InnoDB "
      "DEFAULT CHARACTER SET = utf8 "
      "COLLATE = utf8_unicode_ci; ");
  stmt->execute(create_type);

  static const string create_type_hierarchy(
      "CREATE TABLE IF NOT EXISTS `phasardb`.`type_hierarchy` ( "
      "`type_hierarchy_id` INT(11) NOT NULL AUTO_INCREMENT, "
      "`representation` LONGBLOB NULL DEFAULT NULL, "
      "`representation_ref` VARCHAR(512) NULL DEFAULT NULL, "
      "PRIMARY KEY (`type_hierarchy_id`)) "
      "ENGINE = InnoDB "
      "DEFAULT CHARACTER SET = utf8 "
      "COLLATE = utf8_unicode_ci; ");
  stmt->execute(create_type_hierarchy);

  static const string create_global_variable(
      "CREATE TABLE IF NOT EXISTS `phasardb`.`global_variable` ( "
      "`global_variable_id` INT(11) NOT NULL AUTO_INCREMENT, "
      "`identifier` VARCHAR(512) NULL DEFAULT NULL, "
      "`declaration` TINYINT(1) NULL DEFAULT NULL, "
      "PRIMARY KEY (`global_variable_id`)) "
      "ENGINE = InnoDB "
      "DEFAULT CHARACTER SET = utf8 "
      "COLLATE = utf8_unicode_ci; ");
  stmt->execute(create_global_variable);

  static const string create_callgraph(
      "CREATE TABLE IF NOT EXISTS `phasardb`.`callgraph` ( "
      "`callgraph_id` INT(11) NOT NULL AUTO_INCREMENT, "
      "`representation` LONGBLOB NULL DEFAULT NULL, "
      "`representation_ref` VARCHAR(512) NULL DEFAULT NULL, "
      "PRIMARY KEY (`callgraph_id`)) "
      "ENGINE = InnoDB "
      "DEFAULT CHARACTER SET = utf8 "
      "COLLATE = utf8_unicode_ci; ");
  stmt->execute(create_callgraph);

  static const string create_points_to_graph(
      "CREATE TABLE IF NOT EXISTS `phasardb`.`points-to_graph` ( "
      "`points-to_graph_id` INT(11) NOT NULL AUTO_INCREMENT, "
      "`representation` LONGBLOB NULL DEFAULT NULL, "
      "`representation_ref` VARCHAR(512) NULL DEFAULT NULL, "
      "PRIMARY KEY (`points-to_graph_id`)) "
      "ENGINE = InnoDB "
      "DEFAULT CHARACTER SET = utf8 "
      "COLLATE = utf8_unicode_ci; ");
  stmt->execute(create_points_to_graph);

  static const string create_project(
      "CREATE TABLE IF NOT EXISTS `phasardb`.`project` ( "
      "`project_id` INT(11) NOT NULL AUTO_INCREMENT, "
      "`identifier` VARCHAR(512) NULL DEFAULT NULL, "
      "PRIMARY KEY (`project_id`)) "
      "ENGINE = InnoDB "
      "DEFAULT CHARACTER SET = utf8 "
      "COLLATE = utf8_unicode_ci; ");
  stmt->execute(create_project);

  static const string create_project_has_module(
      "CREATE TABLE IF NOT EXISTS `phasardb`.`project_has_module` ( "
      "`project_id` INT(11) NOT NULL, "
      "`module_id` INT(11) NOT NULL, "
      "PRIMARY KEY (`project_id`, `module_id`), "
      "INDEX `fk_project_has_module_module1_idx` (`module_id` ASC), "
      "INDEX `fk_project_has_module_project1_idx` (`project_id` ASC), "
      "CONSTRAINT `fk_project_has_module_project1` "
      "FOREIGN KEY (`project_id`) "
      "REFERENCES `phasardb`.`project` (`project_id`) "
      "ON DELETE CASCADE "
      "ON UPDATE CASCADE, "
      "CONSTRAINT `fk_project_has_module_module1` "
      "FOREIGN KEY (`module_id`) "
      "REFERENCES `phasardb`.`module` (`module_id`) "
      "ON DELETE CASCADE "
      "ON UPDATE CASCADE) "
      "ENGINE = InnoDB "
      "DEFAULT CHARACTER SET = utf8 "
      "COLLATE = utf8_unicode_ci; ");
  stmt->execute(create_project_has_module);

  static const string create_module_has_callgraph(
      "CREATE TABLE IF NOT EXISTS `phasardb`.`module_has_callgraph` ( "
      "`module_id` INT(11) NOT NULL, "
      "`callgraph_id` INT(11) NOT NULL, "
      "PRIMARY KEY (`module_id`, `callgraph_id`), "
      "INDEX `fk_module_has_callgraph_callgraph1_idx` (`callgraph_id` ASC), "
      "INDEX `fk_module_has_callgraph_module1_idx` (`module_id` ASC), "
      "CONSTRAINT `fk_module_has_callgraph_module1` "
      "FOREIGN KEY (`module_id`) "
      "REFERENCES `phasardb`.`module` (`module_id`) "
      "ON DELETE CASCADE "
      "ON UPDATE CASCADE, "
      "CONSTRAINT `fk_module_has_callgraph_callgraph1` "
      "FOREIGN KEY (`callgraph_id`) "
      "REFERENCES `phasardb`.`callgraph` (`callgraph_id`) "
      "ON DELETE CASCADE "
      "ON UPDATE CASCADE) "
      "ENGINE = InnoDB "
      "DEFAULT CHARACTER SET = utf8 "
      "COLLATE = utf8_unicode_ci; ");
  stmt->execute(create_module_has_callgraph);

  static const string create_callgraph_has_points_to_graph(
      "CREATE TABLE IF NOT EXISTS `phasardb`.`callgraph_has_points-to_graph` ( "
      "`callgraph_id` INT(11) NOT NULL, "
      "`points-to_graph_id` INT(11) NOT NULL, "
      "PRIMARY KEY (`callgraph_id`, `points-to_graph_id`), "
      "INDEX `fk_callgraph_has_points-to_graph_points-to_graph1_idx` "
      "(`points-to_graph_id` ASC), "
      "INDEX `fk_callgraph_has_points-to_graph_callgraph1_idx` (`callgraph_id` "
      "ASC), "
      "CONSTRAINT `fk_callgraph_has_points-to_graph_callgraph1` "
      "FOREIGN KEY (`callgraph_id`) "
      "REFERENCES `phasardb`.`callgraph` (`callgraph_id`) "
      "ON DELETE CASCADE "
      "ON UPDATE CASCADE, "
      "CONSTRAINT `fk_callgraph_has_points-to_graph_points-to_graph1` "
      "FOREIGN KEY (`points-to_graph_id`) "
      "REFERENCES `phasardb`.`points-to_graph` (`points-to_graph_id`) "
      "ON DELETE CASCADE "
      "ON UPDATE CASCADE) "
      "ENGINE = InnoDB "
      "DEFAULT CHARACTER SET = utf8 "
      "COLLATE = utf8_unicode_ci; ");
  stmt->execute(create_callgraph_has_points_to_graph);

  static const string create_module_has_function(
      "CREATE TABLE IF NOT EXISTS `phasardb`.`module_has_function` ( "
      "`module_id` INT(11) NOT NULL, "
      "`function_id` INT(11) NOT NULL, "
      "PRIMARY KEY (`module_id`, `function_id`), "
      "INDEX `fk_module_has_function_function1_idx` (`function_id` ASC), "
      "INDEX `fk_module_has_function_module1_idx` (`module_id` ASC), "
      "CONSTRAINT `fk_module_has_function_module1` "
      "FOREIGN KEY (`module_id`) "
      "REFERENCES `phasardb`.`module` (`module_id`) "
      "ON DELETE NO ACTION "
      "ON UPDATE NO ACTION, "
      "CONSTRAINT `fk_module_has_function_function1` "
      "FOREIGN KEY (`function_id`) "
      "REFERENCES `phasardb`.`function` (`function_id`) "
      "ON DELETE NO ACTION "
      "ON UPDATE NO ACTION) "
      "ENGINE = InnoDB "
      "DEFAULT CHARACTER SET = utf8 "
      "COLLATE = utf8_unicode_ci; ");
  stmt->execute(create_module_has_function);

  static const string create_module_has_global_variable(
      "CREATE TABLE IF NOT EXISTS `phasardb`.`module_has_global_variable` ( "
      "`module_id` INT(11) NOT NULL, "
      "`global_variable_id` INT(11) NOT NULL, "
      "PRIMARY KEY (`module_id`, `global_variable_id`), "
      "INDEX `fk_module_has_global_variable_global_variable1_idx` "
      "(`global_variable_id` ASC), "
      "INDEX `fk_module_has_global_variable_module1_idx` (`module_id` ASC), "
      "CONSTRAINT `fk_module_has_global_variable_module1` "
      "FOREIGN KEY (`module_id`) "
      "REFERENCES `phasardb`.`module` (`module_id`) "
      "ON DELETE CASCADE "
      "ON UPDATE CASCADE, "
      "CONSTRAINT `fk_module_has_global_variable_global_variable1` "
      "FOREIGN KEY (`global_variable_id`) "
      "REFERENCES `phasardb`.`global_variable` (`global_variable_id`) "
      "ON DELETE CASCADE "
      "ON UPDATE CASCADE) "
      "ENGINE = InnoDB "
      "DEFAULT CHARACTER SET = utf8 "
      "COLLATE = utf8_unicode_ci; ");
  stmt->execute(create_module_has_global_variable);

  static const string create_module_has_type_hierarchy(
      "CREATE TABLE IF NOT EXISTS `phasardb`.`module_has_type_hierarchy` ( "
      "`module_id` INT(11) NOT NULL, "
      "`type_hierarchy_id` INT(11) NOT NULL, "
      "PRIMARY KEY (`module_id`, `type_hierarchy_id`), "
      "INDEX `fk_module_has_type_hierarchy_type_hierarchy1_idx` "
      "(`type_hierarchy_id` ASC), "
      "INDEX `fk_module_has_type_hierarchy_module1_idx` (`module_id` ASC), "
      "CONSTRAINT `fk_module_has_type_hierarchy_module1` "
      "FOREIGN KEY (`module_id`) "
      "REFERENCES `phasardb`.`module` (`module_id`) "
      "ON DELETE CASCADE "
      "ON UPDATE CASCADE, "
      "CONSTRAINT `fk_module_has_type_hierarchy_type_hierarchy1` "
      "FOREIGN KEY (`type_hierarchy_id`) "
      "REFERENCES `phasardb`.`type_hierarchy` (`type_hierarchy_id`) "
      "ON DELETE CASCADE "
      "ON UPDATE CASCADE) "
      "ENGINE = InnoDB "
      "DEFAULT CHARACTER SET = utf8 "
      "COLLATE = utf8_unicode_ci; ");
  stmt->execute(create_module_has_type_hierarchy);

  static const string create_module_has_type(
      "CREATE TABLE IF NOT EXISTS `phasardb`.`module_has_type` ( "
      "`module_id` INT(11) NOT NULL, "
      "`type_id` INT(11) NOT NULL, "
      "PRIMARY KEY (`module_id`, `type_id`), "
      "INDEX `fk_module_has_type_type1_idx` (`type_id` ASC), "
      "INDEX `fk_module_has_type_module1_idx` (`module_id` ASC), "
      "CONSTRAINT `fk_module_has_type_module1` "
      "FOREIGN KEY (`module_id`) "
      "REFERENCES `phasardb`.`module` (`module_id`) "
      "ON DELETE CASCADE "
      "ON UPDATE CASCADE, "
      "CONSTRAINT `fk_module_has_type_type1` "
      "FOREIGN KEY (`type_id`) "
      "REFERENCES `phasardb`.`type` (`type_id`) "
      "ON DELETE CASCADE "
      "ON UPDATE CASCADE) "
      "ENGINE = InnoDB "
      "DEFAULT CHARACTER SET = utf8 "
      "COLLATE = utf8_unicode_ci; ");
  stmt->execute(create_module_has_type);

  static const string create_type_hierarchy_has_type(
      "CREATE TABLE IF NOT EXISTS `phasardb`.`type_hierarchy_has_type` ( "
      "`type_hierarchy_id` INT(11) NOT NULL, "
      "`type_id` INT(11) NOT NULL, "
      "PRIMARY KEY (`type_hierarchy_id`, `type_id`), "
      "INDEX `fk_type_hierarchy_has_type_type1_idx` (`type_id` ASC), "
      "INDEX `fk_type_hierarchy_has_type_type_hierarchy1_idx` "
      "(`type_hierarchy_id` ASC), "
      "CONSTRAINT `fk_type_hierarchy_has_type_type_hierarchy1` "
      "FOREIGN KEY (`type_hierarchy_id`) "
      "REFERENCES `phasardb`.`type_hierarchy` (`type_hierarchy_id`) "
      "ON DELETE CASCADE "
      "ON UPDATE CASCADE, "
      "CONSTRAINT `fk_type_hierarchy_has_type_type1` "
      "FOREIGN KEY (`type_id`) "
      "REFERENCES `phasardb`.`type` (`type_id`) "
      "ON DELETE CASCADE "
      "ON UPDATE CASCADE) "
      "ENGINE = InnoDB "
      "DEFAULT CHARACTER SET = utf8 "
      "COLLATE = utf8_unicode_ci; ");
  stmt->execute(create_type_hierarchy_has_type);

  static const string create_function_has_ifds_ide_summary(
      "CREATE TABLE IF NOT EXISTS `phasardb`.`function_has_ifds_ide_summary` ( "
      "`function_id` INT(11) NOT NULL, "
      "`ifds_ide_summary_id` INT(11) NOT NULL, "
      "PRIMARY KEY (`function_id`, `ifds_ide_summary_id`), "
      "INDEX `fk_function_has_ifds_ide_summary_ifds_ide_summary1_idx` "
      "(`ifds_ide_summary_id` ASC), "
      "INDEX `fk_function_has_ifds_ide_summary_function1_idx` (`function_id` "
      "ASC), "
      "CONSTRAINT `fk_function_has_ifds_ide_summary_function1` "
      "FOREIGN KEY (`function_id`) "
      "REFERENCES `phasardb`.`function` (`function_id`) "
      "ON DELETE CASCADE "
      "ON UPDATE CASCADE, "
      "CONSTRAINT `fk_function_has_ifds_ide_summary_ifds_ide_summary1` "
      "FOREIGN KEY (`ifds_ide_summary_id`) "
      "REFERENCES `phasardb`.`ifds_ide_summary` (`ifds_ide_summary_id`) "
      "ON DELETE CASCADE "
      "ON UPDATE CASCADE) "
      "ENGINE = InnoDB "
      "DEFAULT CHARACTER SET = utf8 "
      "COLLATE = utf8_unicode_ci; ");
  stmt->execute(create_function_has_ifds_ide_summary);

  static const string create_function_has_points_to_graph(
      "CREATE TABLE IF NOT EXISTS `phasardb`.`function_has_points-to_graph` ( "
      "`function_id` INT(11) NOT NULL, "
      "`points-to_graph_id` INT(11) NOT NULL, "
      "PRIMARY KEY (`function_id`, `points-to_graph_id`), "
      "INDEX `fk_function_has_points-to_graph_points-to_graph1_idx` "
      "(`points-to_graph_id` ASC), "
      "INDEX `fk_function_has_points-to_graph_function1_idx` (`function_id` "
      "ASC), "
      "CONSTRAINT `fk_function_has_points-to_graph_function1` "
      "FOREIGN KEY (`function_id`) "
      "REFERENCES `phasardb`.`function` (`function_id`) "
      "ON DELETE CASCADE "
      "ON UPDATE CASCADE, "
      "CONSTRAINT `fk_function_has_points-to_graph_points-to_graph1` "
      "FOREIGN KEY (`points-to_graph_id`) "
      "REFERENCES `phasardb`.`points-to_graph` (`points-to_graph_id`) "
      "ON DELETE CASCADE "
      "ON UPDATE CASCADE) "
      "ENGINE = InnoDB "
      "DEFAULT CHARACTER SET = utf8 "
      "COLLATE = utf8_unicode_ci; ");
  stmt->execute(create_function_has_points_to_graph);

  static const string create_type_has_virtual_function(
      "CREATE TABLE IF NOT EXISTS `phasardb`.`type_has_virtual_function` ( "
      "`type_id` INT(11) NOT NULL, "
      "`function_id` INT(11) NOT NULL, "
      "`vtable_index` INT(11) NULL DEFAULT NULL, "
      "PRIMARY KEY (`type_id`, `function_id`), "
      "INDEX `fk_type_has_function_function1_idx` (`function_id` ASC), "
      "INDEX `fk_type_has_function_type1_idx` (`type_id` ASC), "
      "CONSTRAINT `fk_type_has_function_type1` "
      "FOREIGN KEY (`type_id`) "
      "REFERENCES `phasardb`.`type` (`type_id`) "
      "ON DELETE CASCADE "
      "ON UPDATE CASCADE, "
      "CONSTRAINT `fk_type_has_function_function1` "
      "FOREIGN KEY (`function_id`) "
      "REFERENCES `phasardb`.`function` (`function_id`) "
      "ON DELETE CASCADE "
      "ON UPDATE CASCADE) "
      "ENGINE = InnoDB "
      "DEFAULT CHARACTER SET = utf8 "
      "COLLATE = utf8_unicode_ci; ");
  stmt->execute(create_type_has_virtual_function);

  static const string sql_mode("SET SQL_MODE=@OLD_SQL_MODE; ");
  stmt->execute(sql_mode);

  static const string foreign_key_checks(
      "SET FOREIGN_KEY_CHECKS=@OLD_FOREIGN_KEY_CHECKS; ");
  stmt->execute(foreign_key_checks);

  static const string unique_checks("SET UNIQUE_CHECKS=@OLD_UNIQUE_CHECKS; ");
  stmt->execute(unique_checks);
  LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG) << "Database schema done");
}

void DBConn::dropDBAndRebuildScheme() {
  unique_ptr<sql::Statement> stmt(conn->createStatement());
  const string drop_database("DROP DATABASE IF EXISTS `phasardb`");
  stmt->execute(drop_database);
  cout << "DROPED DATABASE SCHEMA" << endl;
  this_thread::sleep_for(5s);
  buildDBScheme();
}

} // namespace psr
*/
