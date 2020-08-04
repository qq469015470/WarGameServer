#include <gtest/gtest.h>
#include "TestUserService.cpp"
#include "TestChatService.cpp"

int main(int _argc, char** _argv)
{
	::testing::InitGoogleTest(&_argc, _argv);

	db::Database db;

	db.UseDb("UnitTestDB");

	return RUN_ALL_TESTS();
}
