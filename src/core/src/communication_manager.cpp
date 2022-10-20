#include <../include/auxiliaryFunctionCommManager.h>


void id_callback(std_msgs::Int64 id_request)
    {

    ROS_INFO("[CORE::COMM_MANAGER] ---- ENTERED ID CALLBACK");
    int i;
    ID_REQUESTED = id_request.data;
    if (!pSearchingActive) {
        ROS_INFO("[CORE::COMM_MANAGER] ----ADDED ID TO BUFFER");
        id_request_buffer.push_back(ID_REQUESTED);
        }

    ROS_INFO("[CORE::COMM_MANAGER] ---- ID REQUESTED: %d", ID_REQUESTED);
    i = fFindIdInMarkers(markers_poses, ID_REQUESTED);

    // FOUND
    if (i >= 0)
        {

        ROS_INFO("[CORE::COMM_MANAGER] ---- BASE STATUS: %d", BASE_STATUS);
        if (BASE_STATUS == BASE_IDLE || pSearchingActive)
            {
            WaitOnVariable(BASE_STATUS,BASE_IDLE);
            ros::WallDuration(1).sleep();
            pick_place.data = PICK;
            ros::WallDuration(1).sleep();
            pub_pick_place.publish(pick_place);
            ros::WallDuration(1).sleep();
            MARKER_POSE_GOAL = markers_poses.markers[i].pose; //id_request_buffer.front()
            fChangeOrientation(base_pose_goal, MARKER_POSE_GOAL.pose);
            fChangePosition(base_pose_goal, MARKER_POSE_GOAL.pose, distance_base);
            pub_mobile_pose_goal.publish(base_pose_goal);
            ROS_INFO("[CORE::COMM_MANAGER] ---- ID FOUND PUBLISHING BASE GOAL");
            if (pSearchingActive)
                pSearchingActive = false;
            }
        else
            {
            ROS_INFO("[CORE::COMM_MANAGER] ---- ROBOT RUNNING, TRY LATER");
            }
        }
    else // NOT FOUND
        {
        bool rSuccess = fSearchFunction();
        if (rSuccess)
            {
            ROS_INFO("[CORE::COMM_MANAGER] ---- SEARCH SUCCESSFUL");
            id_callback(id_request);
            }
        else
            ROS_ERROR("[CORE::COMM_MANAGER] ---- ARTAG NOT FOUND!");

        }
    }

void artag_callback(ar_track_alvar_msgs::AlvarMarkers req)
    {
    markers_poses = req;
    if (pSearchingActive)
        if (fFindIdInMarkers(markers_poses, ID_REQUESTED) >= 0) {
            ROS_INFO("[CORE::COMM_MANAGER] ---- ARTAG FOR SEARCH FOUND!");
            pFoundMarker = true;
            }

    }

void arm_status_callback(std_msgs::Int64 arm_status)
    {
    ROS_INFO("[CORE::COMM_MANAGER] ---- ENTERED ARM CALLBACK");
    ARM_STATUS = arm_status.data;
    uint retry = 0;
    switch (ARM_STATUS)
        {
        case ARM_SUCCESS:
            if (pick_place.data == PICK)
                {
                WaitOnVariable(BASE_STATUS, BASE_IDLE);
                pick_place.data = PLACE;
                ROS_INFO("[CORE::COMM_MANAGER] ---- Starting PLACE");
                pub_mobile_pose_goal.publish(HOME_POSE_GOAL);
                pub_pick_place.publish(pick_place);
                retry_arm = 0;
                }
            else
                {
                id_request_buffer.pop_front();
                retry_arm = 0;

                }
            break;
        case ARM_IDLE:
            ROS_INFO("[CORE::COMM_MANAGER] ---- ARM IDLE");
            break;
        case ARM_FAIL:
            if (pick_place.data == PICK) {
                if (retry_arm > 2)
                    {
                    ROS_ERROR("[CORE::COMM_MANAGER] ---- Max retry pick reached, FAIL!");
                    distance_arm = 0.5;
                    break;
                    }
                distance_arm += 0.15;
                ROS_INFO("[CORE::COMM_MANAGER] ---- ARM FAILED PICK, Repositioning...");
                WaitOnVariable(BASE_STATUS, BASE_IDLE);
                fChangePosition(base_pose_goal, MARKER_POSE_GOAL.pose, distance_arm);
                pub_mobile_pose_goal.publish(base_pose_goal);
                retry_arm++;
                }
            else {
                if (retry_arm > 2)
                    {
                    ROS_ERROR("[CORE::COMM_MANAGER] ---- Max retry place reached, FAIL!");
                    distance_arm = 0.5;
                    break;
                    }
                distance_arm += 0.15;
                ROS_INFO("[CORE::COMM_MANAGER] ---- ARM FAILED PLACE, Repositioning...");
                WaitOnVariable(BASE_STATUS, BASE_IDLE);
                fChangePosition(base_pose_goal, HOME_POSE_GOAL.pose, distance_arm);
                pub_mobile_pose_goal.publish(base_pose_goal);
                retry_arm++;
                }
            break;

        case ARM_RUNNING:
            ROS_INFO("[CORE::COMM_MANAGER] ---- ARM RUNNING");
            break;
        default:
            break;
        }
    }

