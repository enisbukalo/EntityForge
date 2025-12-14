#include <gtest/gtest.h>

#include <CInputController.h>

using namespace Components;

class CInputControllerTest : public ::testing::Test
{
protected:
    CInputController controller;
};

TEST_F(CInputControllerTest, IsActionActiveReturnsFalseForUnknownAction)
{
    EXPECT_FALSE(controller.isActionActive("NonExistentAction"));
}

TEST_F(CInputControllerTest, IsActionActiveReturnsFalseForNoneState)
{
    controller.actionStates["TestAction"] = ActionState::None;
    EXPECT_FALSE(controller.isActionActive("TestAction"));
}

TEST_F(CInputControllerTest, IsActionActiveReturnsFalseForReleasedState)
{
    controller.actionStates["TestAction"] = ActionState::Released;
    EXPECT_FALSE(controller.isActionActive("TestAction"));
}

TEST_F(CInputControllerTest, IsActionActiveReturnsTrueForPressedState)
{
    controller.actionStates["TestAction"] = ActionState::Pressed;
    EXPECT_TRUE(controller.isActionActive("TestAction"));
}

TEST_F(CInputControllerTest, IsActionActiveReturnsTrueForHeldState)
{
    controller.actionStates["TestAction"] = ActionState::Held;
    EXPECT_TRUE(controller.isActionActive("TestAction"));
}

TEST_F(CInputControllerTest, IsActionActiveWorksWithMultipleActions)
{
    controller.actionStates["MoveForward"]  = ActionState::Pressed;
    controller.actionStates["MoveBackward"] = ActionState::None;
    controller.actionStates["Jump"]         = ActionState::Held;
    controller.actionStates["Fire"]         = ActionState::Released;

    EXPECT_TRUE(controller.isActionActive("MoveForward"));
    EXPECT_FALSE(controller.isActionActive("MoveBackward"));
    EXPECT_TRUE(controller.isActionActive("Jump"));
    EXPECT_FALSE(controller.isActionActive("Fire"));
}
