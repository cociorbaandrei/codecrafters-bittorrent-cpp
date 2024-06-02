#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "utils.h"
#include <stdexcept>
using namespace utils::bencode;
class Bittorent : public ::testing::Test {
protected:
    static void SetUpTestSuite();
    void SetUp() override;
    void TearDown() override;
    void quitLoop();
    void setQuitTimeout(uint32_t durationInSeconds);
};

void Bittorent::SetUpTestSuite()
{
}

void Bittorent::SetUp()
{
}

void Bittorent::TearDown()
{
}

void Bittorent::quitLoop()
{
}

void Bittorent::setQuitTimeout(uint32_t durationInSeconds)
{

}

TEST_F(Bittorent, BittorentTest1)
{
    EXPECT_EQ(true, true);
}

TEST_F(Bittorent, DecodeInt) {
    size_t index = 0;
    BencodeInt value = decode_int("i123e", index);
    EXPECT_EQ(value, 123);
    EXPECT_EQ(index, 5);
}

TEST_F(Bittorent, DecodeStr) {
    size_t index = 0;
    BencodeStr value = decode_str("4:spam", index);
    EXPECT_EQ(value, "spam");
    EXPECT_EQ(index, 6);
}

TEST_F(Bittorent, DecodeList) {
    size_t index = 0;
    BencodeListPtr value = decode_list("l4:spam4:eggse", index);
    EXPECT_EQ(value->size(), 2);
    EXPECT_EQ(std::get<BencodeStr>(value->at(0)), "spam");
    EXPECT_EQ(std::get<BencodeStr>(value->at(1)), "eggs");
    EXPECT_EQ(index, 14);
}

TEST_F(Bittorent, DecodeDict) {
    size_t index = 0;
    BencodeDictPtr value = decode_dict("d3:bar4:spam3:fooi42ee", index);
    EXPECT_EQ(value->size(), 2);
    EXPECT_EQ(std::get<BencodeStr>((*value)["bar"]), "spam");
    EXPECT_EQ(std::get<BencodeInt>((*value)["foo"]), 42);
    EXPECT_EQ(index, 22);
}

TEST_F(Bittorent, DecodeBencodedValue) {
    size_t index = 0;
    BencodeValue value = decode_bencoded_v("i42e", index);
    EXPECT_EQ(std::get<BencodeInt>(value), 42);

    index = 0;
    value = decode_bencoded_v("4:spam", index);
    EXPECT_EQ(std::get<BencodeStr>(value), "spam");

    index = 0;
    value = decode_bencoded_v("l4:spam4:eggse", index);
    EXPECT_EQ(std::get<BencodeListPtr>(value)->size(), 2);

    index = 0;
    value = decode_bencoded_v("d3:bar4:spam3:fooi42ee", index);
    EXPECT_EQ(std::get<BencodeDictPtr>(value)->size(), 2);
}

TEST_F(Bittorent, SerializeInt) {
    BencodeInt value = 123;
    std::string encoded = serialize_int(value);
    EXPECT_EQ(encoded, "i123e");
}

TEST_F(Bittorent, SerializeStr) {
    BencodeStr value = "spam";
    std::string encoded = serialize_str(value);
    EXPECT_EQ(encoded, "4:spam");
}

TEST_F(Bittorent, SerializeList) {
    BencodeListPtr list = std::make_shared<BencodeList>();
    list->push_back(BencodeStr("spam"));
    list->push_back(BencodeStr("eggs"));
    std::string encoded = serialize_list(list);
    EXPECT_EQ(encoded, "l4:spam4:eggse");
}


TEST_F(Bittorent, SerializeDict) {
    BencodeDictPtr dict = std::make_shared<BencodeDict>();
    (*dict)["bar"] = BencodeStr("spam");
    (*dict)["foo"] = BencodeInt(42);
    std::string encoded = serialize_dict(dict);
    EXPECT_EQ(encoded, "d3:bar4:spam3:fooi42ee");
}
