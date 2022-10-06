
#include <ros/ros.h>
#include <geometry_msgs/Pose.h>
#include <geometry_msgs/PoseStamped.h>
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

// BOND
#include <bondcpp/bond.h>


// MACRO
#include  "../include/elrond_macro.h"

//HELPER FUNCTIONS 
#include "../include/auxiliaryFunctions.h"
#include <boost/circular_buffer.hpp>
//////////  VARIABLE INITIALIZED //////////

int64_t ARM_STATUS = ARM_IDLE; // ARM_STATUS = 0 is arm idle
int64_t BASE_STATUS = BASE_IDLE;
int64_t BASE_PREV_STATUS = BASE_IDLE;
int ID_REQUESTED = 0;

geometry_msgs::PoseStamped grasp_pose_goal;
geometry_msgs::PoseStamped base_pose_goal;
geometry_msgs::PoseStamped HOME_POSE_GOAL; // Da definire

ar_track_alvar_msgs::AlvarMarkers markers_poses;
std_msgs::String pick_place;

//DATABASE FOR REQUEST AND AR TAG
boost::circular_buffer<int32_t> id_request_buffer;


// ---------- PUBLISHERS ------------------------
ros::Publisher pub_grasp_pose_goal;
ros::Publisher pub_pick_place;
ros::Publisher pub_mobile_pose_goal;
ros::Publisher pub_no_marker;
void id_callback(std_msgs::String id_request)
    {
    int i;
    ID_REQUESTED = stoi(id_request.data);
    id_request_buffer.push_back(ID_REQUESTED);

    ROS_INFO("Id requested: %d", ID_REQUESTED);
    i = fFindIdInMarkers(markers_poses, ID_REQUESTED);
    //FOUND
    if (i >= 0)
        {
        if (BASE_STATUS == BASE_IDLE)
            {
            fGetPoseFromMarker(base_pose_goal, markers_poses.markers[i].pose);
            pick_place.data = "pick";
            pub_pick_place.publish(pick_place);
            pub_mobile_pose_goal.publish(base_pose_goal);
            }
        else
            {
            //TODO DO SOMETHING
            }
        }
    else // NOT FOUND
        {
        ROS_ERROR("IL TESSSSOROOOO");
        }
    }

void artag_callback(ar_track_alvar_msgs::AlvarMarkers req)
    {
    markers_poses = req;
    }

void arm_status_callback(std_msgs::Int64 arm_status)
    {
    switch (arm_status.data)
        {
        case ARM_SUCCESS:
            if (pick_place.data.compare("pick"))
                {
                if (BASE_STATUS == BASE_IDLE)
                    {
                    pick_place.data = "place";
                    pub_mobile_pose_goal.publish(HOME_POSE_GOAL);
                    pub_pick_place.publish(pick_place);
                    }
                }
            else
                {
                id_request_buffer.pop_front();
                }
            break;
        case ARM_IDLE:
            break;
        case ARM_FAIL:
            break;
        default:
            break;
        }
    }


void base_status_idle_switchHandler()
    {
    //todo
    return;
    }
void base_status_ToGoal_switchHandler()

    {
    //todo
    return;
    }
void base_status_GoalFail_switchHandler()

    {
    //todo
    return;
    }

void base_status_GoalOk_switchHandler()
    {
    switch (ARM_STATUS)
        {
        case ARM_SUCCESS:
            break;
        case ARM_IDLE:
            fGetPoseFromMarker(grasp_pose_goal, markers_poses.markers[id_request_buffer.front()].pose);
            pub_grasp_pose_goal.publish(grasp_pose_goal);
            break;
        case ARM_FAIL:
            pub_mobile_pose_goal.publish(HOME_POSE_GOAL);
            break;
        default:
            break;
        }
    }
void base_status_callback(std_msgs::Int64 base_status)
    {
    //Need previous status to handle switching
    BASE_PREV_STATUS = BASE_STATUS;
    BASE_STATUS = base_status.data;
    if (BASE_STATUS == BASE_PREV_STATUS)
        return;
    switch (BASE_STATUS)
        {
        case BASE_IDLE:
            base_status_idle_switchHandler();
            break;
        case BASE_TO_GOAL:
            base_status_ToGoal_switchHandler();
            break;
        case BASE_GOAL_OK:
            base_status_GoalOk_switchHandler();
            break;
        case BASE_GOAL_FAIL:
            base_status_GoalFail_switchHandler();
            break;
        }
    }

