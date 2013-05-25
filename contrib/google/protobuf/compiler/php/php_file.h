#ifndef GOOGLE_PROTOBUF_COMPILER_PHP_FILE_H__
#define GOOGLE_PROTOBUF_COMPILER_PHP_FILE_H__

#include <string>
#include <vector>
#include <google/protobuf/stubs/common.h>
#include <google/protobuf/io/printer.h>
#include <google/protobuf/io/zero_copy_stream.h>
#include <google/protobuf/compiler/code_generator.h>
#include <google/protobuf/descriptor.pb.h>
#include <google/protobuf/dynamic_message.h>
#include "strutil.h"
#include "php_message.h"


using namespace std;

namespace google {
namespace protobuf {
  class FileDescriptor;        // descriptor.h
  namespace io {
    class Printer;             // printer.h
  }
  namespace compiler {
    class GeneratorContext;     // code_generator.h
  }
}




namespace protobuf {
namespace compiler {
namespace php {

class FileGenerator {
 public:
  explicit FileGenerator(const FileDescriptor* file);
  ~FileGenerator();

  // Checks for problems that would otherwise lead to cryptic compile errors.
  // Returns true if there are no problems, or writes an error description to
  // the given string and returns false otherwise.
  bool Validate(string* error);

  void Generate(io::Printer* printer);

  // If we aren't putting everything into one file, this will write all the
  // files other than the outer file (i.e. one for each message, enum, and
  // service type).
  void GenerateSiblings(const string& package_dir,
                        GeneratorContext* generator_context,
                        vector<string>* file_list);

  const string& php_package() { return php_package_; }
  const string& classname()    { return classname_;    }


 private:
  // Returns whether the dependency should be included in the output file.
  // Always returns true for opensource, but used internally at Google to help
  // improve compatibility with version 1 of protocol buffers.
  bool ShouldIncludeDependency(const FileDescriptor* descriptor);

  const FileDescriptor* file_;
  string php_package_;
  string classname_;


  void GenerateEmbeddedDescriptor(io::Printer* printer);

  GOOGLE_DISALLOW_EVIL_CONSTRUCTORS(FileGenerator);
};

}  // namespace php
}  // namespace compiler
}  // namespace protobuf

}  // namespace google
#endif  // GOOGLE_PROTOBUF_COMPILER_PHP_FILE_H__
