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

#include <boost/bind.hpp>

#include <oni_vicon_recorder/oni_recorder.hpp>

#define CHECK_RC(rc, what)									    \
if (rc != XN_STATUS_OK)											\
{																\
    ROS_ERROR("%s failed: %s\n", what, xnGetStatusString(rc));  \
    return false;												\
}

using namespace oni_vicon_recorder;

OniRecorder::OniRecorder(ros::NodeHandle& node_handle, FrameTimeTracker::Ptr frame_time_tracker):
    node_handle_(node_handle),
    recording_(false),
    running_(false),
    run_depth_sensor_as_(node_handle,
                     RunDepthSensorGoal::ACTION_NAME,
                     boost::bind(&OniRecorder::runDepthSensorCB, this, _1),
                     false),
    change_depth_sensor_mode_as_(node_handle,
                     ChangeDepthSensorModeGoal::ACTION_NAME,
                     boost::bind(&OniRecorder::changeDeptSensorModeCB, this, _1),
                     false),
    frame_time_tracker_(frame_time_tracker)
{
    run_depth_sensor_as_.start();
    change_depth_sensor_mode_as_.start();
}

OniRecorder::~OniRecorder()
{

}

void OniRecorder::changeDeptSensorModeCB(
        const oni_vicon_recorder::ChangeDepthSensorModeGoalConstPtr& goal)
{
    ChangeDepthSensorModeResult result;    

    if (running_)
    {
        if (!recording_)
        {
            if (modes_.find(goal->mode) != modes_.end())
            {
                boost::upgrade_lock<boost::shared_mutex> lock(frameLock_);
                boost::upgrade_to_unique_lock<boost::shared_mutex> unique_lock(lock);

                XnMapOutputMode currentMode;
                rgbd_capture_.device()->g_depth->GetMapOutputMode(currentMode);

                if (currentMode.nXRes != modes_[goal->mode].nXRes)
                {
                    result.message = "Changeing resolution while publishing pointcloud not" \
                                     " supported.";
                    change_depth_sensor_mode_as_.setAborted(result);
                    ROS_WARN("%s", result.message.c_str());
                    return;
                }

                rgbd_capture_.device()->g_depth->SetMapOutputMode(modes_[goal->mode]);
                result.message = "Depth sensor mode changed to: " + goal->mode;
                change_depth_sensor_mode_as_.setSucceeded(result);
                ROS_INFO("%s", result.message.c_str());
                return;
            }
            else
            {
                result.message = "Unsupported mode: " + goal->mode;
            }
        }
        else
        {
            result.message = "Cannot change sensor mode. Device is recording.";
        }
    }
    else
    {
        result.message = "Cannot change sensor mode. Device is not running.";
    }

    change_depth_sensor_mode_as_.setAborted(result);
    ROS_WARN("%s", result.message.c_str());
}

void OniRecorder::runDepthSensorCB(const RunDepthSensorGoalConstPtr &goal)
{    
    ROS_INFO("Starting depth sensor");

    RunDepthSensorFeedback feedback;
    RunDepthSensorResult result;

    if(!rgbd_capture_.initDevice())
    {
        run_depth_sensor_as_.setAborted(result);

        ROS_INFO("Cannot start depth sensor");
        return;
    }

    ROS_INFO("Depth sensor started");    

    if (modes_.empty())
    {
        // calling this more than once causes a segmentation fault
        modes_ = getSupportedModes(rgbd_capture_.device()->g_depth);
    }

    feedback.device_type = rgbd_capture_.device()->vendor_id == XTION_VENDOR_ID ? "XTION": "Kinect";
    feedback.device_name = rgbd_capture_.device()->device_name;
    feedback.mode = getModeName(getCurrentMode(rgbd_capture_.device()->g_depth));
    feedback.modes = getSupportedModeList(modes_);
    run_depth_sensor_as_.publishFeedback(feedback);    

    running_ = true;

    while (true)
    {
        boost::shared_lock<boost::shared_mutex> lock(frameLock_);

        if (run_depth_sensor_as_.isPreemptRequested())
        {
            break;
        }
        else if (!ros::ok())
        {
            closeDevice();
            running_ = false;
            run_depth_sensor_as_.setAborted(result);
            ROS_WARN("Running depth sensor aborted. ROS shutting down.");
            return;
        }

        if(!rgbd_capture_.process())
        {
            ROS_ERROR("Running depth sensor aborted. Error during capture.");
            closeDevice();
            running_ = false;
            recording_ = false;
            run_depth_sensor_as_.setAborted(result);
            return;
        }

        if (recording_)
        {
            frame_time_tracker_->depthSensorFrame(rgbd_capture_.currentDepthFrameID(),
                                                  rgbd_capture_.currentDepthFrameTime());
            frames_++;
        }
    }

    ROS_INFO("Closing depth sensor");
    closeDevice();
    running_ = false;
    run_depth_sensor_as_.setSucceeded(result);
}

