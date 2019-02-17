/*-
 * Copyright (c) 2019 TAO Zhijiang<taozhijiang@gmail.com>
 *
 * Licensed under the BSD-3-Clause license, see LICENSE for full information.
 *
 */

#include <memory>
#include <string>
#include <sstream>

#include <google/protobuf/message.h>
#include <google/protobuf/text_format.h>

#include <Utils/Log.h>

#ifndef __CORE_PROTOBUF_H__
#define __CORE_PROTOBUF_H__

namespace google {
namespace protobuf {

// 支持Protobuf的相等比较，可以用于单元测试
static inline
bool operator==(const Message& a, const Message& b) {
    return (a.GetTypeName() == b.GetTypeName() &&
            a.DebugString() == b.DebugString());
}


static inline
bool operator!=(const Message& a, const Message& b) {
	return !(a == b);
}

// Equality and inequality between protocol buffers and their text format
// representations. These are useful for testing.
static inline
bool operator==(const Message& a, const std::string& bStr) {

    // Create new instance of the same type
    std::unique_ptr<Message> b(a.New());

	// Create a LogSilencer if you want to temporarily suppress all log messages.
    LogSilencer _;
    TextFormat::ParseFromString(bStr, b.get());
    return (a == *b);
}

static inline
bool operator==(const std::string& a, const Message& b) {
	return (b == a);
}

static inline
bool operator!=(const Message& a, const std::string& b) {
	return !(a == b);
}

static inline
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
            return false;
        }

        return true;
    }

    static bool marshalling_to_string(const google::protobuf::Message& from, std::string* str ) {

        google::protobuf::LogSilencer _;

        if (!from.IsInitialized()) {
            return false;
        }

        return from.SerializeToString(str);
    }

};




} // end namespace tzrpc




#endif // __CORE_PROTOBUF_H__
