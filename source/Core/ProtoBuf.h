#include <memory>
#include <string>

#include <google/protobuf/message.h>
#include <google/protobuf/text_format.h>

#include <Utils/Log.h>

#ifndef __CORE_PROTOBUF_H__
#define __CORE_PROTOBUF_H__

namespace google {
namespace protobuf {

// 支持Protobuf的相等比较，可以用于单元测试
bool operator==(const Message& a, const Message& b) {
    return (a.GetTypeName() == b.GetTypeName() &&
            a.DebugString() == b.DebugString());
}


bool operator!=(const Message& a, const Message& b) {
	return !(a == b);
}

// Equality and inequality between protocol buffers and their text format
// representations. These are useful for testing.
bool operator==(const Message& a, const std::string& bStr) {

    // Create new instance of the same type
    std::unique_ptr<Message> b(a.New());

	// Create a LogSilencer if you want to temporarily suppress all log messages.
    LogSilencer _;
    TextFormat::ParseFromString(bStr, b.get());
    return (a == *b);
}

bool operator==(const std::string& a, const Message& b) {
	return (b == a);
}

bool operator!=(const Message& a, const std::string& b) {
	return !(a == b);
}

bool operator!=(const std::string& a, const Message& b) {
	return !(a == b);
}

} // namespace google::protobuf
} // namespace google




namespace tzrpc {

class ProtoBuf {

public:

    static std::unique_ptr<google::protobuf::Message> copy(const google::protobuf::Message& protoBuf) {
        std::unique_ptr<google::protobuf::Message> ret(protoBuf.New());
        ret->CopyFrom(protoBuf);
        return ret;
    }

    static std::string dump(const google::protobuf::Message& protoBuf) {

        std::string output;
        google::protobuf::TextFormat::Printer printer;

        printer.SetUseShortRepeatedPrimitives(true);
        printer.PrintToString(protoBuf, &output);

        if (!protoBuf.IsInitialized()) {

            std::vector<std::string> errors;
            protoBuf.FindInitializationErrors(&errors);
            std::ostringstream outputBuf;
            outputBuf << output;

            for (auto it = errors.begin(); it != errors.end(); ++it) {
                outputBuf << *it << ": UNDEFINED\n";
            }
            return output + outputBuf.str();
        }

        return output;
    }


    // unmarshalling
    static bool unmarshalling_from_string(const std::string& str, google::protobuf::Message* protoBuf) {

        google::protobuf::LogSilencer _;

        if (!protoBuf->ParseFromString(str)) {
            log_err("Missing fields in protocol buffer of type %s: %s",
                    protoBuf->GetTypeName().c_str(),
                    protoBuf->InitializationErrorString().c_str());
            return false;
        }

        return true;
    }

    static bool marshalling_to_string(const google::protobuf::Message& from, std::string* str ) {

        google::protobuf::LogSilencer _;

        if (!from.IsInitialized()) {
            log_err("Missing fields in protocol buffer of type %s: %s (have %s)",
                    from.GetTypeName().c_str(),
                    from.InitializationErrorString().c_str(),
                    dump(from).c_str());
            return false;
        }

        return from.SerializeToString(str);
    }

};


#endif // __CORE_PROTOBUF_H__


} // end tzrpc
