#include <2702math.h>
#include <gtest/gtest.h>

TEST(Point2d, Set) {
  Rebels::Point2d point(1, 1.5f);
  EXPECT_EQ(point.u(), 1);
  EXPECT_EQ(point.v(), 1.5f);
  point.set(-2.3f, 15.4f);
  EXPECT_NEAR(point.u(), -2.3f, 1e-6);
  EXPECT_NEAR(point.v(), 15.4f, 1e-6);
}

TEST(Point2d, EqOp) {
  EXPECT_EQ(Rebels::Point2d(1, 1.5f), Rebels::Point2d(1, 1.5f));
}

TEST(Point2d, FromExisting) {
  Rebels::Point2d point1(1.5f, 2.5f);
  Rebels::Point2d point2(point1);
  EXPECT_EQ(point1, point2);
}

TEST(Point2d, Len) {
  Rebels::Point2d point(3, 4);
  EXPECT_EQ(point.length(), 5);
}

TEST(Point3d, Set) {
  Rebels::Point3d point(0.75f, 0.2f, 0.5f);
  EXPECT_FLOAT_EQ(point.x(), 0.75f);
  EXPECT_FLOAT_EQ(point.y(), 0.2f);
  EXPECT_FLOAT_EQ(point.z(), 0.5f);
}

TEST(Point3d, EqOp) {
  EXPECT_EQ(Rebels::Point3d(0.75f, 0.2f, 0.5f),
            Rebels::Point3d(0.75f, 0.2f, 0.5f));
  EXPECT_NE(Rebels::Point3d(1, 0.25f, 3), Rebels::Point3d(1, 0.25f, 3.5f));
}

TEST(Point3d, Len) {
  Rebels::Point3d point(4, 4, 2);
  EXPECT_NEAR(point.length(), 6, 1e-6);
}

TEST(Point3d, LenNegative) {
  Rebels::Point3d point(-4, -4, -2);
  EXPECT_NEAR(point.length(), 6, 1e-6);
}

TEST(Point3d, Copy) {
  Rebels::Point3d point1(0.25f, 0.5f, 0.75f);
  Rebels::Point3d point2 = point1;
  EXPECT_EQ(point1, point2);
}

TEST(Point3d, OpPlus) {
  Rebels::Point3d point1(3, 3, 2);
  Rebels::Point3d point2(-5, -4, 0);
  Rebels::Point3d ans(-2, -1, 2);
  EXPECT_EQ(point1 + point2, ans);
  point1 += point2;
  EXPECT_EQ(point1, ans);
}

TEST(Point3d, OpMinus) {
  Rebels::Point3d point1(3, 3, 2);
  Rebels::Point3d point2(-5, -4, 0);
  Rebels::Point3d ans(8, 7, 2);
  EXPECT_EQ(point1 - point2, ans);
  point1 -= point2;
  EXPECT_EQ(point1, ans);
}

TEST(Point3d, OpMul) {
  Rebels::Point3d point(0.5f, 2, 0);
  Rebels::Point3d ans(1, 4, 0);
  EXPECT_EQ(point * 2, ans);
  point *= 2;
  EXPECT_EQ(point, ans);
}

TEST(Point3d, OpDiv) {
  Rebels::Point3d point(18, 9, 0);
  Rebels::Point3d ans(6, 3, 0);
  Rebels::Point3d divd = point / 3;
  EXPECT_FLOAT_EQ(divd.x(), ans.x());
  EXPECT_FLOAT_EQ(divd.y(), ans.y());
  EXPECT_FLOAT_EQ(divd.z(), ans.z());
  point /= 3;
  EXPECT_FLOAT_EQ(point.x(), ans.x());
  EXPECT_FLOAT_EQ(point.y(), ans.y());
  EXPECT_FLOAT_EQ(point.z(), ans.z());
}

TEST(Point3d, CrossProduct) {
  Rebels::Point3d point1(3, 4, 0);
  Rebels::Point3d point2(1, 2, 3);
  EXPECT_EQ(point1.CrossProduct(point2), Rebels::Point3d(12, -9, 2));
}

TEST(Point3d, DotProduct) {
  Rebels::Point3d point1(3, 4, 0);
  Rebels::Point3d point2(1, 2, 3);
  EXPECT_FLOAT_EQ(point1.DotProduct(point2), 11);
}

