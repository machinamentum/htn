
#include <cstdio>

#include <string>
#include <fstream>
#include <streambuf>
#include <vector>
#include <cstdint>
#include <stack>
#include <cstring>

#include "Code_Gen.h"
#include "Lexer.h"
#include "Parser.h"
#include "common.h"

Function::
Function() {
   scope = new Scope();
   scope->is_function = true;
   scope->function = this;
}

Expression::
Expression() {
   scope = new Scope();
}

extern std::stack<std::string> source_file_name;
extern int error_count;
std::vector<std::string> includes;
std::string prefix_dir = STRING(PREFIX) + "/";

int file_exists(const std::string pathname) {
   int out = 1;
   std::ifstream t(pathname);
   if (!t.good()) {
      out = 0;
   }
   t.close();
   return out;
}

int file_exists_with_include(const std::string pathname) {
   for (std::string ipath : includes) {
      if (file_exists(ipath + "/" + pathname)) return 1;
   }

   return file_exists(prefix_dir + "include/" + pathname);
}

std::string load_file(const std::string pathname) {
   std::ifstream t(pathname);
   if (!t.good()) {
      //printf("File not found: %s\n", pathname.c_str());
      return "";
   }
   std::string str;

   t.seekg(0, std::ios::end);
   str.reserve(t.tellg());
   t.seekg(0, std::ios::beg);

   str.assign((std::istreambuf_iterator<char>(t)), std::istreambuf_iterator<char>());
   return str;
}

std::string load_file_with_include(const std::string pathname) {
   for (std::string ipath : includes) {
      if (file_exists(ipath + "/" + pathname)) {
         return load_file(ipath + "/" + pathname);
      }
   }

   return load_file(prefix_dir + "include/" + pathname);
}

std::vector<char> load_bin_file(const std::string pathname) {
   std::ifstream t(pathname);
   std::vector<char> str;

   t.seekg(0, std::ios::end);
   str.reserve(t.tellg());
   t.seekg(0, std::ios::beg);

   str.assign((std::istreambuf_iterator<char>(t)), std::istreambuf_iterator<char>());
   return str;
}

#include <iostream>


#include "Gen_386.h"
#include "Gen_ARM.h"
#include "Target.h"
#include "common.h"

Target *target = NULL;
std::string ident_str = "HTN (alpha development build) " + STRING(BRANCH_COMMIT);

static void generate_386(Scope &scope, std::ostream &os) {
   os << target->as_text_section() << std::endl;
   Gen_386 g386 = Gen_386(os);
   g386.gen_scope(scope);

   os << target->as_rodata_section() << std::endl;
   g386.gen_rodata();
   os << "\t.ident\t\"" << ident_str << "\"" << std::endl;
}

static void generate_arm(Scope &scope, std::ostream &os) {
   os << target->as_text_section() << std::endl;
   os << "\t.arch armv5te\n\t.fpu softvfp" << std::endl;
   os << "\t.thumb" << std::endl;
   // os << "\t.syntax unified" << std::endl;
   os << "\t.align 2" << std::endl;
   os << "\t.eabi_attribute 23, 1\n\t.eabi_attribute 24, 1\n\t.eabi_attribute 25, 1\n\t.eabi_attribute 26, 1\n\t.eabi_attribute 30, 2\n\t.eabi_attribute 34, 0\n\t.eabi_attribute 18, 4" << std::endl;
   Gen_ARM gARM = Gen_ARM(os);
   gARM.gen_scope(scope);

   os << target->as_rodata_section() << std::endl;
   gARM.gen_rodata();
   os << "\t.ident\t\"" << ident_str << "\"" << std::endl;
}

#include <fstream>
#include <string>
#include <stdio.h>
#include <sstream>



std::string exec(std::string cmd) {
    printf("%s\n", cmd.c_str());
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) return "ERROR";
    char buffer[128];
    std::string result = "";
    while(!feof(pipe)) {
    	if(fgets(buffer, 128, pipe) != NULL)
    		result += buffer;
    }
    pclose(pipe);
    return result;
}


static std::string output_file;
bool no_link = false;
static std::string link_options = "";
bool no_del_s = false;


