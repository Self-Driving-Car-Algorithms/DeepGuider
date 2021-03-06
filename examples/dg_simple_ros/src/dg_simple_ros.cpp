#include <ros/ros.h>
#include <sensor_msgs/Image.h>
#include <sensor_msgs/NavSatStatus.h>
#include <sensor_msgs/NavSatFix.h>
#include <sensor_msgs/Imu.h>
#include <cv_bridge/cv_bridge.h>
#include <thread>
#include "dg_simple_ros/ocr_info.h"
#include "dg_simple.cpp"

class DeepGuiderROS : public DeepGuider
{
public:
    DeepGuiderROS(ros::NodeHandle& nh);
    virtual ~DeepGuiderROS();

    bool initialize();
    int run();
    bool runOnce(double timestamp);

protected:
    double m_wait_sec = 0.01;

    // DeepGuider Topic subscribers
    ros::Subscriber sub_ocr;
    void callbackOCR(const dg_simple_ros::ocr_info::ConstPtr& msg);

    // Topic subscribers (sensor data)
    ros::Subscriber sub_image_webcam;
    ros::Subscriber sub_image_realsense_image;
    ros::Subscriber sub_image_realsense_depth;
    ros::Subscriber sub_gps_asen;
    ros::Subscriber sub_gps_novatel;
    ros::Subscriber sub_imu_xsense;

    // Subscriber callbacks (sensor data)    
    void callbackImage(const sensor_msgs::Image::ConstPtr& msg);
    void callbackImageCompressed(const sensor_msgs::CompressedImageConstPtr& msg);
    void callbackRealsenseImage(const sensor_msgs::CompressedImageConstPtr& msg);
    void callbackRealsenseDepth(const sensor_msgs::CompressedImageConstPtr& msg);
    void callbackGPSAsen(const sensor_msgs::NavSatFixConstPtr& fix);
    void callbackGPSNovatel(const sensor_msgs::NavSatFixConstPtr& fix);
    void callbackIMU(const sensor_msgs::Imu::ConstPtr& msg);

    // Topic publishers
    //ros::Publisher pub_image_gui;

    // A node handler
    ros::NodeHandle& nh_dg;

    // timestamp to framenumber converter (utility function)
    int t2f_n = 0;
    double t2f_offset_fn = 0;
    double t2f_scale = 0;
    double t2f_offset_ts = 0;
    dg::Timestamp t2f_ts;
    int t2f_fn;
    void updateTimestamp2Framenumber(dg::Timestamp ts, int fn);
    int timestamp2Framenumber(dg::Timestamp ts);
};


DeepGuiderROS::DeepGuiderROS(ros::NodeHandle& nh) : nh_dg(nh)
{
    // overwrite configuable parameters of base class
    m_enable_roadtheta = false;
    m_enable_vps = false;
    m_enable_ocr = false;
    m_enable_logo = false;
    m_enable_intersection = false;
    m_enable_exploration = false;

    //m_server_ip = "127.0.0.1";        // default: 127.0.0.1 (localhost)
    m_server_ip = "129.254.87.96";      // default: 127.0.0.1 (localhost)
    m_threaded_run_python = true;
    m_srcdir = "/work/deepguider/src";   // system path of deepguider/src (required for python embedding)

    m_data_logging = false;
    m_enable_tts = false;
    m_recording = false;
    m_recording_fps = 30;
    m_map_image_path = "data/NaverMap_ETRI(Satellite)_191127.png";
    m_recording_header_name = "dg_ros_";

    // Read ros-specific parameters
    m_wait_sec = 0.1;
    nh_dg.param<double>("wait_sec", m_wait_sec, m_wait_sec);

    // Initialize deepguider subscribers
    sub_ocr = nh_dg.subscribe("/dg_ocr/output", 1, &DeepGuiderROS::callbackOCR, this);

    // Initialize sensor subscribers
    sub_image_webcam = nh_dg.subscribe("/uvc_image_raw/compressed", 1, &DeepGuiderROS::callbackImageCompressed, this);
    sub_gps_asen = nh_dg.subscribe("/asen_fix", 1, &DeepGuiderROS::callbackGPSAsen, this);
    sub_gps_novatel = nh_dg.subscribe("/novatel_fix", 1, &DeepGuiderROS::callbackGPSNovatel, this);
    sub_imu_xsense = nh_dg.subscribe("/imu/data", 1, &DeepGuiderROS::callbackIMU, this);
    sub_image_realsense_image = nh_dg.subscribe("/camera/color/image_raw/compressed", 1, &DeepGuiderROS::callbackRealsenseImage, this);
    sub_image_realsense_depth = nh_dg.subscribe("/camera/depth/image_rect_raw/compressed", 1, &DeepGuiderROS::callbackRealsenseDepth, this);

    // Initialize publishers
    //pub_image_gui = nh_dg.advertise<sensor_msgs::CompressedImage>("dg_image_gui", 1, true);
}