bool OniRecorder::closeDevice()
{
    rgbd_capture_.closeCamera();

    ROS_INFO("Depth sensor closed");
}

bool OniRecorder::startRecording(std::string destinationFile)
{
    boost::upgrade_lock<boost::shared_mutex> lock(frameLock_);
    boost::upgrade_to_unique_lock<boost::shared_mutex> unique_lock(lock);

    if (!running_)
    {
        ROS_WARN("Cannot start recording. Depth sensor is not running.");
        return false;
    }

    recorder_ = Recorder();

    ROS_INFO("Starting ONI recording");
    ROS_INFO(" - output file: %s", destinationFile.c_str());

    CHECK_RC(recorder_.Create(*rgbd_capture_.device()->g_context),
             "Create NX Recorder");

    CHECK_RC(recorder_.SetDestination(XN_RECORD_MEDIUM_FILE, destinationFile.c_str()),
             "Set recorder destination file");

    CHECK_RC(recorder_.AddNodeToRecording(*rgbd_capture_.device()->g_depth),
             "Add depth generator node to recording");

    recording_ = true;
    frames_ = 0;

    ROS_INFO("Depth data recording started");

    return true;
}

bool OniRecorder::stopRecording()
{
    boost::upgrade_lock<boost::shared_mutex> lock(frameLock_);
    boost::upgrade_to_unique_lock<boost::shared_mutex> unique_lock(lock);

    if (!recording_)
    {
        ROS_WARN("Recorder is not running.");
        return false;
    }

    CHECK_RC(recorder_.RemoveNodeFromRecording(*rgbd_capture_.device()->g_depth),
             "Remove depth generator node from recording");

    recorder_.Release();
    ROS_INFO("Recording depth images stopped");

    recording_ = false;

    return true;
}

u_int64_t OniRecorder::countFrames()
{
    boost::upgrade_lock<boost::shared_mutex> lock(frameLock_);
    boost::upgrade_to_unique_lock<boost::shared_mutex> unique_lock(lock);

    return frames_;
}

bool OniRecorder::isRecording() const
{
    return recording_;
}

std::map<std::string, XnMapOutputMode> OniRecorder::getSupportedModes(
        const DepthGenerator* generator) const
{
    std::map<std::string, XnMapOutputMode> mode_map;

    XnUInt32 modeCount = generator->GetSupportedMapOutputModesCount();
    XnMapOutputMode* modes = new XnMapOutputMode[modeCount];
    generator->GetSupportedMapOutputModes(modes, modeCount);

    std::string modeName;
    for (int i = 0; i < modeCount; ++i)
    {
        modeName = getModeName(modes[i]);

        mode_map[modeName] = modes[i];
    }

    delete modes;

    return mode_map;
}

std::vector<std::string> OniRecorder::getSupportedModeList(
        const std::map<std::string, XnMapOutputMode>& mode_map) const
{
    std::vector<std::string> mode_list;
    std::map<std::string, XnMapOutputMode>::const_iterator it = mode_map.begin();
    while (it != mode_map.end())
    {
        mode_list.push_back(it->first);
        ++it;
    }

    return mode_list;
}

std::string OniRecorder::getModeName(const XnMapOutputMode &mode) const
{
    std::stringstream modeNameStream;

    switch (mode.nXRes)
    {
    case 160:  modeNameStream << "QQVGA (160x120)"; break;
    case 320:  modeNameStream << "QVGA (320x240)"; break;
    case 640:  modeNameStream << "VGA (640x480)"; break;
    case 1280: modeNameStream << "XVGA (1280x960)"; break;

    default:
        modeNameStream << mode.nXRes << "x" <<mode.nYRes;
    }

    modeNameStream << ", " << mode.nFPS << " FPS";

    return modeNameStream.str();
}


XnMapOutputMode OniRecorder::getCurrentMode(const DepthGenerator* generator) const
{
    XnMapOutputMode mode;
    generator->GetMapOutputMode(mode);

    return mode;
}
