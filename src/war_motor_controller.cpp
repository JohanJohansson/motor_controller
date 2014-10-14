#include "ros/ros.h"
#include "geometry_msgs/Twist.h"
#include "ras_arduino_msgs/Encoders.h"
#include "ras_arduino_msgs/PWM.h"
#include "math.h"

class MotorController
{
public:

    ros::NodeHandle n;
    ros::Subscriber encoder_subscriber;
    ros::Subscriber twist_subscriber;
    ros::Publisher pub;
    double pwmOut1,pwmOut2;
    double actualAngVelRight,actualAngVelLeft,desiredAngVelLeft,desiredAngVelRight;
    double pwm1, pwm2;
    double previousErrorLeft, previousErrorRight;
    double ItermL, ItermR, DtermL, DtermR;
    
    MotorController()
    {
        n = ros::NodeHandle("~");
        motor_cotroller_ = NULL;
    }

    ~MotorController()
    {
        delete motor_cotroller_;
    }
    

    void init()
    {
        motor_cotroller_ = new MotorController();
        encoder_subscriber = n.subscribe("/kobuki/encoders", 1, &MotorController::encoderCallback,this);
        twist_subscriber = n.subscribe("/motor_controller/twist",1, &MotorController::twistCallback,this);
        pub = n.advertise<ras_arduino_msgs::PWM>("/kobuki/pwm", 1);

        //P=3;
        /*
        pwmOut1=0;
        pwmOut2=0;
        actualAngVelRight=0;
        actualAngVelLeft=0;
        desiredAngVelLeft=0;
        desiredAngVelRight=0;
        */
    }
    

    void encoderCallback(const ras_arduino_msgs::Encoders::ConstPtr &enc_msg)
    {
        double enc1=enc_msg->encoder1;
        double enc2=enc_msg->encoder2;
        double delta_enc1=enc_msg->delta_encoder1;
        double delta_enc2=enc_msg->delta_encoder2;
        double sampleTime=0.1;
        //ROS_INFO("I heard: [%d]", enc_msg->encoder1);
        actualAngVelRight=(delta_enc2*(M_PI/180))/sampleTime;
        actualAngVelLeft=(delta_enc1*(M_PI/180))/sampleTime;
        //ROS_INFO("Actual wR: [%f]",actualAngVelRight);
        //ROS_INFO("Actual wL: [%f]",actualAngVelLeft);
    }


    void twistCallback(const geometry_msgs::Twist::ConstPtr &twist_msg)
    {
        double twist_linVel_x=twist_msg->linear.x;
        double twist_angVel_x=twist_msg->angular.z;
        double base=0.23;
        double r=0.0352;
        //ROS_INFO("I heard: [%f]", twist_msg->angular.x);

        desiredAngVelRight = (twist_linVel_x+(base/2)*twist_angVel_x)/r;
        desiredAngVelLeft = (twist_linVel_x-(base/2)*twist_angVel_x)/r;
        //ROS_INFO("wRight: [%f]\n wLeft: [%f]",desiredAngVelRight,desiredAngVelLeft);
    }
    void controllerVelocities()
    {   
        // This controller uses the Ziegler-Nichols method
        // to tune the KP,KI,KD
        double Ku = 4;
        double Tu = 1;
        double KPR = 0.6*Ku;
        double KPL = KPR;
        double KIL = 2*KPL/Tu;
        double KIR = KIL;
        double KDL = KPL*Tu/8;
        double KDR = KDL;
        double dT=0.1;
        ras_arduino_msgs::PWM pwm_msg;
        //ROS_INFO("desiredAngVelRight: [%f]",desiredAngVelRight);
        //ROS_INFO("actualAngVelRight:[%f]",actualAngVelRight);
                
        // Controller for left wheel
        double errorLeft = (desiredAngVelLeft - actualAngVelLeft);
        ItermL = ItermL + KIL*errorLeft*dT;
        DtermL = (errorLeft - previousErrorLeft)/dT;
        pwm1 = pwm1 + KPL*errorLeft + ItermL + KDL*DtermL;
        if (pwm1 > 255)
            pwm1 = 255;

        // Controller for left wheel
        double errorRight = (desiredAngVelRight - actualAngVelRight);
        ItermR = ItermR + KIR*errorRight*dT;
        DtermR = (errorRight - previousErrorRight)/dT;   
        pwm2 = pwm2 + KPR*errorRight + ItermR + KDR*DtermR;
        if (pwm2 > 255)
            pwm2 =255;

        int pwmOut1 = (int)pwm1;
        int pwmOut2 = (int)pwm2;
        //ROS_INFO("PWM1: [%d]",pwmOut1);
        //ROS_INFO("PWM2:[%d]",pwmOut2);

        pwm_msg.PWM1 = pwmOut1;
        pwm_msg.PWM2 = pwmOut2;
        

        //ROS_INFO("ErrorR: [%f]",errorRight);
        //ROS_INFO("ErrorL:[%f]",errorLeft);
        pub.publish(pwm_msg);
      
        previousErrorLeft = errorLeft;
        previousErrorRight = errorRight;
    }
private:
    MotorController *motor_cotroller_;

};

int main(int argc, char **argv)
{
    ros::init(argc, argv, "motor_controller");

    MotorController controller;
    
    controller.init();
    
    ros::Rate loop_rate(10);

    
      while (controller.n.ok())
      {
        ros::spinOnce();
        controller.controllerVelocities();     
        loop_rate.sleep();
        
      } 

    return 0;

}
