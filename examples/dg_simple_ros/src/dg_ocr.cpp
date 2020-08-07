#include <ros/ros.h>
#include <sensor_msgs/Image.h>
#include <sensor_msgs/NavSatStatus.h>
#include <sensor_msgs/NavSatFix.h>
#include <sensor_msgs/Imu.h>
#include <cv_bridge/cv_bridge.h>

#include "dg_simple_ros/ocr_info.h"
#include "dg_core.hpp"
#include "dg_ocr.hpp"
#include "dg_utils.hpp"

#define LOAD_PARAM_VALUE(fn, name_cfg, name_var) \
    if (!(fn)[name_cfg].empty()) (fn)[name_cfg] >> name_var;

using namespace dg;

class DGRosRecognizer
{
public:
    DGRosRecognizer(ros::NodeHandle& nh);
    virtual ~DGRosRecognizer();

    bool initialize();
    int run();
    bool runOnce(double timestamp);

protected:
    bool loadConfig(std::string config_file);

    dg::OCRRecognizer m_recognizer;
    std::string m_srcdir = "/work/deepguider/src";

    cv::Mutex m_cam_mutex;
    cv::Mat m_cam_image;
    dg::Timestamp m_cam_capture_time;
    cv::Mat m_gui_image;
    std::string m_winname;

    double m_wait_sec = 0.1;

    // Topic subscribers
    ros::Subscriber sub_image_webcam;

    // Subscriber callbacks
    void callbackImageCompressed(const sensor_msgs::CompressedImageConstPtr& msg);

    // Topic publishers
    ros::Publisher m_publisher;

    // A node handler
    ros::NodeHandle& nh_dg;
};


bool DGRosRecognizer::loadConfig(std::string config_file)
{
    if (config_file.empty())
    {
        return false;
    }

    cv::FileStorage fs(config_file, cv::FileStorage::READ);
    if (!fs.isOpened())
    {
        return false;
    }

    cv::FileNode fn = fs.root();
    LOAD_PARAM_VALUE(fn, "dg_srcdir", m_srcdir);

    return true;
}


DGRosRecognizer::DGRosRecognizer(ros::NodeHandle& nh) : nh_dg(nh)
{
    // overwrite configuable parameters of base class
    m_srcdir = "/work/deepguider/src";   // system path of deepguider/src (required for python embedding)

    // Read ros-specific parameters
    m_wait_sec = 0.01;
    nh_dg.param<double>("wait_sec", m_wait_sec, m_wait_sec);

    // Initialize subscribers
    sub_image_webcam = nh_dg.subscribe("/uvc_image_raw/compressed", 1, &DGRosRecognizer::callbackImageCompressed, this);

    // Initialize publishers
    m_publisher = nh_dg.advertise<dg_simple_ros::ocr_info>("output", 1, true);
}

DGRosRecognizer::~DGRosRecognizer()
{ 
    m_recognizer.clear();
    close_python_environment();
}

bool DGRosRecognizer::initialize()
{
    printf("Initialize %s...\n", m_recognizer.name());

    // load config
    std::string config_file = "dg_ros.yml";
    bool ok = loadConfig(config_file);
    if(ok) printf("\tConfiguration %s loaded!\n", config_file.c_str());

    // initialize python
    bool threaded_run = false;
    if (!init_python_environment("python3", "", threaded_run)) return false;
    printf("\tPython environment initialized!\n");

    // initialize recognizer
    std::string module_path = m_srcdir + "/ocr_recog";
    if (!m_recognizer.initialize("ocr_recognizer", module_path.c_str())) return false;
    printf("\t%s initialized in %.3lf seconds!\n", m_recognizer.name(), m_recognizer.procTime());

    // reset interval variables
    m_cam_image.release();
    m_cam_capture_time = -1;
    m_winname = cv::format("dg_%s", m_recognizer.name());

    // show GUI window
    cv::namedWindow(m_winname, cv::WINDOW_NORMAL);

    printf("\tInitialization is done!\n\n");

    return true;
}

