/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Christian Stritzke and others
 *****************************************************************************/

#include "phasar/DB/Queries.h"

using namespace std;
using namespace psr;

namespace psr {

const string SPOInsert =
    "insert or ignore into spo_subject (name) "
    "values (\"%1%\");"

    "insert or ignore into spo_predicate (name, sid) "
    "values (\"%2%\", (select id from spo_subject where name=\"%1%\"));"

    "insert or ignore into spo_object (name, sid, pid) "
    "values (\"%3%\", (select id from spo_subject where name=\"%1%\"), "
    "(select id from spo_predicate where name=\"%2%\" and sid=(select id from "
    "spo_subject where name=\"%1%\")));";

const string SOPInsert =
    "insert or ignore into sop_subject (name) "
    "values (\"%1%\");"

    "insert or ignore into sop_object (name, sid) "
    "values (\"%3%\", (select id from sop_subject where name=\"%1%\"));"

    "insert or ignore into sop_predicate (name, sid, oid) "
    "values (\"%2%\", (select id from sop_subject where name=\"%1%\"), "
    "(select id from sop_object where name=\"%3%\" and sid=(select id from "
    "sop_subject where name=\"%1%\")));";

const string PSOInsert =
    "insert or ignore into pso_predicate (name) "
    "values (\"%2%\");"

    "insert or ignore into pso_subject (name, pid) "
    "values (\"%1%\", (select id from pso_predicate where name=\"%2%\"));"

    "insert or ignore into pso_object (name, pid, sid) "
    "values (\"%3%\", (select id from pso_predicate where name=\"%2%\"), "
    "(select id from pso_subject where name=\"%1%\" and pid=(select id from "
    "pso_predicate where name=\"%2%\")));";

const string POSInsert =
    "insert or ignore into pos_predicate (name) "
    "values (\"%2%\");"

    "insert or ignore into pos_object (name, pid) "
    "values (\"%3%\", (select id from pos_predicate where name=\"%2%\"));"

    "insert or ignore into pos_subject (name, oid, pid) "
    "values (\"%1%\", (select id from pos_object where pos_object.name=\"%3%\" "
    "and "
    "pos_object.pid=(select id from pos_predicate where name=\"%2%\")), "
    "(select pid from pos_object where name=\"%3%\" and pid=(select id from "
    "pos_predicate where name=\"%2%\")));";

const string OSPInsert =
    "insert or ignore into osp_object (name) "
    "values (\"%3%\");"

    "insert or ignore into osp_subject (name, oid) "
    "values (\"%1%\", (select id from osp_object where name=\"%3%\"));"

    "insert or ignore into osp_predicate (name, sid, oid) "
    "values (\"%2%\", (select id from osp_subject where name=\"%1%\" and "
    "oid=(select id from osp_object where name=\"%3%\")), "
    "(select id from osp_object where name=\"%3%\" and oid=(select id from "
    "osp_object where name=\"%3%\")));";

const string OPSInsert =
    "insert or ignore into ops_object (name) "
    "values (\"%3%\");"

    "insert or ignore into ops_predicate (name, oid) "
    "values (\"%2%\", (select id from ops_object where name=\"%3%\"));"

