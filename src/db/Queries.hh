#include <string>
using namespace std;

namespace hexastore {

  const string SPO_INSERT =
    "insert or ignore into spo_subject (name) "
    "values (\"%1%\");"

    "insert or ignore into spo_predicate (name, sid) "
    "values (\"%2%\", (select id from spo_subject where name=\"%1%\"));"

    "insert or ignore into spo_object (name, sid, pid) " 
    "values (\"%3%\", (select id from spo_subject where name=\"%1%\"), "
    "(select id from spo_predicate where name=\"%2%\" and sid=(select id from spo_subject where name=\"%1%\")));";
  

  const string SOP_INSERT =
    "insert or ignore into sop_subject (name) "
    "values (\"%1%\");"

    "insert or ignore into sop_object (name, sid) "
    "values (\"%3%\", (select id from sop_subject where name=\"%1%\"));"

    "insert or ignore into sop_predicate (name, sid, oid) " 
    "values (\"%2%\", (select id from sop_subject where name=\"%1%\"), "
    "(select id from sop_object where name=\"%3%\" and sid=(select id from sop_subject where name=\"%1%\")));";
  

  const string PSO_INSERT =
    "insert or ignore into pso_predicate (name) "
    "values (\"%2%\");"

    "insert or ignore into pso_subject (name, pid) "
    "values (\"%1%\", (select id from pso_predicate where name=\"%2%\"));"

    "insert or ignore into pso_object (name, pid, sid) " 
    "values (\"%3%\", (select id from pso_predicate where name=\"%2%\"), "
    "(select id from pso_subject where name=\"%1%\" and pid=(select id from pso_predicate where name=\"%2%\")));";


  const string POS_INSERT =
    "insert or ignore into pos_predicate (name) "
    "values (\"%2%\");"

    "insert or ignore into pos_object (name, pid) "
    "values (\"%3%\", (select id from pos_predicate where name=\"%2%\"));"

    "insert or ignore into pos_subject (name, oid, pid) " 
    "values (\"%1%\", (select id from pos_object where name=\"%3%\"), "
    "(select id from pos_object where name=\"%3%\" and pid=(select id from pos_predicate where name=\"%2%\")));";


  const string OSP_INSERT =
    "insert or ignore into osp_object (name) "
    "values (\"%3%\");"

    "insert or ignore into osp_subject (name, oid) "
    "values (\"%1%\", (select id from osp_object where name=\"%3%\"));"

    "insert or ignore into osp_predicate (name, sid, oid) " 
    "values (\"%2%\", (select id from osp_subject where name=\"%1%\"), "
    "(select id from osp_object where name=\"%3%\" and oid=(select id from osp_object where name=\"%3%\")));";


  const string OPS_INSERT =
    "insert or ignore into ops_object (name) "
    "values (\"%3%\");"

    "insert or ignore into ops_predicate (name, oid) "
    "values (\"%2%\", (select id from ops_object where name=\"%3%\"));"

    "insert or ignore into ops_subject (name, pid, oid) " 
    "values (\"%1%\", (select id from ops_predicate where name=\"%2%\"), "
    "(select id from pos_object where name=\"%3%\" and oid=(select id from osp_object where name=\"%3%\")));";

  const string SEARCH_SPO = 
    "select spo_subject.name, spo_predicate.name, spo_object.name from "
    "spo_subject inner join spo_predicate on spo_subject.id=spo_predicate.sid "
    "inner join spo_object on spo_predicate.id=spo_object.pid and spo_subject.id=spo_object.sid "
    "where spo_subject.name=\"%1%\" and spo_predicate.name=\"%2%\" and spo_object.name=\"%3%\";";

  const string SEARCH_SPX = 
    "-- %3%\n"
    "select spo_subject.name, spo_predicate.name, spo_object.name from "
    "spo_subject inner join spo_predicate on spo_subject.id=spo_predicate.sid "
    "inner join spo_object on spo_predicate.id=spo_object.pid and spo_subject.id=spo_object.sid "
    "where spo_subject.name=\"%1%\" and spo_predicate.name=\"%2%\";";

  const string SEARCH_SXO =
    "-- %2%\n"
    "select sop_subject.name, sop_predicate.name, sop_object.name from sop_subject "
    "inner join sop_object on sop_subject.id=sop_object.sid "
    "inner join sop_predicate on sop_object.id=sop_predicate.id and sop_subject.id=sop_predicate.sid "
    "where sop_subject.name=\"%1%\" and sop_object.name=\"%3%\";";

  const string SEARCH_XPO =
    "-- %2%\n"
    "select pos_subject.name, pos_predicate.name, pos_object.name from pos_predicate "
    "inner join pos_object on pos_subject.id=pos_object.sid "
    "inner join pos_predicate on pos_object.id=pos_predicate.id and pos_subject.id=pos_predicate.sid "
    "where pos_subject.name=\"%1%\" and pos_object.name=\"%3%\";";


  const string SEARCH_SXX = 
    "-- %2%%3%\n"
    "select spo_subject.name, spo_predicate.name, spo_object.name from "
    "spo_subject inner join spo_predicate on spo_subject.id=spo_predicate.sid "
    "inner join spo_object on spo_predicate.id=spo_object.pid and spo_subject.id=spo_object.sid "
    "where spo_subject.name=\"%1%\";";

  const string SEARCH_XPX = 
    "-- %1%%3%\n"
    "select pso_subject.name, pso_predicate.name, pso_object.name from "
    "pso_predicate inner join pso_subject on pso_predicate.id=pso_subject.pid "
    "inner join pso_object on pso_predicate.id=pso_object.pid and pso_subject.id=pso_object.sid "
    "where pso_predicate.name=\"%2%\";";

    
  const string SEARCH_XXO = 
    "-- %1%%2%\n"
    "select osp_subject.name, osp_predicate.name, osp_object.name from"
    "osp_object inner join osp_subject on osp_object.id=osp_subject.oid"
    "inner join osp_predicate on osp_subject.id=osp_predicate.sid and osp_object.id=osp_object.oid"
    "where osp_object.name=\"%3%\";";

  const string SEARCH_XXX = 
    "-- %1%%2%%3%\n"
    "select spo_subject.name, spo_predicate.name, spo_object.name from "
    "spo_subject inner join spo_predicate on spo_subject.id=spo_predicate.sid "
    "inner join spo_object on spo_predicate.id=spo_object.pid and spo_subject.id=spo_object.sid;";
}
