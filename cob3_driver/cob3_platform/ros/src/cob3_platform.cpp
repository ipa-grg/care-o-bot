//##################
//#### includes ####

// standard includes
//--

// ROS includes
#include <ros/ros.h>

// ROS message includes
#include <geometry_msgs/Twist.h>
#include <tf/transform_broadcaster.h>
#include <nav_msgs/Odometry.h>

// ROS service includes
#include <cob3_srvs/Init.h>
#include <cob3_srvs/Stop.h>
#include <cob3_srvs/Shutdown.h>

// external includes
#include <include/PlatformHardware.h>

//####################
//#### node class ####
class NodeClass
{
    //
    public:
	    // create a handle for this node, initialize node
	    ros::NodeHandle n;
                
        // topics to publish
        ros::Publisher topicPub_Pose2D;
        ros::Publisher topicPub_Odometry;
        tf::TransformBroadcaster odom_broadcaster;

	    // topics to subscribe, callback is called for new messages arriving
        ros::Subscriber topicSub_CmdVel;
        
        // service servers
        ros::ServiceServer srvServer_Init;
        ros::ServiceServer srvServer_Home;
        ros::ServiceServer srvServer_Stop;
        ros::ServiceServer srvServer_Shutdown;
            
        // service clients
        //--
        
        // global variables
        PlatformHardware* pltf;
        bool isInitialized;
        double cmdVelX, cmdVelY, cmdVelTh;
        double x, y, th;
        double dx, dy, dth, dvth;
        double vx, vy, vth, vvth;
        ros::Time current_time;

        // Constructor
        NodeClass()
        {
            // initialize global variables
            isInitialized = false;
            current_time = ros::Time::now();
            cmdVelX = cmdVelY = cmdVelTh = 0;
            x = y = th = 0;
            dx = dy = dth = dvth = 0;
            vx = vy = vth = vvth = 0;

        	// implementation of topics to publish
            topicPub_Odometry = n.advertise<nav_msgs::Odometry>("odometry", 50);

            // implementation of topics to subscribe
            topicSub_CmdVel = n.subscribe("cmdVel", 1, &NodeClass::topicCallback_CmdVel, this);
            
            // implementation of service servers
            srvServer_Init = n.advertiseService("Init", &NodeClass::srvCallback_Init, this);
            srvServer_Stop = n.advertiseService("Stop", &NodeClass::srvCallback_Stop, this);
            srvServer_Shutdown = n.advertiseService("Shutdown", &NodeClass::srvCallback_Shutdown, this);
        }
        
        // Destructor
        ~NodeClass() 
        {
            pltf->setVelPltf(0, 0, 0, 0);
            pltf->shutdownPltf();
            isInitialized = false;
            delete pltf;
        }

        // topic callback functions 
        // function will be called when a new message arrives on a topic
        void topicCallback_CmdVel(const geometry_msgs::Twist::ConstPtr& msg)
        {
            ROS_INFO("received new velocity command [cmdVelX=%3.5f,cmdVelY=%3.5f,cmdVelTh=%3.5f]", 
                     msg->linear.x, msg->linear.y, msg->angular.z);

            cmdVelX = msg->linear.x;
            cmdVelY = msg->linear.y;
            cmdVelTh = msg->angular.z;
        }

        // service callback functions
        // function will be called when a service is querried
        bool srvCallback_Init(cob3_srvs::Init::Request &req,
                              cob3_srvs::Init::Response &res )
        {
            ROS_INFO("This is srvCallback_Init");

            if(isInitialized == false)
            {
                ROS_INFO("...initializing platform...");
                pltf = new PlatformHardware();
                pltf->initPltf();
                isInitialized = true;
                res.success = 0; // 0 = true, else = false
            }
            else
            {
                ROS_ERROR("...platform already initialized...");
                res.success = 1;
                res.errorMessage.data = "platform already initialized";
            }            
            return true;
        }
        
