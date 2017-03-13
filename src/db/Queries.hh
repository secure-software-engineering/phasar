#include <string>
using namespace std;

namespace hs_queries {

  const string spo_insert =
    "insert or ignore into spo_subject (name) "
    "values (\"%1%\");"

    "insert or ignore into spo_predicate (name, sid) "
    "values (\"%2%\", (select id from spo_subject where name=\"%1%\"));"

    "insert or ignore into spo_object (name, sid, pid) " 
    "values (\"%3%\", (select id from spo_subject where name=\"%1%\"), (select id from spo_predicate where name=\"%2%\"));";
  

  const string sop_insert =
    "insert or ignore into sop_subject (name) "
    "values (\"%1%\");"

    "insert or ignore into sop_object (name, sid) "
    "values (\"%3%\", (select id from sop_subject where name=\"%1%\"));"

    "insert or ignore into sop_predicate (name, sid, oid) " 
    "values (\"%2%\", (select id from sop_subject where name=\"%1%\"), (select id from sop_object where name=\"%3%\"));";
  

  const string pso_insert =
    "insert or ignore into pso_predicate (name) "
    "values (\"%2%\");"

    "insert or ignore into pso_subject (name, pid) "
    "values (\"%1%\", (select id from pso_predicate where name=\"%2%\"));"

    "insert or ignore into pso_object (name, pid, sid) " 
    "values (\"%3%\", (select id from pso_predicate where name=\"%2%\"), (select id from pso_subject where name=\"%1%\"));";


  const string pos_insert =
    "insert or ignore into pos_predicate (name) "
    "values (\"%2%\");"

    "insert or ignore into pos_object (name, pid) "
    "values (\"%3%\", (select id from pos_predicate where name=\"%2%\"));"

    "insert or ignore into pos_subject (name, oid, pid) " 
    "values (\"%1%\", (select id from pos_object where name=\"%3%\"), (select id from pos_predicate where name=\"%2%\"));";


  const string osp_insert =
    "insert or ignore into osp_object (name) "
    "values (\"%3%\");"

    "insert or ignore into osp_subject (name, oid) "
    "values (\"%1%\", (select id from osp_object where name=\"%3%\"));"

    "insert or ignore into osp_predicate (name, sid, oid) " 
    "values (\"%2%\", (select id from osp_subject where name=\"%1%\"), (select id from osp_object where name=\"%3%\"));";


  const string ops_insert =
    "insert or ignore into ops_object (name) "
    "values (\"%3%\");"

    "insert or ignore into ops_predicate (name, oid) "
    "values (\"%2%\", (select id from ops_object where name=\"%3%\"));"

    "insert or ignore into ops_subject (name, pid, oid) " 
    "values (\"%1%\", (select id from ops_predicate where name=\"%2%\"), (select id from pos_object where name=\"%3%\"));";
}
