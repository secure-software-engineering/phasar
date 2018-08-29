#!/usr/bin/python3

import getopt
import sys
import platform
import os

# Console colors


class bcolors:
    if(platform.system() == "Linux"):
        HEADER = '\033[95m'
        OKBLUE = '\033[94m'
        OKGREEN = '\033[92m'
        WARNING = '\033[93m'
        FAIL = '\033[91m'
        ENDC = '\033[0m'
        BOLD = '\033[1m'
        UNDERLINE = '\033[4m'
    else:
        HEADER = ''
        OKBLUE = ''
        OKGREEN = ''
        WARNING = ''
        FAIL = ''
        ENDC = ''
        BOLD = ''
        UNDERLINE = ''


# filepath for baseclass search
script_dir = os.path.dirname(__file__)

# splits a string on element split and ignores every split element enclosed in startignore and stopignore


def getelem(x, split, startignore, stopignore, startignore2=None, stopignore2=None):
    y = []
    count = 0
    count2 = 0
    func = 0
    y.append("")
    for i in x:
        if(i == split and count == 0 and count2 == 0):
            func += 1
            y.append("")
        elif(i == startignore):
            count += 1
            y[func] += i
        elif(i == stopignore and count > 0):
            count -= 1
            y[func] += i
        elif(i == startignore2):
            count2 += 1
            y[func] += i
        elif(i == stopignore2 and count2 > 0):
            count2 -= 1
            y[func] += i
        else:
            y[func] += i
    return y

# splits a string at a comma but ignores every comma that is enclosed by square or pointy brackets


def disectstring(x):
    y = getelem(x, ",", "[", "]", "<", ">")
    for i in range(0, len(y)):
        y[i] = getelem(y[i], ":", "[", "]", "<", ">")
    return y


def generateparamlist(x):
    out = []
    y = x.replace("[", "")
    y = y.replace("]", "")
    for i in getelem(y, ",", "<", ">"):
        i = i.replace("::", "§")
        j = i.split(":")
        out.append(j[1].replace("§", "::") + " " + j[0])
    return ",".join(out)


def generatemodifier(x):
    out = []
    y = x.replace("[", "")
    y = y.replace("]", "")
    for i in getelem(y, ",", "<", ">"):
        out.append(i)
    return " ".join(out)


def generatetempl(x):
    out = []
    y = x.replace("[", "")
    y = y.replace("]", "")
    for i in getelem(y, ",", "<", ">"):
        out.append("typename " + i)
    return "template <" + ",".join(out) + ">"

# generates all additional functions


def genfunctions(typ, classname=None):
    global functions
    d = {}
    functions = functions.replace("::", "§")
    functions2 = disectstring(functions)
    if(typ == "header"):
        d["pbfunc"] = ""
        d["pvfunc"] = ""
        d["ptfunc"] = ""
    else:
        d["func"] = ""
        d["classname"] = classname
    print(functions2)
    for i in functions2:
        # delete leading whitespace to correct input errors
        if(type(i)==list):
            for x in i:
                x = x.lstrip()
        else:
            i = i.lstrip()
        d["param"] = ""
        d["pref"] = ""
        d["post"] = ""
        d["ftmpl"] = ""
        d["fname"] = i[1]
        d["rettype"] = i[2].replace("§", "::")
        if len(i) == 4:
            d["param"] = generateparamlist(i[3].replace("§", "::"))
        elif len(i) == 5:
            d["param"] = generateparamlist(i[3].replace("§", "::"))
            d["pref"] = generatemodifier(i[4]) + " "
        elif len(i) == 6:
            d["param"] = generateparamlist(i[3].replace("§", "::"))
            d["pref"] = generatemodifier(i[4]) + " "
            d["post"] = " " + generatemodifier(i[5])
        elif len(i) == 7:
            d["param"] = generateparamlist(i[3].replace("§", "::"))
            d["pref"] = generatemodifier(i[4]) + " "
            d["post"] = " " + generatemodifier(i[5])
            d["ftmpl"] = generatetempl(i[6].replace("§", "::"))
        if(typ == "header"):
            with open("template/hfunction.template", "r") as f:
                fu = f.read()

            if(i[0] == "public"):
                d["pbfunc"] += fu.format(**d)
            if(i[0] == "private"):
                d["pvfunc"] += fu.format(**d)
            if(i[0] == "protected"):
                d["ptfunc"] += fu.format(**d)
        else:
            with open("template/cppfunction.template", "r") as f:
                fu = f.read()
            d["func"] += fu.format(**d)
    return d