TEST(Point3d, Normalize) {
  Rebels::Point3d point1(0, 1, 0);
  point1.Normalize();
  EXPECT_FLOAT_EQ(point1.x(), 0);
  EXPECT_FLOAT_EQ(point1.y(), 1);
  EXPECT_FLOAT_EQ(point1.z(), 0);
  Rebels::Point3d point2(1, 0, 1);
  point2.Normalize();
  EXPECT_FLOAT_EQ(point2.x(), 1 / sqrt(2));
  EXPECT_FLOAT_EQ(point2.y(), 0);
  EXPECT_FLOAT_EQ(point2.z(), 1 / sqrt(2));
}

TEST(LinearSlope, InitMxB) {
  Rebels::LinearSlope slope(0.5f, 2);
  EXPECT_FLOAT_EQ(slope.getSlope(), 0.5f);
  EXPECT_FLOAT_EQ(slope.getYIntercept(), 2);
}

TEST(LinearSlope, InitPoints) {
  Rebels::LinearSlope slope(0, 4, 2, 8);
  EXPECT_FLOAT_EQ(slope.getSlope(), 2);
  EXPECT_FLOAT_EQ(slope.getYIntercept(), 4);
}

TEST(LinearSlope, SetMethods) {
  Rebels::LinearSlope slope(0, 0);
  slope.setSlope(8);
  slope.setYIntercept(10);
  EXPECT_FLOAT_EQ(slope.getSlope(), 8);
  EXPECT_FLOAT_EQ(slope.getYIntercept(), 10);
}

TEST(LinearSlope, GetY) {
  Rebels::LinearSlope slope(2, 4);
  EXPECT_FLOAT_EQ(slope.CalculateY(-2), 0);
  EXPECT_FLOAT_EQ(slope.CalculateY(0), 4);
}

TEST(LinearSlope, GetX) {
  Rebels::LinearSlope slope(2, 4);
  EXPECT_FLOAT_EQ(slope.CalculateX(4), 0);
  EXPECT_FLOAT_EQ(slope.CalculateX(0), -2);
}

TEST(LinearSlope, PointTest) {
  Rebels::LinearSlope slope(2, 4);
  EXPECT_TRUE(slope.IsPointAboveLine(4, 15));
  EXPECT_FALSE(slope.IsPointBelowLine(4, 15));
  EXPECT_TRUE(slope.IsPointBelowLine(-5, -7));
  EXPECT_FALSE(slope.IsPointAboveLine(-5, -7));
}

TEST(Quaternion, SetQXYZ) {
  Rebels::Quaternion rot(1, 0.25f, 0.4f, 0.8f);
  EXPECT_FLOAT_EQ(rot.q0(), 1);
  EXPECT_FLOAT_EQ(rot.x(), 0.25f);
  EXPECT_FLOAT_EQ(rot.y(), 0.4f);
  EXPECT_FLOAT_EQ(rot.z(), 0.8f);
}

TEST(Quaternion, EqOperator) {
  Rebels::Quaternion rot1(1, 0.25f, 0.4f, 0.8f);
  Rebels::Quaternion rot2(1, 0.25f, 0.4f, 0.8f);
  Rebels::Quaternion rot3(0.25f, 0.25f, 0.4f, 0.8f);
  EXPECT_EQ(rot1, rot2);
  EXPECT_NE(rot1, rot3);
}

TEST(Quaternion, CopyQuaternion) {
  Rebels::Quaternion rot(1, 0.25f, 0.4f, 0.8f);
  Rebels::Quaternion rot2(rot);
  Rebels::Quaternion rot3 = rot;
  EXPECT_EQ(rot, rot2);
  EXPECT_EQ(rot, rot3);
}

TEST(Quaternion, Normalize) {
  Rebels::Quaternion rot(0, 0, 0, 0);
  rot.Normalize();
  EXPECT_EQ(rot, Rebels::Quaternion(1, 0, 0, 0));
  Rebels::Quaternion rot2(1, 2, 2, 4);
  rot2.Normalize();
  EXPECT_FLOAT_EQ(rot2.q0(), 0.2f);
  EXPECT_FLOAT_EQ(rot2.x(), 0.4f);
  EXPECT_FLOAT_EQ(rot2.y(), 0.4f);
  EXPECT_FLOAT_EQ(rot2.z(), 0.8f);
}