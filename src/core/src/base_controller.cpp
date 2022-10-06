#include <ros/ros.h>
#include <geometry_msgs/Pose.h>
#include <std_msgs/String.h>
#include <std_msgs/Int64.h>
#include <iostream>
#include <string>
// TF2
#include <tf2_geometry_msgs/tf2_geometry_msgs.h>

// Ar_track_alvar
#include <tf/transform_datatypes.h>
#include <ar_track_alvar_msgs/AlvarMarkers.h>

// Move base
#include <move_base_msgs/MoveBaseAction.h>
#include <actionlib/client/simple_action_client.h>


//frodo costants
#include "../include/elrond_macro.h"

ros::Publisher pub_status;
ros::Publisher goal_pub;
typedef actionlib::SimpleActionClient<move_base_msgs::MoveBaseAction> MoveBaseClient;

std_msgs::Int64 base_status_msg;

bool wait;
bool success_navigation;
//Handles simple case of goal pose from Communication Manacer
void moveBaseCallback(geometry_msgs::PoseStamped pPose)
    {
    actionlib::SimpleClientGoalState rResult = actionlib::SimpleClientGoalState::ABORTED;
    move_base_msgs::MoveBaseActionGoal rGoal;

    MoveBaseClient client("/locobot/move_base", true);
    rGoal.goal.target_pose = pPose;

    client.waitForServer();
    client.sendGoal(rGoal.goal);
    base_status_msg.data = BASE_TO_GOAL;
    pub_status.publish(base_status_msg);
    wait = client.waitForResult();
    if (wait)
        {
        rResult = client.getState();
        }
    else
        {
        ROS_ERROR("Action server not available!");
        ros::requestShutdown();
        }

    if (rResult == actionlib::SimpleClientGoalState::SUCCEEDED)
        {
        ROS_INFO("Goal execution done!");
        base_status_msg.data = BASE_GOAL_OK;
        pub_status.publish(base_status_msg);
        }
    else {
        ROS_INFO("Goal not reached!");
        base_status_msg.data = BASE_GOAL_FAIL;
        pub_status.publish(base_status_msg);
        }

    }
int main(int argc, char** argv)
    {
    putenv((char*)"ROS_NAMESPACE=locobot");
    ros::init(argc, argv, "base_controller");
    ros::NodeHandle node_handle;
    ros::Subscriber sub_mobile_goal_pose = node_handle.subscribe("/locobot/frodo/mobile_goal_pose", 1, moveBaseCallback);
    pub_status = node_handle.advertise<std_msgs::Int64>("locobot/frodo/base_status", 1);
    ros::Rate loop_rate(1);
    ros::spin();
    }