# reads config content from file and writes it to the right global variable


def readconfigfile(s):
    global baseclass
    global attributes
    global functions
    try:
        with open(s) as fi:
            next(fi)
            file = fi.read()
    except IOError:
        print(bcolors.FAIL + "ERROR:config file could not be opened. Ignoring config file..." + bcolors.ENDC)
        return
    else:
        file = file.split()
        b = file.index("--baseclass")
        a = file.index("--attributes")
        f = file.index("--functions")
        if(b == -1 or a == -1 or f == -1):
            print(
                bcolors.FAIL + "ERROR:wrong formatted config file. Ignoring config file..." + bcolors.ENDC)
            return
        else:
            b = file[b + 1:a]
            a = file[a + 1:f]
            f = file[f + 1:]
            if(b != []):
                for i in range(len(b)):
                    b[i] = b[i].replace(",", "")
                if "baseclass" in globals():
                    baseclass += b
                else:
                    baseclass = b
            if(a != []):
                a = " ".join(a)
                if "attributes" in globals():
                    attributes += a
                else:
                    attributes = a
            if(f != []):
                f = " ".join(f)
                if "functions" in globals():
                    functions += f
                else:
                    functions = f


def usage():
    print("Options for using the program code generator:")
    print("-? [--help]               : prints this message")
    print("-d [--debug]              : turns on the debug option")
    print("-h [--header]             : DISABLES generation of a header file")
    print("-c [--cpp]                : DISABLES generation of a cpp file")
    print("-n [--name] <input>       : name for the generated class, supports namespaces: namespacename::classname")
    print("-b [--baseclass] <input>  : header files of one or more baseclasses; comma seperated")
    print("-e [--empty]              : no standard methods will be generated")
    print(
        "-a [--attributes] <input> : list of class attributes that should be generated")
    print(
        "                            Form: accesstype[private|public|protected]:name:attributetype")
    print("                                  list seperated with comma")
    print("                                  put =value behind name for standard value")
    print(
        "-f [--functions] <input>  : list of member functions that should be generated")
    print(
        "                            Form: accesstype[private|public|protected]:name:returntype:[parameterlist]:[modifier list pre function]")
    print(
        "                                  :[modifier list post function]:[template parameters]")
    print("                                  list seperated with comma, paramterlist syntax same as attribute syntax without access parameter")
    print("                                  list in [] are optional, but a empty pair has to be included if a following list is not empty")
    print(
        "-t [--template] <input>   : list of class template parameters; comma seperated")
    print("-o [--clang-format] <style> turns on clang-format for the output files, only available on Linux systems")
    print("                            Style Options: None = default, LLVM, Google, Chromium, Mozilla, WebKit")
    print("-i [--include] <input>    : comma seperated list of files that should be included in the header file, <> and \"\" will be computed automaticly")
    print("-k [--config] <input>     : config file for easier use of long baseclass,attribute and functions lists. Example file: example.cfg")


