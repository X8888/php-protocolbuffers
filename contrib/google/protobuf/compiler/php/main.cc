#include "php_generator.h"
#include <google/protobuf/compiler/plugin.h>

int main(int argc, char* argv[]) {
    google::protobuf::compiler::php::PhpGenerator generator;

   return PluginMain(argc, argv, &generator);
}