int DGRosRecognizer::run()
{
    printf("Run %s...\n", m_recognizer.name());

    ros::Rate loop(1 / m_wait_sec);
    while (ros::ok())
    {
        ros::Time timestamp = ros::Time::now();
        if (!runOnce(timestamp.toSec())) break;
        ros::spinOnce();
        loop.sleep();
    }

    // end system
    printf("End %s...\n", m_recognizer.name());
    nh_dg.shutdown();

    return 0;
}

bool DGRosRecognizer::runOnce(double timestamp)
{
    cv::Mat cam_image;
    dg::Timestamp cam_timestamp;
    m_cam_mutex.lock();
    if(!m_cam_image.empty())
    {
        cam_image = m_cam_image.clone();
        m_cam_image.release();
        cam_timestamp = m_cam_capture_time;
    }    
    m_cam_mutex.unlock();

    if (!cam_image.empty() && m_recognizer.apply(cam_image, cam_timestamp))
    {
        m_recognizer.print();
        std::vector<OCRResult> ocrs;
        m_recognizer.get(ocrs);
        if(!ocrs.empty())
        {
            dg_simple_ros::ocr_info msg;
            for(int i=0; i<(int)ocrs.size(); i++)
            {
                dg_simple_ros::ocr ocr;
                ocr.label = ocrs[i].label;
                ocr.xmin = ocrs[i].xmin;
                ocr.ymin = ocrs[i].ymin;
                ocr.xmax = ocrs[i].xmax;
                ocr.ymax = ocrs[i].ymax;
                ocr.confidence = ocrs[i].confidence;

                msg.ocrs.push_back(ocr);
            }
            msg.timestamp = cam_timestamp;
            msg.processingtime = m_recognizer.procTime();
            m_publisher.publish(msg);

            // draw results & fps
            m_gui_image = cam_image;
            std::string fn = cv::format("FPS: %.1lf", 1.0 / m_recognizer.procTime());
            cv::putText(m_gui_image, fn.c_str(), cv::Point(20, 50), cv::FONT_HERSHEY_PLAIN, 2.0, cv::Scalar(0, 0, 0), 4);
            cv::putText(m_gui_image, fn.c_str(), cv::Point(20, 50), cv::FONT_HERSHEY_PLAIN, 2.0, cv::Scalar(0, 255, 255), 2);
            m_recognizer.draw(m_gui_image);
        }
    }
    
    if(!m_gui_image.empty())
    {
        cv::imshow(m_winname, m_gui_image);
        int key = cv::waitKey(1);
        if (key == cx::KEY_SPACE) key = cv::waitKey(0);
        if (key == cx::KEY_ESC) return false;
    }

    return true;
}

// A callback function for subscribing a compressed RGB image
void DGRosRecognizer::callbackImageCompressed(const sensor_msgs::CompressedImageConstPtr& msg)
{
    //ROS_INFO_THROTTLE(1.0, "Compressed RGB(timestamp: %f [sec]).", msg->header.stamp.toSec());
    cv_bridge::CvImagePtr image_ptr;
    try
    {
        m_cam_mutex.lock();
        m_cam_image = cv::imdecode(cv::Mat(msg->data), 1);//convert compressed image data to cv::Mat
        m_cam_capture_time = msg->header.stamp.toSec();
        m_cam_mutex.unlock();
    }
    catch (cv_bridge::Exception& e)
    {
        ROS_ERROR("exception @ callbackImageCompressed(): %s", e.what());
        return;
    }
}

// The main function
int main(int argc, char** argv)
{
    ros::init(argc, argv, "dg_ocr");
    ros::NodeHandle nh("~");
    DGRosRecognizer dg_node(nh);
    if (!dg_node.initialize()) return -1;
    dg_node.run();
    return 0;
}