def getBaseclass():
    if "debug" in globals():
        print(bcolors.OKGREEN + "START:Reading baseclass files..." + bcolors.ENDC)
    global virtualfunctions
    virtualfunctions = []
    global baseclass
    global baseclassn
    baseclassn = []
    for inp in baseclass:
        # delete leading whitespace to correct input error
        inp = inp.lstrip()
        # read in baseclass files
        try:
            with open(os.path.join(script_dir, inp)) as f:
                file = f.read()
        except OSError:
            print(bcolors.FAIL + "ERROR: Baseclassfile " + inp +
                  " could not be opened. Shutting down..." + bcolors.ENDC)
            sys.exit(2)
        file = file.split()

        # extract classname and ignore enum classes
        y = 0
        for i in range(len(file)):
            if(file[i] == "class" and file[i - 1] != "enum"):
                y = i + 1
                break
        if "debug" in globals():
            print("Extracting classname...")
        if(file[y].find(":") != -1):
            baseclassn.append(file[y].split(":")[0])
            if "debug" in globals():
                print("Baseclass name is " + file[y].split(":")[0] + "...")
        else:
            baseclassn.append(file[y])
            if "debug" in globals():
                print("Baseclass name is " + file[y] + "...")

        # read virtual functions and access parameter areas
        if "debug" in globals():
            print("Computing access parameter areas...")
        private = -1
        protected = -1
        public = -1
        virtual = []

        for i in range(len(file)):
            if(file[i] == "public:"):
                public = i
            elif(file[i] == "protected:"):
                protected = i
            elif(file[i] == "private:"):
                private = i
            elif(file[i] == "virtual"):
                virtual.append(i)

        # compute whether a method can be accessed from a subclass and remove inaccessible virtual methods from the list
        if(public <= private and protected <= private):
            for x in range(len(virtual)):
                if (virtual[x] >= private and virtual[x] != -1):
                    virtual[x] = -1
        elif(public >= private and protected >= private):
            for x in range(len(virtual)):
                if (virtual[x] <= min(public, protected) and virtual[x] != -1):
                    virtual[x] = -1
        elif(public <= private and protected >= private):
            for x in range(len(virtual)):
                if (virtual[x] >= private and virtual[x] <= protected and virtual[x] != -1):
                    virtual[x] = -1
        elif(protected <= private and public >= private):
            for x in range(len(virtual)):
                if (virtual[x] >= private and virtual[x] <= public and virtual[x] != -1):
                    virtual[x] = -1

        for x in range(virtual.count(-1)):
            virtual.remove(-1)

        out = []

        if "debug" in globals():
            print("Extracting virtual functions from accessible areas...")
        # extract the whole method definition/declaration (without body)
        for x in virtual:
            semikolon = 0
            end = 0
            count = 0
            for i in range(x):
                if(-1 != file[i].find(";") or -1 != file[i].find(":") or -1 != file[i].find("}")):
                    semikolon = i
            for i in range(x + 1, len(file)):
                if(-1 != file[i].find(";")):
                    end = i
                    break
                if(-1 != file[i].find("(")):
                    count += 1
                if(-1 != file[i].find(")")):
                    count -= 1
                # a method body is starting if there is a { on a global level (not in ()) and no semikolon before
                if(-1 != file[i].find("{") and count == 0):
                    end = i
                    break
            out.append([semikolon, x, end])

        # compute the actual word from the numeric markers in the file array
        func = []
        for x in out:
            x = [file[i] for i in range(x[0], x[2] + 1)]
            func.append(" ".join(x))

        # cut of everything before the method start and after finish, including global { and semikolon
        # necessary because if the baseclass is not correct formatted, some bits of the previous method could be at the front of this method
        out = []
        for x in func:
            for i in range(len(x)):
                if(x[i] == ";" or x[i] == ":" or x[i] == "}"):
                    x = x[i + 1:]
                    x = x[:max(x.find(";"), x.find("{"))]
                    out.append(x)
                    break
        virtualfunctions.extend(out)
    if "debug" in globals():
        print(bcolors.OKGREEN + "STOP:Reading baseclass files..." + bcolors.ENDC)