        bool srvCallback_Stop(cob3_srvs::Stop::Request &req,
                              cob3_srvs::Stop::Response &res )
        {
            ROS_INFO("This is srvCallback_Stop");
            if(isInitialized == true)
            {
                ROS_INFO("...stopping platform...");
                cmdVelX = 0;
                cmdVelY = 0;
                cmdVelTh = 0;
                res.success = 0; // 0 = true, else = false
            }
            else
            {
                ROS_ERROR("...platform not initialized...");
                res.success = 1;
                res.errorMessage.data = "platform not initialized";
            }
            return true;
        }

        bool srvCallback_Shutdown(cob3_srvs::Shutdown::Request &req,
                                  cob3_srvs::Shutdown::Response &res )
        {
            ROS_INFO("This is srvCallback_Shutdown");
            if(isInitialized == true)
            {
                ROS_INFO("...shutting down platform...");
                pltf->shutdownPltf();
                isInitialized = false;
                res.success = 0; // 0 = true, else = false
            }            
            else
            {
                ROS_ERROR("...platform not initialized...");
                res.success = 1;
                res.errorMessage.data = "platform not initialized";
            }
            return true;
        }
        
        // other function declarations
        void updateCmdVel()
        {
            // send vel if platform is initialized
            if(isInitialized == true)
            {
                ROS_INFO("update vel");
                pltf->setVelPltf(cmdVelX*1000, cmdVelY*1000, cmdVelTh, 0);
            }
        }

        void updateOdometry()
        {
            if(isInitialized == true)
            {
                // get odometry from platform
                pltf->getDeltaPosePltf(dx, dy, dth, dvth,
        					           vx, vy, vth, vvth);

                // add delta values to old values
                x += dx;
                y += dy;
                th += dth;

                // since all odometry is 6DOF we'll need a quaternion created from yaw
                geometry_msgs::Quaternion odom_quat = tf::createQuaternionMsgFromYaw(th);

                //first, we'll publish the transform over tf
                geometry_msgs::TransformStamped odom_trans;
                odom_trans.header.stamp = current_time;
                odom_trans.header.frame_id = "odom";
                odom_trans.child_frame_id = "base_link";

                odom_trans.transform.translation.x = x;
                odom_trans.transform.translation.y = y;
                odom_trans.transform.translation.z = 0.0;
                odom_trans.transform.rotation = odom_quat;

                //send the transform
                odom_broadcaster.sendTransform(odom_trans);

                //next, we'll publish the odometry message over ROS
                nav_msgs::Odometry odom;
                odom.header.stamp = current_time;
                odom.header.frame_id = "odom";

                //set the position
                odom.pose.pose.position.x = x;
                odom.pose.pose.position.y = y;
                odom.pose.pose.position.z = 0.0;
                odom.pose.pose.orientation = odom_quat;

                //set the velocity
                odom.child_frame_id = "base_link";
                odom.twist.twist.linear.x = vx;
                odom.twist.twist.linear.y = vy;
                odom.twist.twist.angular.z = vth;

                //publish the message
                topicPub_Odometry.publish(odom);

                ROS_INFO("published Pose2D: pos[x=%3.2f,y=%3.2f,th=%3.2f]",x, y, th);
                ROS_INFO("published Pose2D: vel[vy=%3.2f,vx=%3.2f,vth=%3.2f]",vx, vy, vth);
            }
        }
};

//#######################
//#### main programm ####
int main(int argc, char** argv)
{
    // initialize ROS, spezify name of node
    ros::init(argc, argv, "cob3_driver_platform");
    
    NodeClass nodeClass;

    // main loop
 	ros::Rate loop_rate(10); // Hz 
    while(nodeClass.n.ok())
    {
        nodeClass.current_time = ros::Time::now();

        // publish Odometry
        nodeClass.updateOdometry();

        // update velocity of platform
        nodeClass.updateCmdVel();

        ros::spinOnce();
        loop_rate.sleep();
    }
    
//    ros::spin();

    return 0;
}