DeepGuiderROS::~DeepGuiderROS()
{    
}

bool DeepGuiderROS::initialize()
{
    bool ok = DeepGuider::initialize("dg_ros.yml");
    return ok;
}

int DeepGuiderROS::run()
{
    printf("Run deepguider system...\n");

    // start recognizer threads
    if (m_enable_vps) vps_thread = new std::thread(threadfunc_vps, this);
    if (m_enable_ocr) ocr_thread = new std::thread(threadfunc_ocr, this);    
    if (m_enable_logo) logo_thread = new std::thread(threadfunc_logo, this);
    if (m_enable_intersection) intersection_thread = new std::thread(threadfunc_intersection, this);
    if (m_enable_roadtheta) roadtheta_thread = new std::thread(threadfunc_roadtheta, this);

    // run main loop
    ros::Rate loop(1 / m_wait_sec);
    while (ros::ok())
    {
        ros::Time timestamp = ros::Time::now();
        if (!runOnce(timestamp.toSec())) break;
        ros::spinOnce();
        loop.sleep();
    }

    // end system
    printf("End deepguider system...\n");
    terminateThreadFunctions();
    printf("\tthread terminated\n");
    if(m_recording) m_video_gui.release();
    if(m_data_logging) m_video_cam.release();
    printf("\tclose recording\n");
    cv::destroyWindow(m_winname);
    printf("\tgui window destroyed\n");
    nh_dg.shutdown();
    printf("\tros shutdowned\n");
    printf("all done!\n");

    return 0;
}

bool DeepGuiderROS::runOnce(double timestamp)
{
    // process Guidance
    procGuidance(timestamp);

    // draw GUI display
    cv::Mat gui_image = m_map_image.clone();
    drawGuiDisplay(gui_image);

    // recording
    if (m_recording) m_video_gui << gui_image;

    cv::imshow(m_winname, gui_image);
    int key = cv::waitKey(1);
    if (key == cx::KEY_SPACE) key = cv::waitKey(0);
    if (key == cx::KEY_ESC) return false;

    return true;
}

// A callback function for subscribing a RGB image
void DeepGuiderROS::callbackImage(const sensor_msgs::Image::ConstPtr& msg)
{
    ROS_INFO_THROTTLE(1.0, "RGB image (timestamp: %f [sec]).", msg->header.stamp.toSec());
    cv_bridge::CvImagePtr image_ptr;
    cv::Mat image;
    try
    {
        image_ptr = cv_bridge::toCvCopy(msg);
        cv::cvtColor(image_ptr->image, image, cv::COLOR_RGB2BGR);
    }
    catch (cv_bridge::Exception& e)
    {
        ROS_ERROR("cv_bridge exception @ callbackImage(): %s", e.what());
        return;
    }
}

// A callback function for subscribing a compressed RGB image
void DeepGuiderROS::callbackImageCompressed(const sensor_msgs::CompressedImageConstPtr& msg)
{
    ROS_INFO_THROTTLE(1.0, "Compressed RGB(timestamp: %f [sec]).", msg->header.stamp.toSec());
    cv_bridge::CvImagePtr image_ptr;
    try
    {
        cv::Mat logging_image;
        m_cam_mutex.lock();
        m_cam_image = cv::imdecode(cv::Mat(msg->data), 1);//convert compressed image data to cv::Mat
        m_cam_capture_time = msg->header.stamp.toSec();
        m_cam_gps = m_localizer.getPoseGPS();
        m_cam_fnumber++;
        if (m_data_logging) logging_image = m_cam_image.clone();
        m_cam_mutex.unlock();

        updateTimestamp2Framenumber(m_cam_capture_time, m_cam_fnumber);

        if (m_data_logging)
        {
            m_video_cam << logging_image;
        }
    }
    catch (cv_bridge::Exception& e)
    {
        ROS_ERROR("exception @ callbackImageCompressed(): %s", e.what());
        return;
    }
}

