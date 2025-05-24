#include <gtest/gtest.h>
#include <2702math.h>

TEST(Point2d, Set) {
    Rebels::Point2d point(1, 1.5);
    EXPECT_EQ(point.u(), 1);
    EXPECT_EQ(point.v(), 1.5);
    point.set(-2.3, 15.4);
    EXPECT_NEAR(point.u(), -2.3, 1e-6);
    EXPECT_NEAR(point.v(), 15.4, 1e-6);
}

TEST(Point2d, EqOp) {
    EXPECT_EQ(Rebels::Point2d(1, 1.5), Rebels::Point2d(1, 1.5));
}

TEST(Point2d, FromExisting) {
    Rebels::Point2d point1(1.5, 2.5);
    Rebels::Point2d point2(point1);
    EXPECT_EQ(point1, point2);
}

TEST(Point2d, Len) {
    Rebels::Point2d point(3, 4);
    EXPECT_EQ(point.length(), 5);
}

TEST(Point3d, Set) {
    Rebels::Point3d point(0.75, 0.2, 0.5);
    EXPECT_EQ(point.x(), 0.75);
    EXPECT_EQ(point.y(), 0.2);
    EXPECT_EQ(point.z(), 0.5);
}

TEST(Point3d, EqOp) {
    EXPECT_EQ(Rebels::Point3d(0.75, 0.2, 0.5), Rebels::Point3d(0.75, 0.2, 0.5));
    EXPECT_NE(Rebels::Point3d(1, 0.25, 3), Rebels::Point3d(1, 0.25, 3.5));
}

TEST(Point3d, Len) {
    Rebels::Point3d point(4, 4, 2);
    EXPECT_NEAR(point.length(), 6, 1e-6);
}

TEST(Point3d, LenNegative){
    Rebels::Point3d point(-4, -4, -2);
    EXPECT_NEAR(point.length(), 6, 1e-6);
}

TEST(Point3d, Set){
    Rebels::Point3d point1(0.25, 0.5, 0.75);
    Rebels::Point3d point2 = point1;
    EXPECT_EQ(point1, point2);
}

TEST(Point3d, OpPlus){
    Rebels::Point3d point1(3, 3, 2);
    Rebels::Point3d point2(-5, -4, 0);
    Rebels::Point3d ans(-2, -1, 2);
    EXPECT_EQ(point1 + point2, ans);
    point1 += point2;
    EXPECT_EQ(point1, ans);
}

TEST(Point3d, OpMinus){
    Rebels::Point3d point1(3, 3, 2);
    Rebels::Point3d point2(-5, -4, 0);
    Rebels::Point3d ans(8, 7, 2);
    EXPECT_EQ(point1 - point2, ans);
    point1 -= point2;
    EXPECT_EQ(point1, ans);
}

TEST(Point3d, OpMul){
    Rebels::Point3d point(0.5, 2, 0);
    Rebels::Point3d ans(1, 4, 0);
    EXPECT_EQ(point * 2, ans);
    point *= 2;
    EXPECT_EQ(point, ans);
}

TEST(Point3d, OpDiv){
    Rebels::Point3d point(9, 4.5, 0);
    Rebels::Point3d ans(3, 4.5 / 3, 0);
    EXPECT_EQ(point / 3, ans);
    point /= 3;
    EXPECT_EQ(point, ans);
}

TEST(Point3d, CrossProduct){
    Rebels::Point3d point1(3, 4, 0);
    Rebels::Point3d point2(1, 2, 3);
    EXPECT_EQ(point1.CrossProduct(point2), Rebels::Point3d(12, -9, 2));
}

TEST(Point3d, DotProduct){
    Rebels::Point3d point1(3, 4, 0);
    Rebels::Point3d point2(1, 2, 3);
    EXPECT_FLOAT_EQ(point1.DotProduct(point2), 11);
}

TEST(Point3d, Normalize){
    Rebels::Point3d point1(0, 1, 0);
    point1.Normalize();
    EXPECT_EQ(point1, Rebels::Point3d(0, 1, 0));
    Rebels::Point3d point2(1, 0, 1);
    point2.Normalize();
    EXPECT_EQ(point2, Rebels::Point3d(0.5, 0, 0.5));
}