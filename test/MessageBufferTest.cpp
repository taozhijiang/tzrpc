#include <iostream>
#include <string>

#include <gmock/gmock.h>
using namespace ::testing;

#include <Core/Message.h>
#include <Core/Buffer.h>

using namespace tzrpc;

TEST(MessageBufferTest, MessageHeadTest) {

    struct Header head {};

    head.magic   = kHeaderMagic;
    head.version = kHeaderVersion;
    head.length  = 0;

    std::cout << head.dump() << std::endl;

    head.to_net_endian();
    std::cout << head.dump() << std::endl;
    ASSERT_TRUE( (head.magic &0xFF) == (kHeaderMagic >> 8) && (head.magic >> 8) == (kHeaderMagic & 0xFF));
    ASSERT_TRUE( (head.version &0xFF) == (kHeaderVersion >> 8) && (head.version >> 8) == (kHeaderVersion & 0xFF));

    std::cout << head.dump() << std::endl;
}


TEST(MessageBufferTest, BufferTest) {

    const std::string str1 = "nicol";
    Buffer buff(str1);
    ASSERT_THAT(strncmp(buff.get_data(), str1.c_str(), str1.size()), Eq(0));
    ASSERT_THAT(buff.get_length(), Eq(str1.size()));

    const std::string str2 = "taokan";
    buff.append_internal(str2);
    ASSERT_THAT(strncmp(buff.get_data(), "nicoltaokan", buff.get_length()), Eq(0));
    ASSERT_THAT(buff.get_length(), Eq(str1.size() + str2.size()));

    std::string store1;
    ASSERT_FALSE(buff.consume(store1, 0));
    ASSERT_TRUE(buff.consume(store1, 5));
    ASSERT_THAT(store1, Eq(str1));
    ASSERT_THAT(buff.get_length(), str2.size());

    ASSERT_TRUE(buff.consume(store1, 100));
    ASSERT_THAT(store1, Eq(str2));
    ASSERT_THAT(buff.get_length(), 0);
}