// A callback function for Realsense compressed RGB
void DeepGuiderROS::callbackRealsenseImage(const sensor_msgs::CompressedImageConstPtr& msg)
{
    ROS_INFO_THROTTLE(1.0, "Realsense: RGB (timestamp=%f)", msg->header.stamp.toSec());
    cv::Mat image;
    try
    {
        cv::Mat image = cv::imdecode(cv::Mat(msg->data), 1);//convert compressed image data to cv::Mat
    }
    catch (cv_bridge::Exception& e)
    {
        ROS_ERROR("exception @ callbackRealsenseImage(): %s", e.what());
        return;
    }
}

// A callback function for Realsense compressed Depth
void DeepGuiderROS::callbackRealsenseDepth(const sensor_msgs::CompressedImageConstPtr& msg)
{
    ROS_INFO_THROTTLE(1.0, "Realsense: Depth (timestamp=%f)", msg->header.stamp.toSec());
    cv::Mat image;
    try
    {
        cv::Mat image = cv::imdecode(cv::Mat(msg->data), 1);//convert compressed image data to cv::Mat
    }
    catch (cv_bridge::Exception& e)
    {
        ROS_ERROR("exception @ callbackRealsenseDepth(): %s", e.what());
        return;
    }
}

// A callback function for subscribing GPS Asen
void DeepGuiderROS::callbackGPSAsen(const sensor_msgs::NavSatFixConstPtr& fix)
{
    //ROS_INFO_THROTTLE(1.0, "GPS Asen is subscribed (timestamp: %f [sec]).", fix->header.stamp.toSec());

    if (fix->status.status == sensor_msgs::NavSatStatus::STATUS_NO_FIX) {
        ROS_DEBUG_THROTTLE(60, "Asen: No fix.");
        return;
    }

    if (fix->header.stamp == ros::Time(0)) {
        return;
    }

    double lat = fix->latitude;
    double lon = fix->longitude;
    ROS_INFO_THROTTLE(1.0, "GPS Asen: lat=%f, lon=%f", lat, lon);

    // apply & draw gps
    const dg::LatLon gps_datum(lat, lon);
    const dg::Timestamp gps_time = fix->header.stamp.toSec();
    if (!m_use_high_gps) procGpsData(gps_datum, gps_time);
    m_painter.drawNode(m_map_image, m_map_info, gps_datum, 2, 0, cv::Vec3b(0, 255, 0));
    m_gps_history_asen.push_back(gps_datum);
}

// A callback function for subscribing GPS Novatel
void DeepGuiderROS::callbackGPSNovatel(const sensor_msgs::NavSatFixConstPtr& fix)
{
    //ROS_INFO_THROTTLE(1.0, "GPS Novatel is subscribed (timestamp: %f [sec]).", fix->header.stamp.toSec());

    if (fix->status.status == sensor_msgs::NavSatStatus::STATUS_NO_FIX) {
        ROS_DEBUG_THROTTLE(60, "Novatel: No fix.");
        return;
    }

    if (fix->header.stamp == ros::Time(0)) {
        return;
    }

    double lat = fix->latitude;
    double lon = fix->longitude;
    ROS_INFO_THROTTLE(1.0, "GPS Novatel: lat=%f, lon=%f", lat, lon);

    // apply & draw gps
    const dg::LatLon gps_datum(lat, lon);
    const dg::Timestamp gps_time = fix->header.stamp.toSec();
    if (m_use_high_gps) procGpsData(gps_datum, gps_time);
    m_painter.drawNode(m_map_image, m_map_info, gps_datum, 2, 0, cv::Vec3b(0, 0, 255));
    m_gps_history_novatel.push_back(gps_datum);
}

