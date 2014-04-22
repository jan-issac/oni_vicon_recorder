
#ifndef ONI_VICON_RECORDER_VICON_RECORDER_HPP
#define ONI_VICON_RECORDER_VICON_RECORDER_HPP

#include <iostream>
#include <fstream>
#include <cassert>
#include <ctime>
#include <iomanip>

#include <vicon_sdk/vicon_client.h>

#include <actionlib/server/simple_action_server.h>

#include <oni_vicon_recorder/ConnectToViconAction.h>

/**
 * @class ViconRecorder records a subset of the vicon data
 *
 * Recorded file format
 *
 *  Each line contains data of a single frame. Each line is build up as follows
 *
 *  - (unsigned int) FrameNumber
 *  - (unsigned int) Output_GetTimecode.Hours;
 *  - (unsigned int) Output_GetTimecode.Minutes;
 *  - (unsigned int) Output_GetTimecode.Seconds;
 *  - (unsigned int) Output_GetTimecode.Frames;
 *  - (unsigned int) Output_GetTimecode.SubFrame;
 *  - (TimecodeStandard::Enum) Output_GetTimecode.Standard;
 *  - (unsigned int) Output_GetTimecode.SubFramesPerFrame;
 *  - (unsigned int) Output_GetTimecode.UserBits;
 *
 *  - (double) Output_GetSegmentGlobalTranslation.Translation[ 0 ]
 *  - (double) Output_GetSegmentGlobalTranslation.Translation[ 1 ]
 *  - (double) Output_GetSegmentGlobalTranslation.Translation[ 2 ]
 *
 *  - (double) Output_GetSegmentGlobalRotationMatrix.Rotation[ 0 ]
 *  - (double) Output_GetSegmentGlobalRotationMatrix.Rotation[ 1 ]
 *  - (double) Output_GetSegmentGlobalRotationMatrix.Rotation[ 2 ]
 *  - (double) Output_GetSegmentGlobalRotationMatrix.Rotation[ 3 ]
 *  - (double) Output_GetSegmentGlobalRotationMatrix.Rotation[ 4 ]
 *  - (double) Output_GetSegmentGlobalRotationMatrix.Rotation[ 5 ]
 *  - (double) Output_GetSegmentGlobalRotationMatrix.Rotation[ 6 ]
 *  - (double) Output_GetSegmentGlobalRotationMatrix.Rotation[ 7 ]
 *  - (double) Output_GetSegmentGlobalRotationMatrix.Rotation[ 8 ]
 *
 *  - (double) Output_GetSegmentGlobalRotationQuaternion.Rotation[ 0 ]
 *  - (double) Output_GetSegmentGlobalRotationQuaternion.Rotation[ 1 ]
 *  - (double) Output_GetSegmentGlobalRotationQuaternion.Rotation[ 2 ]
 *  - (double) Output_GetSegmentGlobalRotationQuaternion.Rotation[ 3 ]
 *
 *  - (double) Output_GetSegmentGlobalRotationEulerXYZ.Rotation[ 0 ]
 *  - (double) Output_GetSegmentGlobalRotationEulerXYZ.Rotation[ 1 ]
 *  - (double) Output_GetSegmentGlobalRotationEulerXYZ.Rotation[ 2 ]
 *  "\n"
 */
class ViconRecorder
{
public:
    ViconRecorder(ros::NodeHandle& node_handle, int float_precision = 5);
    ~ViconRecorder();

    std::ofstream& beginRecord(std::ofstream& ofs);
    std::ofstream& record(std::ofstream& ofs);
    std::ofstream& endRecord(std::ofstream& ofs);

    void connectCB(const oni_vicon_recorder::ConnectToViconGoalConstPtr& goal);

private:
    int float_precision_;

    std::string hostname_;
    std::string multicast_address_;
    std::string object_;
    bool connect_to_multicast_;
    bool multicast_enabled_;
    ViconDataStreamSDK::CPP::Client vicon_client_;

    actionlib::SimpleActionServer<
        oni_vicon_recorder::ConnectToViconAction> connect_to_vicon_as_;
};

#endif