#include <cstdio>

void assemble(std::string path_str) {
   std::string out(path_str);
   out.replace(out.rfind(".s"), 2, ".o");
   if (no_link) {
      if (output_file.compare("") == 0) {
         output_file = out;
      }
      std::cout << exec(std::string(target->get_target_as() + target->arch_flag() + " -g -o ") + output_file + " " + path_str) << std::endl;
   } else {
      std::cout << exec(std::string(target->get_target_as() + target->arch_flag() + " -g -o ") + out + " " + path_str) << std::endl;
      if (output_file.compare("") == 0) {
         output_file = out;
         output_file.replace(output_file.rfind(".o"), 2, "");
      }
      std::string _static = "";
      if (link_options.size() == 0) {
         _static = "-static ";
      }
      std::cout << exec(std::string(target->get_target_ld() + " " + _static + target->link_ops()  +  " " + " -o ") + output_file + " " + out + " " + link_options) << std::endl;
      remove(out.c_str());
   }
   if (!no_del_s) {
      remove(path_str.c_str());
   }
}

static void print_usage() {
   printf("%s\n", ident_str.c_str());
   printf("Usage: htn [options] <sources> \n");
   printf("\nOptions:\n");
   printf("  --target <sys>  Specifies the CPU/OS to compile to.\n");
   printf("  -o       <out>  Specify file for output\n");
   printf("  -c              Stop after compilation, does not invoke linker\n");
}

int main(int argc, char** argv) {
   if (argc < 2) {
      print_usage();
      return 0;
   }

   std::string def_tar = STRING(DEFAULT_TARGET);

   std::string source_path;
   for (int i = 1; i < argc; ++i) {
      std::string arch = argv[i];
      if (arch.compare("-c") == 0) {
         no_link = true;
      } else if (arch.compare("-S") == 0) {
         no_del_s = true;
      } else if (arch.compare(0, 2, "-I") == 0) {
         std::string include_path = arch.substr(2);
         printf("New include path: %s\n", include_path.c_str());
         includes.push_back(include_path);
      } else if (arch.compare("-o") == 0) {
         ++i;
         if (i >= argc) {
            printf("Not enough args to support -o switch\n");
            return -1;
         }
         output_file = argv[i];
      } else if (arch.compare("--target") == 0) {
         ++i;
         if (i >= argc) {
            printf("Not enough args to support --target\n");
            return -1;
         }
         def_tar = argv[i];
         std::cout << "New target: " << def_tar << std::endl;
      } else if (arch.compare(0, 2, "-l") == 0) {
         link_options += arch + " ";
      } else if (arch.compare("-framework") == 0) {
         link_options += arch + " ";
         ++i;
         if (i >= argc) {
            printf("Not enough args to support -framework\n");
            break;
         }
         std::string fw = argv[i];
         link_options += fw + " ";
      } else if (arch.rfind("-") == 0) {
         printf("Unrecognized option: %s\n", arch.c_str());
      } else if (arch.find(".a") != std::string::npos) {
         link_options += arch + " ";
      }else {
         source_path = argv[i];
      }
   }
   prefix_dir += def_tar + "/";
   if (def_tar.find("darwin") != std::string::npos) {
      target = new Target_Apple(def_tar);
   } else {
      target = new Target_GNU(def_tar);
   }

   if (source_path.compare("") == 0) {
      print_usage();
      return -1;
   }
   std::string source = load_file(source_path);
   source_file_name.push(source_path);
   Scope scope = Parser::parse(source);
   source_file_name.pop();
   if (error_count) {
      return -1;
   }
   source_path.replace(source_path.rfind(".htn"), std::string::npos, ".s");
   std::ofstream ofs(source_path);
   if (target->get_target_cpu() == Target::X86) {
      generate_386(scope, ofs);
      ofs.close();
      assemble(source_path);
   } else if (target->get_target_cpu() == Target::ARM) {
      generate_arm(scope, ofs);
      ofs.close();
      assemble(source_path);
   } else {
      std::cout << "Invalid target triple: " << target->target_triple << std::endl;
   }

   return 0;
}
