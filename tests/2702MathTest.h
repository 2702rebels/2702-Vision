#include <gtest/gtest.h>
#include <2702math.h>

TEST(Point2d, Point2dSet) {
    Rebels::Point2d point(1, 1.5);
    EXPECT_EQ(point.u(), 1);
    EXPECT_EQ(point.v(), 1.5);
    point.set(-2.3, 15.4);
    EXPECT_NEAR(point.u(), -2.3, 1e-6);
    EXPECT_NEAR(point.v(), 15.4, 1e-6);
}

TEST(Point2d, Point2dEq) {
    EXPECT_EQ(Rebels::Point2d(1, 1.5), Rebels::Point2d(1, 1.5));
}

TEST(Point2d, Point2dFromExisting) {
    Rebels::Point2d point1(1.5, 2.5);
    Rebels::Point2d point2(point1);
    EXPECT_EQ(point1, point2);
}

TEST(Point2d, Point2dLen) {
    Rebels::Point2d point(3, 4);
    EXPECT_EQ(point.length(), 5);
}