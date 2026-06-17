#include "boost/test/unit_test.hpp"
#include "../../jrd/blb.h"

#include "TestContext.h"

#include <utility>


BOOST_AUTO_TEST_SUITE(EngineSuite)
BOOST_AUTO_TEST_SUITE(JrdClassesSuite)


BOOST_AUTO_TEST_SUITE(BlobTests)

BOOST_AUTO_TEST_SUITE(BlobRandomWriteTests)

namespace {

static constexpr UCHAR STREAM_BLOB_BPB[] = {
	isc_bpb_version1,
	isc_bpb_type, 1, isc_bpb_type_stream,
};


std::string getDefaultString(std::string_view header = "", int pageNumber = 0, std::optional<char> dum = std::nullopt)
{
	std::string output;
	output += header;

	// Bigger page size - level 1
	// Bigger 2 pages - level 2
	auto tdbb = JRD_get_thread_data();
	auto size = tdbb->getDatabase()->dbb_page_size;

	for (int i = 0 ; i < pageNumber; i++)
	{
		std::string dummy;
		dummy.resize(size, dum.value_or('0' + i));
		output += dummy;
	}

	return output;
}

Jrd::blb* makeBlob(Jrd::bid& id, std::string_view testData = "")
{
	auto tdbb = JRD_get_thread_data();

	Jrd::blb* blob = Jrd::blb::create2(tdbb, tdbb->getTransaction(), &id, sizeof(STREAM_BLOB_BPB), STREAM_BLOB_BPB);
	BOOST_REQUIRE(blob != nullptr);

	blob->BLB_put_data(tdbb, (const UCHAR*)testData.data(), testData.length());

	return blob;
}


std::string readBlob(Jrd::bid id)
{
	auto tdbb = JRD_get_thread_data();

	auto blob = Jrd::blb::open(tdbb, tdbb->getTransaction(), &id);

	std::string buffer;
	buffer.resize(blob->blb_length, '\0');
	const ULONG readLength = blob->BLB_get_data(JRD_get_thread_data(), (UCHAR*)buffer.data(), blob->blb_length, true);
	return buffer;
}

void replaceInBlob(Jrd::thread_db* tdbb, Jrd::blb*& blob, const ULONG pos, const std::string_view replacement)
{
	blob->BLB_write(tdbb, pos, replacement.data(), replacement.length());
	blob->BLB_close(tdbb);
	blob = nullptr;
}

std::string replaceInContent(std::string defaultData, ULONG posToInplace, std::string_view contentToInplace)
{
	auto sourceLength = defaultData.length();

	const auto replacementEnd = posToInplace + contentToInplace.length();
	if (replacementEnd < sourceLength)
	{
		for (ULONG i = 0; i < contentToInplace.length(); ++i)
		{
			defaultData[posToInplace + i] = contentToInplace[i];
		}
	}
	else
	{
		defaultData.resize(posToInplace);
		defaultData += contentToInplace;
	}
	return defaultData;
}

} // anonymous namespace

BOOST_FIXTURE_TEST_CASE(Level0Test, EngineHolder)
{
	Jrd::bid id;

	{ // level 0
		const std::string_view testData = "Hello World, BLB_get_data, level=0";

		// Full rewrite
		auto blob = makeBlob(id, testData);
		std::string buffer;
		buffer.resize(blob->blb_length, '*');
		replaceInBlob(tdbb, blob, 0, buffer);
		BOOST_TEST(readBlob(id) == buffer);

		// Middle write
		blob = makeBlob(id, testData);
		replaceInBlob(tdbb, blob, 12, " __BLB_write_,");
		BOOST_TEST(readBlob(id) == "Hello World, __BLB_write_, level=0");

		// Ending is out of range - add to end
		blob = makeBlob(id, testData);
		replaceInBlob(tdbb, blob, 27, testData);
		BOOST_TEST(readBlob(id) == "Hello World, BLB_get_data, Hello World, BLB_get_data, level=0");

		// Beginning is out of range
		blob = makeBlob(id, testData);
		BOOST_CHECK_THROW(blob->BLB_write(tdbb, 40, (const void*)testData.data(), testData.length()), Firebird::Exception);
	}
}

