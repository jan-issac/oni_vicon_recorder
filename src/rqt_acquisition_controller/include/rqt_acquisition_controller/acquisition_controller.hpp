

#ifndef RQT_ACQUISITION_CONTROLLER_PLUGIN_HPP
#define RQT_ACQUISITION_CONTROLLER_PLUGIN_HPP

#include <ui_acquisition_controller.h>

#include <boost/thread/mutex.hpp>

#include <QWidget>
#include <QStandardItem>
#include <QStandardItemModel>
#include <QStringListModel>
#include <QListWidgetItem>
#include <QTreeWidgetItem>
#include <QMessageBox>

#include <ros/ros.h>
#include <rqt_gui_cpp/plugin.h>
#include <actionlib/client/simple_action_client.h>

#include <oni_vicon_recorder/ViconObjects.h>
#include <oni_vicon_recorder/VerifyObjectExists.h>

#include <oni_vicon_recorder/RecordAction.h>
#include <oni_vicon_recorder/RunDepthSensorAction.h>
#include <oni_vicon_recorder/ChangeDepthSensorModeAction.h>
#include <oni_vicon_recorder/ConnectToViconAction.h>
#include <depth_sensor_vicon_calibration/GlobalCalibrationAction.h>
#include <depth_sensor_vicon_calibration/ContinueGlobalCalibrationAction.h>

#include <ros_action_helper/action_helper.hpp>

namespace rqt_acquisition_controller
{
    class AcquisitionController:
        public rqt_gui_cpp::Plugin
    {
    Q_OBJECT

    /**
     * Groovy actionlib has a reported bug on reconnecting to an action server:
     *    @link https://github.com/ros/actionlib/issues/7
     * The Client is not able to reconnect and stay connected!
     *
     * This issue has been fixed in Hydro.
     */
    ACTION_IMPLEMENT(oni_vicon_recorder, Record)
    ACTION_IMPLEMENT(oni_vicon_recorder, RunDepthSensor)
    ACTION_IMPLEMENT(oni_vicon_recorder, ConnectToVicon)
    ACTION_IMPLEMENT(oni_vicon_recorder, ChangeDepthSensorMode)
    ACTION_IMPLEMENT(depth_sensor_vicon_calibration, GlobalCalibration)
    ACTION_IMPLEMENT(depth_sensor_vicon_calibration, ContinueGlobalCalibration)

    public:
        struct StatusItem
        {
            QStandardItem* object;
            QStandardItem* status;

            StatusItem& appendRow(StatusItem& status_item)
            {
                object->appendRow(QList<QStandardItem *>() << status_item.object
                                                           << status_item.status);

                return *this;
            }
        };

        AcquisitionController();
        virtual void initPlugin(qt_gui_cpp::PluginContext& context);
        virtual void shutdownPlugin();
        virtual void saveSettings(qt_gui_cpp::Settings& plugin_settings,
                                  qt_gui_cpp::Settings& instance_settings) const;
        virtual void restoreSettings(const qt_gui_cpp::Settings& plugin_settings,
                                     const qt_gui_cpp::Settings& instance_settings);

    private slots:
        void onStartRecording();
        void onStopRecording();
        void onStartDepthSensor();
        void onCloseDepthSensor();
        void onApplyDepthSensorMode();
        void onConnectToVicon();
        void onDisconnectFromVicon();
        void onSubmitSettings();
        void onToggleSameModel(bool single_model);
        void onUpdateStatus();
        void oUpdateFeedback(int vicon_frames, int kinect_frames, u_int64_t duration);
        void onSelectDirectory();
        void onGenerateRecordName();
        void onDetectViconObjects();
        void onStopAll();
        void onSettingsChanged(QString change);
        void onSettingsChanged(int change);
        void onSetStatusIcon(QString setting, QString url);

        void onStartGlobalCalibration();
        void onContinueGlobalCalibration();
        void onAbortGlobalCalibration();
        void onGlobalCalibrationFeedback(int progress, int max_progress, QString status);
        void onCompleteGlobalCalibration();

    signals:
        void feedbackReceived(int vicon_frames, int kinect_frames, u_int64_t duration);
        void setStatusIcon(QString setting, QString url);
        void globalCalibrationFeedback(int progress, int max_progress, QString status);

    private: /* implementation details */               
        bool box(QString message, bool rval = false, QMessageBox::Icon type = QMessageBox::Warning);
        void setDepthSensorClosedStatus();

        bool validateSettings();
        bool validateObjectName(const QString &style_error);
        bool validateRecordingDirectory(const QString &style_error);
        bool validateRecordName(const QString &style_error);
        bool validateModelLocation(const QString &style_error);
        bool validateDisplayModelFile(const QString &style_error);
        bool validateTrackingModelFile(const QString &style_error);

        // fluent interface to simplify things
        StatusItem& statusItem(std::string item_name);
        StatusItem& statusTreeRoot(QStandardItem *root);
        QList<QStandardItem*> statusRow(std::string item_name);
        StatusItem& createStatusRow(std::string object_text, std::string status_text = "");

        void setActivity(std::string section_name, bool active);
        bool isActive(std::string section_name);

        void ensureStateConsistency(bool sensor_node_online,
                                    bool vicon_node_online,
                                    bool recorder_node_online);

    private:
        Ui::AcquisitionController ui_;
        QWidget* widget_;

        QIcon empty_icon_;

        QTimer* timer_;
        QStandardItemModel* status_model_;                
        std::map<std::string, StatusItem> status_tree_container_;

        std::map<std::string, bool> activity_status_map_;

        ros::ServiceClient vicon_object_sc_;

        std::string object_model_dir_;
        std::string object_model_display_file_;
        std::string object_model_tracking_file_;
    };
}

#endif