    "insert or ignore into ops_subject (name, pid, oid) "
    "values (\"%1%\", (select id from ops_predicate where name=\"%2%\"), "
    "(select id from pos_object where name=\"%3%\" and oid=(select id from "
    "osp_object where name=\"%3%\")));";

const string SearchSPO =
    "select spo_subject.name, spo_predicate.name, spo_object.name from "
    "spo_subject inner join spo_predicate on spo_subject.id=spo_predicate.sid "
    "inner join spo_object on spo_predicate.id=spo_object.pid and "
    "spo_subject.id=spo_object.sid "
    "where spo_subject.name=\"%1%\" and spo_predicate.name=\"%2%\" and "
    "spo_object.name=\"%3%\";";

const string SearchSPX =
    "-- %3%\n"
    "select spo_subject.name, spo_predicate.name, spo_object.name from "
    "spo_subject inner join spo_predicate on spo_subject.id=spo_predicate.sid "
    "inner join spo_object on spo_predicate.id=spo_object.pid and "
    "spo_subject.id=spo_object.sid "
    "where spo_subject.name=\"%1%\" and spo_predicate.name=\"%2%\";";

const string SearchSXO =
    "-- %2%\n"
    "select sop_subject.name, sop_predicate.name, sop_object.name from "
    "sop_subject "
    "inner join sop_object on sop_subject.id=sop_object.sid "
    "inner join sop_predicate on sop_object.id=sop_predicate.id and "
    "sop_subject.id=sop_predicate.sid "
    "where sop_subject.name=\"%1%\" and sop_object.name=\"%3%\";";

const string SearchXPO =
    "-- %1%\n"
    "select pos_subject.name, pos_predicate.name, pos_object.name from "
    "pos_predicate "
    "inner join pos_object on pos_object.pid=pos_predicate.id "
    "inner join pos_subject on pos_subject.pid=pos_predicate.id and "
    "pos_subject.oid=pos_object.id "
    "where pos_predicate.name=\"%2%\" and pos_object.name=\"%3%\";";

const string SearchSXX =
    "-- %2%%3%\n"
    "select spo_subject.name, spo_predicate.name, spo_object.name from "
    "spo_subject inner join spo_predicate on spo_subject.id=spo_predicate.sid "
    "inner join spo_object on spo_predicate.id=spo_object.pid and "
    "spo_subject.id=spo_object.sid "
    "where spo_subject.name=\"%1%\";";

const string SearchXPX =
    "-- %1%%3%\n"
    "select pso_subject.name, pso_predicate.name, pso_object.name from "
    "pso_predicate inner join pso_subject on pso_predicate.id=pso_subject.pid "
    "inner join pso_object on pso_predicate.id=pso_object.pid and "
    "pso_subject.id=pso_object.sid "
    "where pso_predicate.name=\"%2%\";";

const string SearchXXO =
    "-- %1%%2%\n"
    "select osp_subject.name, osp_predicate.name, osp_object.name from "
    "osp_object inner join osp_subject on osp_object.id=osp_subject.oid "
    "inner join osp_predicate on osp_subject.id=osp_predicate.sid and "
    "osp_object.id=osp_predicate.oid "
    "where osp_object.name=\"%3%\";";

const string SearchXXX =
    "-- %1%%2%%3%\n"
    "select spo_subject.name, spo_predicate.name, spo_object.name from "
    "spo_subject inner join spo_predicate on spo_subject.id=spo_predicate.sid "
    "inner join spo_object on spo_predicate.id=spo_object.pid and "
    "spo_subject.id=spo_object.sid;";

const string INIT = R"(
-- SPO Tables
create table if not exists spo_subject (
    id integer not null primary key AUTOINCREMENT,
    name varchar unique not null
);
create table if not exists spo_predicate(
    id integer not null unique primary key AUTOINCREMENT,
    name varchar not null,
    sid integer not null,
    foreign key (sid) references spo_subject(id),
    unique(name, sid)
);
create table if not exists spo_object(
    id integer not null unique primary key AUTOINCREMENT,
    name varchar not null,
    pid integer not null,
    sid integer not null,
    foreign key (pid) references spo_predicate(id),
    foreign key (sid) references spo_subject(id),
    unique(name, pid, sid)
);



-- SOP Tables
create table if not exists sop_subject (
    id integer not null primary key AUTOINCREMENT,
    name varchar unique not null
);
create table if not exists sop_object(
    id integer not null unique primary key AUTOINCREMENT,
    name varchar not null,
    sid integer not null,
    foreign key (sid) references sop_subject(id),
    unique(name, sid)
);
create table if not exists sop_predicate(
    id integer not null unique primary key AUTOINCREMENT,
    name varchar not null,
    oid integer not null,
    sid integer not null,
    foreign key (oid) references sop_object(id),
    foreign key (sid) references sop_subject(id),
    unique(name, oid, sid)
);



-- PSO Tables
create table if not exists pso_predicate (
    id integer not null primary key AUTOINCREMENT,
    name varchar unique not null
);
create table if not exists pso_subject(
    id integer not null unique primary key AUTOINCREMENT,
    name varchar not null,
    pid integer not null,
    foreign key (pid) references pso_predicate(id),
    unique(name, pid)
);
create table if not exists pso_object(
    id integer not null unique primary key AUTOINCREMENT,
    name varchar not null,
    sid integer not null,
    pid integer not null,
    foreign key (sid) references pso_subject(id),
    foreign key (pid) references pso_predicate(id),
    unique(name, sid, pid)
);



-- POS Tables
create table if not exists pos_predicate (
    id integer not null primary key AUTOINCREMENT,
    name varchar unique not null
);
create table if not exists pos_object(
    id integer not null unique primary key AUTOINCREMENT,
    name varchar not null,
    pid integer not null,
    foreign key (pid) references pos_predicate(id),
    unique(name, pid)
);
create table if not exists pos_subject(
    id integer not null unique primary key AUTOINCREMENT,
    name varchar not null,
    oid integer not null,
    pid integer not null,
    foreign key (oid) references pos_object(id),
    foreign key (pid) references pos_predicate(id),
    unique(name, oid, pid)
);



-- OSP Tables
create table if not exists osp_object (
    id integer not null primary key AUTOINCREMENT,
    name varchar unique not null
);
create table if not exists osp_subject(
    id integer not null unique primary key AUTOINCREMENT,
    name varchar not null,
    oid integer not null,
    foreign key (oid) references osp_object(id),
    unique(name, oid)
);
create table if not exists osp_predicate(
    id integer not null unique primary key AUTOINCREMENT,
    name varchar not null,
    sid integer not null,
    oid integer not null,
    foreign key (sid) references osp_subject(id),
    foreign key (oid) references osp_object(id),
    unique(name, sid, oid)
);



-- OPS Tables
create table if not exists ops_object (
    id integer not null primary key AUTOINCREMENT,
    name varchar unique not null
);
create table if not exists ops_predicate(
    id integer not null unique primary key AUTOINCREMENT,
    name varchar not null,
    oid integer not null,
    foreign key (oid) references ops_object(id),
    unique(name, oid)
);
create table if not exists ops_subject(
    id integer not null unique primary key AUTOINCREMENT,
    name varchar not null,
    pid integer not null,
    oid integer not null,
    foreign key (pid) references ops_predicate(id),
    foreign key (oid) references ops_object(id),
    unique(name, pid, oid)
);

  )";

} // namespace psr