int main(int argc, char** argv)
    {

    putenv((char*)"ROS_NAMESPACE=locobot");
    ros::init(argc, argv, "communication_manager");
    ros::NodeHandle node_handle;
    ros::Subscriber sub_id_request = node_handle.subscribe("/locobot/frodo/id_request", 1, id_callback);
    ros::Subscriber sub_artag = node_handle.subscribe("/locobot/move_group/ar_pose_marker", 1, artag_callback);
    ros::Subscriber sub_status_arm = node_handle.subscribe("/locobot/frodo/arm_status", 1, arm_status_callback);
    ros::Subscriber sub_status_base = node_handle.subscribe("/locobot/frodo/base_status", 1, base_status_callback);

    pub_grasp_pose_goal = node_handle.advertise<geometry_msgs::PoseStamped>("locobot/frodo/grasp_pose_goal", 1);
    pub_pick_place = node_handle.advertise<std_msgs::String>("locobot/frodo/pick_or_place", 1);
    pub_mobile_pose_goal = node_handle.advertise<geometry_msgs::PoseStamped>("locobot/frodo/mobile_pose_goal", 1);
    pub_no_marker = node_handle.advertise<std_msgs::String>("locobot/frodo/no_marker", 1);
    bond::Bond bond_pick_arm("/locobot/pick_arm", "PickArm");
    bond::Bond bond_place_arm("/locobot/place_arm", "PlaceArm");

    int i;
    int rCurrentIdRequest;
    ros::Rate loop_rate(1);
    while (ros::ok())
        {
        ros::spinOnce();
        //Wait for artag id request
        while (id_request_buffer.empty()) {}
        i = fFindIdInMarkers(markers_poses, id_request_buffer.front());
        if (i >= 0)
            {
            ROS_INFO("Lettura n%d OK, trovato id richiesto", i);
            //remove request from circular buffer
            id_request_buffer.pop_front();
            fGetPoseFromMarker(base_pose_goal, markers_poses.markers[i].pose);
            ROS_INFO("Movimento base a posizione id richiesto");
            pub_mobile_pose_goal.publish(base_pose_goal);

            }
        // BASE MOVING TO TARGET

        // BASE SEARCHING FOR TARGET

       // ARM COMMUNICATION PART PICK;
        i = fFindIdInMarkers(markers_poses, id_request_buffer.front());
        if (i >= 0)
            {
            ROS_INFO("Lettura n%d OK, trovato id richiesto", i);

            if (ARM_STATUS == 0)
                {
                fGetPoseFromMarker(grasp_pose_goal, markers_poses.markers[i].pose);
                pick_place.data = "pick";

                pub_grasp_pose_goal.publish(grasp_pose_goal);
                ros::Duration(5).sleep(); // Wait 5 seconds that the topic receives the pick command
                pub_pick_place.publish(pick_place);
                bond_pick_arm.start();
                if (!bond_pick_arm.waitUntilFormed(ros::Duration(10.0)))
                    {
                    ROS_ERROR("ERROR bond not formed!");
                    return false;
                    }
                bond_pick_arm.waitUntilBroken(ros::Duration(-1.0));
                ROS_INFO("Arm finished the pick routine");
                }
            else
                {
                ROS_INFO("Arm currently running");
                }
            }

        // BASE MOVING TO DEPOT

        // ARM COMMUNICATION PART PLACE
        if (ARM_STATUS == 0)
            {
            pick_place.data = "place";
            pub_grasp_pose_goal.publish(HOME_POSE_GOAL);
            ros::Duration(5).sleep(); // Wait 5 seconds that the topic receives the pick command
            pub_pick_place.publish(pick_place);

            bond_place_arm.start();
            if (!bond_place_arm.waitUntilFormed(ros::Duration(10.0)))
                {
                ROS_ERROR("ERROR bond not formed!");
                }
            bond_place_arm.waitUntilBroken(ros::Duration(-1.0));
            ROS_INFO("Arm finished the place routine");
            }

        loop_rate.sleep();
        }
    return 0;
    }