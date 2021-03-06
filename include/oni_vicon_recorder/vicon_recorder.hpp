/*
 * Software License Agreement (BSD License)
 *
 *  Copyright (c) 2014 Max-Planck-Institute for Intelligent Systems,
 *                     University of Southern California,
 *                     Karlsruhe Institute of Technology
 *    Jan Issac (jan.issac@gmail.com)
 *
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above
 *     copyright notice, this list of conditions and the following
 *     disclaimer in the documentation and/or other materials provided
 *     with the distribution.
 *   * Neither the name of Willow Garage, Inc. nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 *  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 *  COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 *  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 *  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 *  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 *  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 *  ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 *
 */

/**
 * @date 04/14/2014
 * @author Jan Issac (jan.issac@gmail.com)
 * Max-Planck-Institute for Intelligent Systems, University of Southern California (USC),
 *   Karlsruhe Institute of Technology (KIT)
 */

#ifndef ONI_VICON_RECORDER_VICON_RECORDER_HPP
#define ONI_VICON_RECORDER_VICON_RECORDER_HPP

#include <iostream>
#include <fstream>
#include <cassert>
#include <ctime>
#include <iomanip>
#include <set>
#include <vector>

#include <boost/thread/shared_mutex.hpp>

#include <vicon_sdk/vicon_client.h>

#include <actionlib/server/simple_action_server.h>

#include <oni_vicon_recorder/ConnectToViconAction.h>

#include <oni_vicon_recorder/VerifyObjectExists.h>
#include <oni_vicon_recorder/ViconObjectPose.h>
#include <oni_vicon_recorder/ViconObjects.h>

#include "oni_vicon_recorder/frame_time_tracker.hpp"


/**
 * @class ViconRecorder records a set of the vicon data
 *
 * Recorded file format
 *
 *  Each line contains data of a single frame. Each line is build up as follows
 *
 *  - (unsigned int) Recording frame number (starting from zero)
 *  - (unsigned int) FrameNumber
 *
 *  - (double) Output_GetSegmentGlobalTranslation.Translation[ 0 ]
 *  - (double) Output_GetSegmentGlobalTranslation.Translation[ 1 ]
 *  - (double) Output_GetSegmentGlobalTranslation.Translation[ 2 ]
 *
 *  - (double) Output_GetSegmentGlobalRotationQuaternion.Rotation[ 0 ]
 *  - (double) Output_GetSegmentGlobalRotationQuaternion.Rotation[ 1 ]
 *  - (double) Output_GetSegmentGlobalRotationQuaternion.Rotation[ 2 ]
 *  - (double) Output_GetSegmentGlobalRotationQuaternion.Rotation[ 3 ]
 *
 */
class ViconRecorderStub
{
public:
    ViconRecorderStub(ros::NodeHandle& node_handle,
                      const std::string& vicon_objects_srv_name,
                      const std::string& object_verification_srv_name,
                      const std::string& vicon_frame_srv_name,
                      int float_precision = 5);
    ~ViconRecorderStub();

public: /* Action Callbacks */
    void connectCB(const oni_vicon_recorder::ConnectToViconGoalConstPtr& goal);

public: /* Service Callbacks */
    bool viconObjectsCB(oni_vicon_recorder::ViconObjects::Request& request,
                        oni_vicon_recorder::ViconObjects::Response& response);
    bool objectExistsCB(oni_vicon_recorder::VerifyObjectExists::Request& request,
                        oni_vicon_recorder::VerifyObjectExists::Response& response);
private:
    int float_precision_;
    bool connected_;
    boost::shared_mutex iteration_mutex_;

    std::string hostname_;
    std::string multicast_address_;
    std::string object_;
    bool connect_to_multicast_;
    bool multicast_enabled_;

    actionlib::SimpleActionServer<
        oni_vicon_recorder::ConnectToViconAction> connect_to_vicon_as_;

    ros::ServiceServer vicon_objects_srv_;
    ros::ServiceServer object_verification_srv_;
};

class ViconRecorder
{
public:
    ViconRecorder(
        ros::NodeHandle& node_handle,
        FrameTimeTracker::Ptr frame_time_tracker,
        int float_precision = 5);
    ~ViconRecorder();

    bool startRecording(const std::string& file, const std::string& object_name);
    bool stopRecording();
    u_int64_t countFrames() const;
    bool isRecording() const;
    void closeConnection();

    std::ofstream& beginRecord(std::ofstream& ofs);
    std::ofstream& record(std::ofstream& ofs) const;
    std::ofstream& endRecord(std::ofstream& ofs) const;

public: /* Action Callbacks */
    void connectCB(const oni_vicon_recorder::ConnectToViconGoalConstPtr& goal);

public: /* Service Callbacks */
    bool viconObjectsCB(oni_vicon_recorder::ViconObjects::Request& request,
                        oni_vicon_recorder::ViconObjects::Response& response);
    bool objectExistsCB(oni_vicon_recorder::VerifyObjectExists::Request& request,
                        oni_vicon_recorder::VerifyObjectExists::Response& response);
    bool viconObjectPose(oni_vicon_recorder::ViconObjectPose::Request& request,
                         oni_vicon_recorder::ViconObjectPose::Response& response);

    bool waitForFrame(double wait_time_in_sec = 3.);

private:
    /**
     * @brief getViconObject() Gets the set of defined objects/subjects in the vicon system
     *
     * @return objects set
     */
    std::set<std::string> getViconObjects();

    /**
     * @brief recordFrame Records current Vicon frame
     */
    bool recordFrame();

private:
    int float_precision_;
    bool connected_;
    bool recording_;
    bool first_record_;
    u_int64_t frames_;
    boost::shared_mutex iteration_mutex_;
    std::ofstream ofs_;
    std::string object_name_;

    std::string hostname_;
    std::string multicast_address_;
    std::string object_;
    bool connect_to_multicast_;
    bool multicast_enabled_;
    ViconDataStreamSDK::CPP::Client vicon_client_;

    actionlib::SimpleActionServer<
        oni_vicon_recorder::ConnectToViconAction> connect_to_vicon_as_;

    ros::ServiceServer vicon_objects_srv_;
    ros::ServiceServer vicon_object_pose_srv_;
    ros::ServiceServer object_exists_srv_;

    FrameTimeTracker::Ptr frame_time_tracker_;
};


#endif
