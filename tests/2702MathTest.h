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

TEST(Point3d, Copy){
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

TEST(LinearSlope, InitMxB){
    Rebels::LinearSlope slope(0.5, 2);
    EXPECT_FLOAT_EQ(slope.getSlope(), 0.5);
    EXPECT_FLOAT_EQ(slope.getYIntercept(), 2);
}

TEST(LinearSlope, InitPoints){
    Rebels::LinearSlope slope(0, 4, 2, 8);
    EXPECT_FLOAT_EQ(slope.getSlope(), 2);
    EXPECT_FLOAT_EQ(slope.getYIntercept(), 4);
}

TEST(LinearSlope, SetMethods){
    Rebels::LinearSlope slope(0, 0);
    slope.setSlope(8);
    slope.setYIntercept(10);
    EXPECT_FLOAT_EQ(slope.getSlope(), 8);
    EXPECT_FLOAT_EQ(slope.getYIntercept(), 10);
}

TEST(LinearSlope, GetY){
    Rebels::LinearSlope slope(2, 4);
    EXPECT_FLOAT_EQ(slope.CalculateY(-2), 0);
    EXPECT_FLOAT_EQ(slope.CalculateY(0), 4);
}

TEST(LinearSlope, GetX){
    Rebels::LinearSlope(2, 4);
    EXPECT_FLOAT_EQ(slope.CalculateX(4), 0);
    EXPECT_FLOAT_EQ(slope.CalculateX(0), -2);
}

TEST(LinearSlope, PointTest){
    Rebels::LinearSlope slope(2, 4);
    EXPECT_TRUE(slope.IsPointAboveLine(4, 15));
    EXPECT_FALSE(slope.IsPointBelowLine(4, 15));
    EXPECT_TRUE(slope.IsPointBelowLine(-5, -7));
    EXPECT_FALSE(slope.IsPointAboveLine(-5, -7));
}

TEST(Quaternion, SetQXYZ){
    Rebels::Quaternion rot(1, 0.25, 0.4, 0.8);
    EXPECT_FLOAT_EQ(rot.q0(), 1);
    EXPECT_FLOAT_EQ(rot.x(), 0.25);
    EXPECT_FLOAT_EQ(rot.y(), 0.4);
    EXPECT_FLOAT_EQ(rot.z(), 0.8);
}

TEST(Quaternion, EqOperator){
    Rebels::Quaternion rot1(1, 0.25, 0.4, 0.8);
    Rebels::Quaternion rot2(1, 0.25, 0.4, 0.8);
    Rebels::Quaternion rot3(0.25, 0.25, 0.4, 0.8);
    EXPECT_EQ(rot1, rot2);
    EXPECT_NE(rot1, rot3);
}

TEST(Quaternion, CopyQuaternion){
    Rebels::Quaternion rot(1, 0.25, 0.4, 0.8);
    Rebels::Quaternion rot2(rot);
    Rebels::Quaternion rot3 = rot;
    EXPECT_EQ(rot, rot2);
    EXPECT_EQ(rot, rot3);
}

TEST(Quaternion, Normalize){
    Rebels::Quaternion rot(0, 0, 0, 0);
    rot.Normalize();
    EXPECT_EQ(rot, Rebels::Quaternion(1, 0, 0, 0));
    Rebels::Quaternion rot2(1, 2, 2, 4);
    EXPECT_FLOAT_EQ(rot2.q0(), 1/5);
    EXPECT_FLOAT_EQ(rot2.x(), 2/5);
    EXPECT_FLOAT_EQ(rot2.y(), 2/5);
    EXPECT_FLOAT_EQ(rot2.z(), 4/5);
}

TEST(Quaternion, QuatFromAxisAngle){
    Rebels::Point3d axis(90, 120, 0);
    Rebels::Quaternion rot(5, axis);
    EXPECT_FLOAT_EQ(rot.q0(), -0.8011436);
    EXPECT_FLOAT_EQ(rot.x(), 0.3590833);
    EXPECT_FLOAT_EQ(rot.y(), 0.4787777);
    EXPECT_FLOAT_EQ(rot.z(), 0);
}

TEST(Quaternion, RotatePoint){
    Rebels::Point3d pt(3, 4, 5);
    // x = 180, y = 0, z = -90 in euler angles
    Rebels::Quaternion rot(0.7071068, 0.7071068, 0, 0);
    Rebels::Point3d rotatedPt = rot * pt;
    EXPECT_FLOAT_EQ(rotatedPt.x(), 4);
    EXPECT_FLOAT_EQ(rotatedPt.y(), 3);
    EXPECT_FLOAT_EQ(rotatedPt.z(), -5);
}

TEST(RotationMatrix, RotatePoint){
    Rebels::Point3d pt(3, 4, 5);
    // x = 180, y = 0, z = -90 in euler angles
    Rebels::Quaternion rot(0.7071068, 0.7071068, 0, 0);
    Rebels::RotationMatrix matrix(rot);
    Rebels::Point3d rotatedPt = matrix * pt;
    EXPECT_FLOAT_EQ(rotatedPt.x(), 4);
    EXPECT_FLOAT_EQ(rotatedPt.y(), 3);
    EXPECT_FLOAT_EQ(rotatedPt.z(), -5);
}