def generateHeaderFile():
    global functions
    global headerincludes
    global virtualfunctions
    global namespace

    if "debug" in globals():
        print(bcolors.OKGREEN + "START:Generating Header files..." + bcolors.ENDC)
    d = {}
    d["classname"] = classname
    d["define"] = "_" + classname.upper() + "_H_"
    d["include"] = ""
    d["namespacestart"] = ""
    d["namespacestop"] = ""

    # generate namespace start
    if "namespace" in globals():
        d["namespacestart"] = "using namespace " + \
            namespace + ";\nnamespace " + namespace + "\n{"
        d["namespacestop"] = "} //namespace " + namespace

    # generate include list
    if "baseclass" in globals():
        for i in baseclass:
            d["include"] += "#include \"" + i + "\"\n"

    if "headerincludes" in globals():
        if "debug" in globals():
            print("including:" + str(headerincludes))
        for x in headerincludes:
            if x.find(".h") != -1:
                d["include"] += "#include \"" + x + "\"\n"
            else:
                d["include"] += "#include <" + x + ">\n"

    # class template line is generated here
    if "tpl" in globals():
        d["template"] = generatetempl(tpl)
        if "debug" in globals():
            print("Generating class templateparameters...")
    else:
        d["template"] = ""
        if "debug" in globals():
            print("No template parameter defined...")

    # input information from baseclass
    if "baseclass" in globals():
        d["baseclass"] = " : public " + ", public ".join(baseclassn)
        if "debug" in globals():
            print("Baseclass defined:" + ",".join(baseclassn) + "...")
        d["basefunc"] = ""
        for i in virtualfunctions:
            d["basefunc"] += "   " + i + ";\n"
        if "debug" in globals():
            print("Writing virtual functions to file...")
    else:
        d["baseclass"] = ""
        d["basefunc"] = ""
        if "debug" in globals():
            print("No baseclass defined...")

    # if standard functions are generated they will be first generated as as dict value and then writen to the file
    if "nostandardfunc" not in globals():
        with open("template/stdfforheader.template", "r") as f:
            stdf = f.read()
        d["stdf"] = stdf.format(**d)
        if "debug" in globals():
            print("Generating standard functions...")
    else:
        d["stdf"] = ""
        if "debug" in globals():
            print("No standard functions are generated...")

    # additional functions are generated here
    if "functions" in globals():
        if "debug" in globals():
            print("Additional functions are generated...")
        # generates functions heads
        d = {**d, **genfunctions("header")}

    else:
        d["pbfunc"] = ""
        d["pvfunc"] = ""
        d["ptfunc"] = ""
        if "debug" in globals():
            print("No additional functions are defined...")

    # additional attributes are created here
    if "attributes" in globals():
        d["pbattributes"] = ""
        d["pvattributes"] = ""
        d["ptattributes"] = ""
        if "debug" in globals():
            print("Additional attributes are generated...")
        for par in getelem(attributes, ",", "<", ">"):
            # replace :: so that split can seperate parts of definition
            par = par.replace("::", "§")
            # removes leading whitespace to correct for input errors
            par = par.lstrip()
            a = par.split(":")
            if(a[0] == "public"):
                d["pbattributes"] += "    " + \
                    a[2].replace("§", "::") + " " + \
                    a[1].replace("§", "::") + ";\n"
            if(a[0] == "private"):
                d["pvattributes"] += "    " + \
                    a[2].replace("§", "::") + " " + \
                    a[1].replace("§", "::") + ";\n"
            if(a[0] == "protected"):
                d["ptattributes"] += "    " + \
                    a[2].replace("§", "::") + " " + \
                    a[1].replace("§", "::") + ";\n"
    else:
        d["pbattributes"] = ""
        d["pvattributes"] = ""
        d["ptattributes"] = ""
        if "debug" in globals():
            print("No addtional attributes are defined...")

    # write all the stuff to a the file
    with open("template/header.template", "r") as ftemp:
        template = ftemp.read()
    with open("output/" + classname + ".h", "w") as out:
        out.write(template.format(**d))

    if "debug" in globals():
        print(bcolors.OKGREEN + "STOP:Generating Header Files..." + bcolors.ENDC)


def generateImplementationFiles():
    global virtualfunctions
    global baseclass
    global namespace
    if "debug" in globals():
        print(bcolors.OKGREEN +
              "START:Generating implementation file..." + bcolors.ENDC)

    d = {}
    d["classname"] = classname
    d["binclude"] = "#include \"" + classname + ".h\"\n"
    d["namespacestart"] = ""
    d["namespacestop"] = ""

    # generate namespace start
    if "namespace" in globals():
        d["namespacestart"] = "namespace " + namespace + "\n{"
        d["namespacestop"] = "} //namespace " + namespace

    # additional functions are generated here
    if "functions" in globals():
        if "debug" in globals():
            print("Additional functions are generated...")
        d = {**d, **genfunctions("Implemantation", classname)}

    else:
        d["func"] = ""

    if "virtualfunctions" in globals():
        func = []
        for i in range(len(virtualfunctions)):
            j = virtualfunctions[i].split()
            if(j[-1].find("0") == -1 and j[-1].find("default") == -1):
                func.append("")
                for k in j:
                    if(k.find("(") != -1):
                        k = classname + "::" + k
                    func[-1] += " " + k
        for i in func:
            d["func"] += i + "\n{\n\n}\n\n"

    # generate implementaion files content from template
    if "nostandardfunc" not in globals():

        with open("template/cpp.template", "r") as ftemp:
            template = ftemp.read()
        with open("output/" + classname + ".cpp", "w") as out:
            out.write(template.format(**d))
        if "debug" in globals():
            print("Generating standard functions...")
    else:
        with open("output/" + classname + ".cpp", "w") as out:
            out.write(d["func"])
        if "debug" in globals():
            if(d["func"] != ""):
                print("Additional Functions are generated...")
            print("No standard functions are generated...")

    if "debug" in globals():
        print(bcolors.OKGREEN +
              "STOP:Generating implementation file..." + bcolors.ENDC)


