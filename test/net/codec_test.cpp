#include <gtest/gtest.h>
#include "net/codec.h"
#include "message.pb.h"

TEST(CodecTest, should_encode_when_message_valid)
{
    im::Message msg;
    msg.mutable_header()->set_cmd(1);
    msg.mutable_header()->set_seq(100);
    msg.set_body("hello");

    auto encoded = Codec::Encode(msg);

    EXPECT_FALSE(encoded.empty());
}

TEST(CodecTest, should_decode_when_encoded_message_valid)
{
    im::Message msg;
    msg.mutable_header()->set_cmd(2);
    msg.mutable_header()->set_seq(200);
    msg.set_body("hello");

    auto encoded = Codec::Encode(msg);

    auto decoded = Codec::Decode(encoded.data() + 4, encoded.size() - 4);

    ASSERT_TRUE(decoded.has_value());
    EXPECT_EQ(decoded->header().cmd(), 2);
    EXPECT_EQ(decoded->header().seq(), 200);
    EXPECT_EQ(decoded->body(), "hello");
}

TEST(CodecTest, should_fail_when_decode_empty_buffer)
{
    auto decoded = Codec::Decode(nullptr, 0);

    EXPECT_FALSE(decoded.has_value());
}