void base_status_callback(std_msgs::Int64 base_status)
    {
    ROS_INFO("[CORE::COMM_MANAGER] ---- ENTERD BASE CALLBACK");
    BASE_STATUS = base_status.data;
    switch (BASE_STATUS)
        {
        case BASE_IDLE:
            ROS_INFO("[CORE::COMM_MANAGER] ---- BASE IDLE");
            base_status_idle_switchHandler();
            break;
        case BASE_TO_GOAL:
            ROS_INFO("[CORE::COMM_MANAGER] ---- BASE TO GOAL");
            base_status_ToGoal_switchHandler();
            break;
        case BASE_GOAL_OK:
            ROS_INFO("[CORE::COMM_MANAGER] ---- BASE ARRIVED AT GOAL");
            base_status_GoalOk_switchHandler();
            break;
        case BASE_GOAL_FAIL:
            ROS_ERROR("[CORE::COMM_MANAGER] ---- BASE FAILED");
            base_status_GoalFail_switchHandler();
            break;
        }
    }

void mobileBasePositionCallback(nav_msgs::Odometry pOdometryInfo)
    {
    pMobileBasePosition = pOdometryInfo.pose.pose;
    }

int main(int argc, char** argv)
    {

    putenv((char*)"ROS_NAMESPACE=locobot");
    ros::init(argc, argv, "communication_manager");

    HOME_POSE_GOAL.header.frame_id = "map";
    HOME_POSE_GOAL.pose.position.x = 0;
    HOME_POSE_GOAL.pose.position.y = 0;
    HOME_POSE_GOAL.pose.position.z = 0;
    HOME_POSE_GOAL.pose.orientation.x = 0;
    HOME_POSE_GOAL.pose.orientation.y = 0;
    HOME_POSE_GOAL.pose.orientation.z = 0;
    HOME_POSE_GOAL.pose.orientation.w = 1;

    PLACE_GRASP_GOAL.header.frame_id = "map";
    PLACE_GRASP_GOAL.pose.position.x = 0.4;
    PLACE_GRASP_GOAL.pose.position.y = 0;
    PLACE_GRASP_GOAL.pose.position.z = 0.2;
    PLACE_GRASP_GOAL.pose.orientation.x = 0;
    PLACE_GRASP_GOAL.pose.orientation.y = 0;
    PLACE_GRASP_GOAL.pose.orientation.z = 0;
    PLACE_GRASP_GOAL.pose.orientation.w = 1;
    arm_status.data = ARM_IDLE; // Initialize arm as idle

    ros::NodeHandle node_handle;
    ros::Subscriber sub_id_request = node_handle.subscribe("/locobot/frodo/id_request", 1, id_callback);
    ros::Subscriber sub_artag = node_handle.subscribe("/locobot/move_group/ar_pose_marker", 1, artag_callback);
    ros::Subscriber sub_status_arm = node_handle.subscribe("/locobot/frodo/arm_status", 1, arm_status_callback);
    ros::Subscriber sub_status_base = node_handle.subscribe("/locobot/frodo/base_status", 1, base_status_callback);
    ros::Subscriber sub_mobile_base_position = node_handle.subscribe("/locobot/mobile_base/odom", 1, mobileBasePositionCallback);

    pub_grasp_pose_goal = node_handle.advertise<geometry_msgs::PoseStamped>("/locobot/frodo/grasp_pose_goal", 1);
    pub_pick_place = node_handle.advertise<std_msgs::Int64>("/locobot/frodo/pick_or_place", 1);
    pub_mobile_pose_goal = node_handle.advertise<geometry_msgs::PoseStamped>("/locobot/frodo/mobile_pose_goal", 1);
    pub_no_marker = node_handle.advertise<std_msgs::String>("/locobot/frodo/no_marker", 1);

    geometry_msgs::PoseStamped rTempPose;

    rTempPose.header.frame_id = "map";
    rTempPose.pose.position.x = 0.22;
    rTempPose.pose.position.y = -4.6;
    rTempPose.pose.position.z = 0.0;
    rTempPose.pose.orientation.x = 0;
    rTempPose.pose.orientation.y = 0;
    rTempPose.pose.orientation.z = 0;
    rTempPose.pose.orientation.w = 1;
    pSearchPoses.push_back(rTempPose);
    pSearchPoses.push_back(HOME_POSE_GOAL);
    rTempPose.header.frame_id = "map";
    rTempPose.pose.position.x = -3.91;
    rTempPose.pose.position.y = 4.27;
    rTempPose.pose.position.z = 0.0;
    rTempPose.pose.orientation.x = 0;
    rTempPose.pose.orientation.y = 0;
    rTempPose.pose.orientation.z = 0;
    rTempPose.pose.orientation.w = 1;
    pSearchPoses.push_back(rTempPose);
    pSearchPoses.push_back(HOME_POSE_GOAL);

    ros::spin();
    return 0;
    }
