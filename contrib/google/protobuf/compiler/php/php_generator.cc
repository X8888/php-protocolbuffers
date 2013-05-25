#include "php_generator.h"

#include <google/protobuf/io/printer.h>
#include <google/protobuf/io/zero_copy_stream.h>
#include <google/protobuf/descriptor.pb.h>
#include "strutil.h"

namespace google {
namespace protobuf {
namespace compiler {
namespace php {

PhpGenerator::PhpGenerator() {}
PhpGenerator::~PhpGenerator() {}

bool PhpGenerator::Generate(const FileDescriptor* file,
                             const string& parameter,
                             GeneratorContext* context,
                             string* error) const {

    string output_list_file;
    vector<pair<string, string> > options;
    string php_filename = "proto.pb.php";


    scoped_ptr<io::ZeroCopyOutputStream> output(context->Open(php_filename));
    io::Printer printer(output.get(), '$');
    printer.Print("<?php\n");

    ParseGeneratorParameter(parameter, &options);

    for (int i = 0; i < options.size(); i++) {
        if (options[i].first == "output_list_file") {
            output_list_file = options[i].second;
        } else {
            *error = "Unknown generator option: " + options[i].first;
            return false;
        }
    }


    return true;
}

}  // namespace php
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
