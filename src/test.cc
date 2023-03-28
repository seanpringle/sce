
#include "doc.h"
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
