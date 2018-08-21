#!/usr/bin/env python3

# author: Richard Leer
#
# This little script reads the .sql file that contains all SQL Statements that are needed
# to create the database schema and all tables, and generates the appropriate C++ statements
# that are necessary to generate the database schema and all tables within the phasar framework.

import sys, getopt, re

# The generated file has the following format:
#   HEADER + <body> + FOOTER
HEADER = "auto lg = lg::get();\nLOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG) << \"Building database schema\");\n\nunique_ptr<sql::Statement> stmt(conn->createStatement());"
FOOTER = "LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG) << \"Database schema done\");"

# A query has the following format:
#   QUERY_HEAD_START + <query_name> + QUERY_HEAD_END +
#   QUERY_BODY_START + <query_first_line> + QUERY_BODY_END +
#   QUERY_BODY_START + <query_second_line> + QUERY_BODY_END +
#   ...
#   QUERY_FOOT_START + <query_name> + QUERY_FOOT_END
QUERY_HEAD_START = "\nstatic const string "
QUERY_HEAD_END = "("
QUERY_BODY_START = "\n    \""
QUERY_BODY_END = " \""
QUERY_FOOT_START = ");\nstmt->execute("
QUERY_FOOT_END = ");\n"
# Every query ends on a ';'. Every query is followed by an empty line, except the 'SET' queries.

# The following keywords initiate a new query:
KEYWORDS = ['SET', 'CREATE TABLE', 'CREATE SCHEMA']


def main(argv):
  path_to_input_file = "createPhasarScheme.sql"
  path_to_output_file = "createPhasarScheme.cpp"
  try:
    opts, args = getopt.getopt(argv, "hi:o:", ["ifile=", "ofile="])
  except getopt.GetoptError:
    print("sqltocpp -i path/to/sqlfile -o path/to/outputfile")
    sys.exit(2)
  for opt, arg in opts:
    if opt == '-h':
      print("sqltocpp.py -i path/to/sqlfile -o path/to/outputfile")
      sys.exit()
    elif opt in ("-i", "--ifile"):
      path_to_input_file = arg
    elif opt in ("-o", "--ofile"):
      path_to_output_file = arg

  output_file = open(path_to_output_file,"w")
  input_file = open(path_to_input_file, "r")

  # write the header
  output_file.write(HEADER)

  query_name, query_header, query_body = "","",""
  for line in input_file:
    # skip sql comments and empty lines
    if not line.startswith("--") and not line == '\n':
      query_body += QUERY_BODY_START + line.strip() + QUERY_BODY_END
      if line.startswith(tuple(KEYWORDS)):
        # figure out the query name - must be unique
        try:
          if line.startswith(KEYWORDS[0]):
            # SET query name
            query_name = re.search(r'\s@?(?P<name>.+?)=@', line).group('name').lower()
            query_name = query_name.replace('-','_')
          elif line.startswith(KEYWORDS[1]):
            # CREATE TABLE query name
            query_name = "create_"
            query_name += re.search(r'\s\`.+?\`[.]\`(?P<name>.+?)\`', line).group('name')
            query_name = query_name.replace('-','_')
          elif line.startswith(KEYWORDS[2]):
            # CREATE SCHEMA query name
            query_name = "create_schema"
        except AttributeError:
          print("Couldn't find appropriate query name for line:\n",line)
          sys.exit(2)
        # build new query
        query_header = QUERY_HEAD_START + query_name + QUERY_HEAD_END
      if (line.endswith(";\n")):
        # add footer to query
        query_footer = QUERY_FOOT_START + query_name + QUERY_FOOT_END
        # write query to file
        output_file.write(query_header + query_body + query_footer)
        query_body = ""

  output_file.write(FOOTER)
  input_file.close()
  output_file.close()

if __name__ == "__main__":
  main(sys.argv[1:])
