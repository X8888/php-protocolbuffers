#include "php_message.h"

#include <algorithm>
#include <map>
#include <vector>

#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/io/printer.h>
#include <google/protobuf/descriptor.pb.h>
#include <google/protobuf/wire_format.h>
#include "strutil.h"
#include "hash.h"
//#include <google/protobuf/stubs/substitute.h>

namespace google {
namespace protobuf {
namespace compiler {
namespace php {

using internal::WireFormat;
using internal::WireFormatLite;

namespace {

void PrintFieldComment(io::Printer* printer, const FieldDescriptor* field) {
}

struct FieldOrderingByNumber {
  inline bool operator()(const FieldDescriptor* a,
                         const FieldDescriptor* b) const {
    return a->number() < b->number();
  }
};

struct ExtensionRangeOrdering {
  bool operator()(const Descriptor::ExtensionRange* a,
                  const Descriptor::ExtensionRange* b) const {
    return a->start < b->start;
  }
};

// Sort the fields of the given Descriptor by number into a new[]'d array
// and return it.
const FieldDescriptor** SortFieldsByNumber(const Descriptor* descriptor) {
  const FieldDescriptor** fields =
    new const FieldDescriptor*[descriptor->field_count()];
  for (int i = 0; i < descriptor->field_count(); i++) {
    fields[i] = descriptor->field(i);
  }
  sort(fields, fields + descriptor->field_count(),
       FieldOrderingByNumber());
  return fields;
}

// Get an identifier that uniquely identifies this type within the file.
// This is used to declare static variables related to this type at the
// outermost file scope.
string UniqueFileScopeIdentifier(const Descriptor* descriptor) {
  return "static_" + StringReplace(descriptor->full_name(), ".", "_", true);
}

// Returns true if the message type has any required fields.  If it doesn't,
// we can optimize out calls to its isInitialized() method.
//
// already_seen is used to avoid checking the same type multiple times
// (and also to protect against recursion).
static bool HasRequiredFields(
    const Descriptor* type,
    hash_set<const Descriptor*>* already_seen) {
  return false;
}

static bool HasRequiredFields(const Descriptor* type) {
  hash_set<const Descriptor*> already_seen;
  return HasRequiredFields(type, &already_seen);
}

}  // namespace

// ===================================================================

MessageGenerator::MessageGenerator(const Descriptor* descriptor)
  : descriptor_(descriptor){
    //field_generators_(descriptor) {
}

MessageGenerator::~MessageGenerator() {}

void MessageGenerator::GenerateStaticVariables(io::Printer* printer) {
}

void MessageGenerator::GenerateStaticVariableInitializers(
    io::Printer* printer) {
}

// ===================================================================

void MessageGenerator::GenerateInterface(io::Printer* printer) {

}

// ===================================================================

void MessageGenerator::Generate(io::Printer* printer) {
}


// ===================================================================

void MessageGenerator::
GenerateMessageSerializationMethods(io::Printer* printer) {
}

void MessageGenerator::
GenerateParseFromMethods(io::Printer* printer) {
}

void MessageGenerator::GenerateSerializeOneField(
    io::Printer* printer, const FieldDescriptor* field) {
}

void MessageGenerator::GenerateSerializeOneExtensionRange(
    io::Printer* printer, const Descriptor::ExtensionRange* range) {
}

// ===================================================================

void MessageGenerator::GenerateBuilder(io::Printer* printer) {

}

void MessageGenerator::GenerateDescriptorMethods(io::Printer* printer) {
}

// ===================================================================

void MessageGenerator::GenerateCommonBuilderMethods(io::Printer* printer) {
}

// ===================================================================

void MessageGenerator::GenerateBuilderParsingMethods(io::Printer* printer) {
}

// ===================================================================

void MessageGenerator::GenerateIsInitialized(
    io::Printer* printer, UseMemoization useMemoization) {
  bool memoization = useMemoization == MEMOIZE;
}

// ===================================================================

void MessageGenerator::GenerateEqualsAndHashCode(io::Printer* printer) {
}

// ===================================================================

void MessageGenerator::GenerateExtensionRegistrationCode(io::Printer* printer) {
}

// ===================================================================
void MessageGenerator::GenerateParsingConstructor(io::Printer* printer) {
}

// ===================================================================
void MessageGenerator::GenerateParser(io::Printer* printer) {
}

}  // namespace php
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