def main(argv):
    # get parameter from console
    try:
        opts, args = getopt.getopt(argv, "hcdn:b:?ea:f:t:o:i:k:", [
                                   "header", "cpp", "debug", "name=", "baseclass=", "help", "empty", "attributes=", "functions=", "template=", "clang-format=", "include=", "config="])
    except getopt.GetoptError as err:
        print(bcolors.FAIL + "ERROR:", str(err) + bcolors.ENDC)
        usage()
        sys.exit(2)
    # write command line arguments to variables
    for opt, arg in opts:
        if opt in ("-?", "--help"):
            usage()
            sys.exit()
        elif opt in ("-d", "--debug"):
            global debug
            debug = 1
        elif opt in ("-h", "--header"):
            global noheader
            noheader = 1
        elif opt in ("-c", "--cpp"):
            global nocpp
            nocpp = 1
        elif opt in ("-n", "--name"):
            global classname
            if(arg.find("::") != -1):
                arg = arg.split("::")
                classname = arg[1]
                global namespace
                namespace = arg[0]
            else:
                classname = arg
        elif opt in ("-b", "--baseclass"):
            global baseclass
            baseclass = arg
            baseclass = baseclass.split(",")
        elif opt in ("-e", "--empty"):
            global nostandardfunc
            nostandardfunc = 1
        elif opt in ("-a", "--attributes"):
            global attributes
            attributes = arg
        elif opt in ("-f", "--functions"):
            global functions
            functions = arg
        elif opt in ("-t", "--template"):
            global tpl
            tpl = arg
        elif opt in ("-o", "--clang-format"):
            global clang
            clang = arg
            if (clang != "None" and clang != "LLVM" and clang != "Google" and clang != "Chromium" and clang != "Mozilla" and clang != "WebKit"):
                print(bcolors.FAIL + "ERROR: Clang style not supported" + bcolors.ENDC)
                print(bcolors.WARNING +
                      "Ignoring clang-format flag..." + bcolors.ENDC)
                del clang
        elif opt in ("-i", "--include"):
            global headerincludes
            headerincludes = arg.split(",")
        elif opt in ("-k", "--config"):
            # reads config file and saves the content to the fitting global variables
            readconfigfile(arg)

    if "classname" not in globals():
        print(bcolors.FAIL + "ERROR: A classname has to be provided" + bcolors.ENDC)
        sys.exit(2)
    if "baseclass" in globals():
        getBaseclass()
    if "noheader" not in globals():
        generateHeaderFile()
    if "nocpp" not in globals():
        generateImplementationFiles()

    if (platform.system() == "Linux" and "clang" in globals()):
        if "debug" in globals():
            print(bcolors.OKGREEN + "START:Calling clang-format..." + bcolors.ENDC)
        if clang == "None":
            os.system("clang-format -style=file -i output/" + classname + ".h")
            os.system("clang-format -style=file -i output/" +
                      classname + ".cpp")
        else:
            os.system("clang-format -style=" + clang +
                      " -i output/" + classname + ".h")
            os.system("clang-format -style=" + clang +
                      " -i output/" + classname + ".cpp")
        if "debug" in globals():
            print(bcolors.OKGREEN + "STOP:clang-format finished..." + bcolors.ENDC)
    elif(platform.system() != "Linux"):
        print("Error: clang is not supported on this operating system...")
        print("Ignoring clang flag.")

    if (platform.system() == "Linux"):
        if "debug" in globals():
            print(bcolors.OKGREEN + "START:Doing compile test..." + bcolors.ENDC)
        os.system("clang++ -std=c++14 -Wall -Wextra -c output/" +
                  classname + ".cpp")
        try:
            os.remove(classname + ".o")
        except OSError:
            pass

        if "debug" in globals():
            print(bcolors.OKGREEN + "STOP:compile test finished..." + bcolors.ENDC)

    print(bcolors.OKGREEN + "...done!" + bcolors.ENDC)


if __name__ == "__main__":
    main(sys.argv[1:])