// A callback function for subscribing IMU
void DeepGuiderROS::callbackIMU(const sensor_msgs::Imu::ConstPtr& msg)
{
    int seq = msg->header.seq;
    double ori_x = msg->orientation.x;
    double ori_y = msg->orientation.y;
    double ori_z = msg->orientation.z;
    double angvel_x = msg->angular_velocity.x;
    double angvel_y = msg->angular_velocity.y;
    double angvel_z = msg->angular_velocity.z;
    double linacc_x = msg->linear_acceleration.x;
    double linacc_y = msg->linear_acceleration.y;
    double linacc_z = msg->linear_acceleration.z;

    ROS_INFO_THROTTLE(1.0, "IMU: seq=%d, orientation=(%f,%f,%f), angular_veloctiy=(%f,%f,%f), linear_acceleration=(%f,%f,%f)", seq, ori_x, ori_y, ori_z, angvel_x, angvel_y, angvel_z, linacc_x, linacc_y, linacc_z);
}

// A callback function for subscribing OCR output
void DeepGuiderROS::callbackOCR(const dg_simple_ros::ocr_info::ConstPtr& msg)
{
    std::vector<OCRResult> ocrs;
    for(int i = 0; i<(int)msg->ocrs.size(); i++)
    {
        OCRResult ocr;
        ocr.label = msg->ocrs[i].label;
        ocr.xmin = msg->ocrs[i].xmin;
        ocr.ymin = msg->ocrs[i].ymin;
        ocr.xmax = msg->ocrs[i].xmax;
        ocr.ymax = msg->ocrs[i].ymax;
        ocr.confidence = msg->ocrs[i].confidence;

        ocrs.push_back(ocr);
    }
    dg::Timestamp ts = msg->timestamp;
    double proc_time = msg->processingtime;
    int cam_fnumber = timestamp2Framenumber(ts);

    if (!ocrs.empty() && cam_fnumber>=0)
    {
        m_ocr.set(ocrs, ts, proc_time);

        if (m_data_logging)
        {
            m_log_mutex.lock();
            m_ocr.write(m_log, cam_fnumber);
            m_log_mutex.unlock();
        }
        m_ocr.print();

        std::vector<dg::ID> ids;
        std::vector<Polar2> obs;
        std::vector<double> confs;
        std::vector<OCRResult> ocrs;
        for (int k = 0; k < (int)ocrs.size(); k++)
        {
            dg::ID ocr_id = 0;
            std::vector<dg::POI> pois = m_map_manager.getPOI(ocrs[k].label);
            if(!pois.empty()) ocr_id = pois[0].id;
            ids.push_back(ocr_id);
            obs.push_back(rel_pose_defualt);
            confs.push_back(ocrs[k].confidence);
        }
        m_localizer_mutex.lock();
        VVS_CHECK_TRUE(m_localizer.applyLocClue(ids, obs, ts, confs));
        m_localizer_mutex.unlock();
    }
}

void DeepGuiderROS::updateTimestamp2Framenumber(dg::Timestamp ts, int fn)
{
    if(t2f_n>1)
    {
        double scale = (fn - t2f_fn) / (ts - t2f_ts);
        t2f_scale = t2f_scale * 0.9 + scale * 0.1;

        double fn_est = (ts - t2f_offset_ts)*t2f_scale + t2f_offset_fn;
        double est_err = fn - fn_est;
        t2f_offset_fn = t2f_offset_fn + est_err;

        t2f_ts = ts;
        t2f_fn = fn;
        t2f_n++;
        //int fn_est2 = timestamp2Framenumber(ts);
        //printf("[timestamp=%d] err=%.1lf, fn=%d, fn_est=%d", t2f_n, est_err, fn, fn_est2);
        return;
    }
    if(t2f_n == 1)
    {
        t2f_scale = (fn - t2f_fn) / (ts - t2f_ts);
        t2f_ts = ts;
        t2f_fn = fn;
        t2f_n = 2;        
        return;
    }
    if(t2f_n<=0)
    {
        t2f_ts = ts;
        t2f_fn = fn;        
        t2f_offset_ts = ts;
        t2f_offset_fn = fn;
        t2f_scale = 1;
        t2f_n = 1;
        return;
    }
}

int DeepGuiderROS::timestamp2Framenumber(dg::Timestamp ts)
{
    if(t2f_n<=0) return -1;

    int fn = (int)((ts - t2f_offset_ts)*t2f_scale + t2f_offset_fn + 0.5);
    return fn;
}

// The main function
int main(int argc, char** argv)
{
    ros::init(argc, argv, "dg_simple_ros");
    ros::NodeHandle nh("~");
    DeepGuiderROS dg_node(nh);
    if (!dg_node.initialize()) return -1;
    dg_node.run();
    return 0;
}
