
#include "doc.h"
#include "utf8.h"
#include "gtest/gtest.h"

Doc doc;

TEST(doc, init) {
	doc.push_back("abc\ndef");
	EXPECT_EQ(std::string(doc), "abc\ndef");
	EXPECT_EQ(doc.size(), 7U);
	EXPECT_EQ(doc.lines.size(), 2U);
	EXPECT_EQ(doc[6], 'f');
	EXPECT_EQ(doc.last.index, 6U);
	EXPECT_EQ(doc.last.line, 1U);
	EXPECT_EQ(doc.last.cell, 2U);
	EXPECT_TRUE(doc.sanity());
}

TEST(doc, insert) {
	doc.insert(doc.begin()+2, 'a');
	EXPECT_EQ(std::string(doc), "abac\ndef");
	EXPECT_EQ(doc[2], 'a');
	EXPECT_EQ(doc.last.index, 2U);
	EXPECT_EQ(doc.last.line, 0U);
	EXPECT_EQ(doc.last.cell, 2U);
	EXPECT_EQ(doc.size(), 8U);
	EXPECT_EQ(doc.lines.size(), 2U);
	EXPECT_TRUE(doc.sanity());
}

TEST(doc, erase) {
	EXPECT_EQ(doc[4], '\n');
	EXPECT_EQ(doc.last.index, 4U);
	EXPECT_EQ(doc.last.line, 0U);
	EXPECT_EQ(doc.last.cell, 4U);
	doc.erase(doc.begin()+4);
	EXPECT_EQ(doc.last.index, 3U);
	EXPECT_EQ(doc.last.line, 0U);
	EXPECT_EQ(doc.last.cell, 3U);
	EXPECT_EQ(std::string(doc), "abacdef");
	EXPECT_EQ(doc[4], 'd');
	EXPECT_EQ(doc.last.index, 4U);
	EXPECT_EQ(doc.last.line, 0U);
	EXPECT_EQ(doc.last.cell, 4U);
	EXPECT_TRUE(doc.sanity());
}

TEST(UTF8Constructor, DecodeValidUTF8) {
    UTF8 utf8_input("Hello, 世界!"); // UTF-8 string with ASCII and non-ASCII characters

    EXPECT_EQ(utf8_input.decodeErrors.size(), 0);
    EXPECT_EQ(utf8_input.encodeErrors.size(), 0);
    EXPECT_EQ(utf8_input.codes.size(), 10);

    std::vector<uint32_t> expected_codes = {72, 101, 108, 108, 111, 44, 32, 19990, 30028, 33};
    EXPECT_EQ(utf8_input.codes, expected_codes);
}

TEST(UTF8Constructor, DecodeInvalidUTF8) {
    std::string invalid_utf8 = "Hello, 世界";
    invalid_utf8.push_back(static_cast<char>(0xC3)); // Adding an invalid continuation byte

    UTF8 utf8_input(invalid_utf8);

    EXPECT_EQ(utf8_input.decodeErrors.size(), 1);
    EXPECT_EQ(utf8_input.encodeErrors.size(), 0);
    EXPECT_EQ(utf8_input.codes.size(), 10);
}

TEST(UTF8Constructor, EncodeValidCodePoints) {
    std::vector<uint32_t> code_points = {72, 101, 108, 108, 111, 44, 32, 19990, 30028, 33};

    UTF8 utf8_input(code_points);

    EXPECT_EQ(utf8_input.decodeErrors.size(), 0);
    EXPECT_EQ(utf8_input.encodeErrors.size(), 0);
    EXPECT_EQ(utf8_input.text, "Hello, 世界!");
}

TEST(UTF8Constructor, EncodeInvalidCodePoints) {
    std::vector<uint32_t> code_points = {72, 101, 108, 108, 111, 44, 32, 19990, 30028, 33, 0x110000}; // Code point beyond Unicode range

    UTF8 utf8_input(code_points);

    EXPECT_EQ(utf8_input.decodeErrors.size(), 0);
    EXPECT_EQ(utf8_input.encodeErrors.size(), 1);
}

