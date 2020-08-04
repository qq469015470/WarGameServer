#include "../UserService.h"

TEST(UserService, Register)
{
	UserService userService;

	userService.Register("qq469015470", "123123");

	auto result = userService.Login("qq469015470", "123123");

	EXPECT_TRUE(result.has_value());
}

TEST(UserService, InvaildRegister)
{
	UserService userService;

	EXPECT_THROW(userService.Register("123456789012345678910", "123123"), std::logic_error);
	EXPECT_THROW(userService.Register("1234567890", "123456789012345678901"), std::logic_error);
	EXPECT_THROW(userService.Register("", "123123"), std::logic_error);
	EXPECT_THROW(userService.Register("123123", ""), std::logic_error);
}

TEST(UserService, SameUserName)
{
	UserService userService;

	EXPECT_THROW(userService.Register("testA", "55555"), std::logic_error);
}

TEST(UserService, Login)
{
	UserService userService;

	auto result = userService.Login("testA", "123123");	

	EXPECT_TRUE(result.has_value());

	EXPECT_FALSE(userService.Login("none", "123123").has_value());


	auto result2 = userService.GetUser("5349b4ddd2781d08c09890f3");

	EXPECT_EQ(result2->name, "testB");
	EXPECT_EQ(result2->password, "456456");
}
