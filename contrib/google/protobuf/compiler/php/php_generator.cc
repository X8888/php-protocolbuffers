#include "php_generator.h"

#include <google/protobuf/io/printer.h>
#include <google/protobuf/io/zero_copy_stream.h>
#include <google/protobuf/descriptor.pb.h>
#include "strutil.h"
#include "php_file.h"

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

    // -----------------------------------------------------------------
    // parse generator options

    // Name a file where we will write a list of generated file names, one
    // per line.
    string output_list_file;
    vector<pair<string, string> > options;
    string php_filename = "proto.pb.php";

    ParseGeneratorParameter(parameter, &options);

    for (int i = 0; i < options.size(); i++) {
        if (options[i].first == "output_list_file") {
            output_list_file = options[i].second;
        } else {
            *error = "Unknown generator option: " + options[i].first;
            return false;
        }
    }

    // -----------------------------------------------------------------


    FileGenerator file_generator(file);
    if (!file_generator.Validate(error)) {
        return false;
    }

    vector<string> all_files;
    string package_dir = "Hoge";

    php_filename += file_generator.classname();
    php_filename += ".php";
    all_files.push_back(php_filename);

    scoped_ptr<io::ZeroCopyOutputStream> output(
        context->Open(php_filename)
    );
    io::Printer printer(output.get(), '$');

    file_generator.Generate(&printer);
    file_generator.GenerateSiblings(package_dir, context, &all_files);

    if (!output_list_file.empty()) {
        // Generate output list.  This is just a simple text file placed in a
        // deterministic location which lists the .java files being generated.
        scoped_ptr<io::ZeroCopyOutputStream> srclist_raw_output(
            context->Open(output_list_file)
        );

        io::Printer srclist_printer(srclist_raw_output.get(), '$');

        for (int i = 0; i < all_files.size(); i++) {
            srclist_printer.Print("$filename$\n", "filename", all_files[i]);
        }
    }

    return true;
}

}  // namespace php
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