BOOST_FIXTURE_TEST_CASE(Level1Test, EngineHolder)
{
	Jrd::bid id;
	Jrd::blb* blob = nullptr;

	std::string result;
	std::string expected;

	const std::string_view testData = "Hello World, BLB_get_data, level=1 | ";
	std::string defaultData = getDefaultString(testData, 1);
	{
		// Full rewrite
		auto blob = makeBlob(id, defaultData);

		replaceInBlob(tdbb, blob, 0, "new data");

		result = readBlob(id);
		expected = replaceInContent(defaultData, 0, "new data");
		BOOST_TEST(result == expected);
	}

	std::string replacement;

	{
		// Middle to end write
		replacement.resize(200, '*');
		blob = makeBlob(id, defaultData);
		replaceInBlob(tdbb, blob, blob->blb_length - 200, replacement);

		result = readBlob(id);
		expected = replaceInContent(defaultData, defaultData.length() - 200, replacement);
		BOOST_TEST(result.length() == expected.length());
		BOOST_REQUIRE(result.substr(0, 400) == expected.substr(0, 400));
		BOOST_REQUIRE(result.substr(result.length() - 300) == expected.substr(expected.length() - 300));
		BOOST_TEST(result == expected);
	}

	{
		// Middle
		replacement.resize(200, '*');
		blob = makeBlob(id, defaultData);
		replaceInBlob(tdbb, blob, blob->blb_length - 4000, replacement);

		result = readBlob(id);
		expected = replaceInContent(defaultData, defaultData.length() - 4000, replacement);
		BOOST_TEST(result.length() == expected.length());
		BOOST_REQUIRE(result.substr(result.length() - 4000, 300) == expected.substr(expected.length() - 4000, 300));
		BOOST_TEST(result == expected);
	}

	{
		// Ending is out of range - add to end
		blob = makeBlob(id, defaultData);
		replacement.clear();
		replacement.resize(blob->blb_length, '@');
		const auto insertPos = blob->blb_length - 1000;
		replaceInBlob(tdbb, blob, insertPos, replacement);

		result = readBlob(id);
		expected = replaceInContent(defaultData, insertPos, replacement);
		BOOST_TEST(result.length() == expected.length());
		BOOST_REQUIRE(result.substr(0, 400) == expected.substr(0, 400));
		BOOST_REQUIRE(result.substr(result.length() - 300) == expected.substr(expected.length() - 300));
		BOOST_TEST(result == expected);
	}

	{
		defaultData = getDefaultString(testData, 8);
		replacement = getDefaultString(testData, 3, '*');

		// Big
		blob = makeBlob(id, defaultData);

		const auto insertPos = blob->blb_length / 2;
		blob->BLB_write(tdbb, insertPos, replacement.data(), replacement.length());
		blob->BLB_close(tdbb);
		blob = nullptr;

		result = readBlob(id);
		expected = replaceInContent(defaultData, insertPos, replacement);
		BOOST_REQUIRE(result.length() == expected.length());

		std::string_view resultView(result);
		std::string_view expectedView(expected);
		for (FB_SIZE_T i = 0; i < expected.length(); i += 1000)
		{
			auto left = std::min<int>(1000, expected.length() - i);
			BOOST_TEST(resultView.substr(i, left) == expected.substr(i, left));
		}
	}
}


BOOST_FIXTURE_TEST_CASE(Level2Test, EngineHolder)
{
	// Takes some time

	Jrd::bid id;
	Jrd::blb* blob = nullptr;

	std::string result;
	std::string expected;

	const std::string_view testData = "Hello World, BLB_get_data, level=2 | ";
	std::string defaultData;
	std::string replacement;

	{
		blob = makeBlob(id, defaultData);

		const auto insertPos = blob->blb_length / 2;
		replaceInBlob(tdbb, blob, insertPos, replacement);

		result = readBlob(id);
		expected = replaceInContent(defaultData, insertPos, replacement);
		BOOST_REQUIRE(result.length() == expected.length());

		std::string_view resultView(result);
		std::string_view expectedView(expected);
		for (FB_SIZE_T i = 0; i < expected.length(); i += 1000)
		{
			auto left = std::min<int>(1000, expected.length() - i);
			BOOST_TEST_INFO("Chunk position is " + std::to_string(i));
			BOOST_TEST(resultView.substr(i, left) == expected.substr(i, left));
			// if (resultView.substr(i, left) != expected.substr(i, left))
			// 	break;
		}
	}

	{
		defaultData = getDefaultString(testData, 5050);
		replacement = getDefaultString(testData, 150, '*');

		blob = makeBlob(id, defaultData);

		const auto insertPos = 1998;
		replaceInBlob(tdbb, blob, insertPos, replacement);

		result = readBlob(id);
		expected = replaceInContent(defaultData, insertPos, replacement);
		BOOST_REQUIRE(result.length() == expected.length());

		std::string_view resultView(result);
		std::string_view expectedView(expected);
		for (FB_SIZE_T i = 0; i < expected.length(); i += 1000)
		{
			auto left = std::min<int>(1000, expected.length() - i);
			BOOST_TEST_INFO("Chunk position is " + std::to_string(i));
			BOOST_TEST(resultView.substr(i, left) == expected.substr(i, left));
		}
	}
}

BOOST_AUTO_TEST_SUITE_END()	// BlobRandomWriteTest

BOOST_AUTO_TEST_SUITE_END()	// BlobTests

BOOST_AUTO_TEST_SUITE_END()	// JrdClassesSuite
BOOST_AUTO_TEST_SUITE_END()	// EngineSuite
