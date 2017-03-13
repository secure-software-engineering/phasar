 

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


