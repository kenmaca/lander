/*
	Lander Control simulation.

	Updated by F. Estrada for CSC C85, Oct. 2013
	Mod by Per Parker, Sep. 2015

	Learning goals:

	- To explore the implementation of control software
	  that is robust to malfunctions/failures.

	The exercise:

	- The program loads a terrain map from a .ppm file.
	  the map shows a red platform which is the location
	  a landing module should arrive at.
	- The control software has to navigate the lander
	  to this location and deposit the lander on the
	  ground considering:

	  * Maximum vertical speed should be less than 5 m/s at touchdown
	  * Maximum landing angle should be less than 10 degrees w.r.t vertical

	- Of course, touching any part of the terrain except
	  for the landing platform will result in destruction
	  of the lander

	This has been made into many videogames. The oldest one
	I know of being a C64 game called 1985 The Day After.
        There are older ones! (for bonus credit, find the oldest
        one and send me a description/picture plus info about the
        platform it ran on!)

	Your task:

	- These are the 'sensors' you have available to control
          the lander.

	  Velocity_X();  - Gives you the lander's horizontal velocity
	  Velocity_Y();	 - Gives you the lander's vertical velocity
	  Position_X();  - Gives you the lander's horizontal position (0 to 1024)
	  Position Y();  - Gives you the lander's vertical position (0 to 1024)

          Angle();	 - Gives the lander's angle w.r.t. vertical in DEGREES (upside-down = 180 degrees)

	  SONAR_DIST[];  - Array with distances obtained by sonar. Index corresponds
                           to angle w.r.t. vertical direction measured clockwise, so that
                           SONAR_DIST[0] is distance at 0 degrees (pointing upward)
                           SONAR_DIST[1] is distance at 10 degrees from vertical
                           SONAR_DIST[2] is distance at 20 degrees from vertical
                           .
                           .
                           .
                           SONAR_DIST[35] is distance at 350 degrees from vertical

                           if distance is '-1' there is no valid reading. Note that updating
                           the sonar readings takes time! Readings remain constant between
                           sonar updates.

          RangeDist();   - Uses a laser range-finder to accurately measure the distance to ground
                           in the direction of the lander's main thruster.
                           The laser range finder never fails (probably was designed and
                           built by PacoNetics Inc.)

          Note: All sensors are NOISY. This makes your life more interesting.

	- Variables accessible to your 'in flight' computer

	  MT_OK		- Boolean, if 1 indicates the main thruster is working properly
	  RT_OK		- Boolean, if 1 indicates the right thruster is working properly
	  LT_OK		- Boolean, if 1 indicates thr left thruster is working properly
          PLAT_X	- X position of the landing platform
          PLAY_Y        - Y position of the landing platform

	- Control of the lander is via the following functions
          (which are noisy!)

	  Main_Thruster(double power);   - Sets main thurster power in [0 1], 0 is off
	  Left_Thruster(double power);	 - Sets left thruster power in [0 1]
	  Right_Thruster(double power);  - Sets right thruster power in [0 1]
	  Rotate(double angle);	 	 - Rotates module 'angle' degrees clockwise
					   (ccw if angle is negative) from current
                                           orientation (i.e. rotation is not w.r.t.
                                           a fixed reference direction).

 					   Note that rotation takes time!


	- Important constants

	  G_ACCEL = 8.87	- Gravitational acceleration on Venus
	  MT_ACCEL = 35.0	- Max acceleration provided by the main thruster
	  RT_ACCEL = 25.0	- Max acceleration provided by right thruster
	  LT_ACCEL = 25.0	- Max acceleration provided by left thruster
          MAX_ROT_RATE = .075    - Maximum rate of rotation (in radians) per unit time

	- Functions you need to analyze and possibly change

	  * The Lander_Control(); function, which determines where the lander should
	    go next and calls control functions
          * The Safety_Override(); function, which determines whether the lander is
            in danger of crashing, and calls control functions to prevent this.

	- You *can* add your own helper functions (e.g. write a robust thruster
	  handler, or your own robust sensor functions - of course, these must
	  use the noisy and possibly faulty ones!).

	- The rest is a black box... life sometimes is like that.

        - Program usage: The program is designed to simulate different failure
                         scenarios. Mode '1' allows for failures in the
                         controls. Mode '2' allows for failures of both
                         controls and sensors. There is also a 'custom' mode
                         that allows you to test your code against specific
                         component failures.

			 Initial lander position, orientation, and velocity are
                         randomized.

	  * The code I am providing will land the module assuming nothing goes wrong
          with the sensors and/or controls, both for the 'easy.ppm' and 'hard.ppm'
          maps.

	  * Failure modes: 0 - Nothing ever fails, life is simple
			   1 - Controls can fail, sensors are always reliable
			   2 - Both controls and sensors can fail (and do!)
			   3 - Selectable failure mode, remaining arguments determine
                               failing component(s):
                               1 - Main thruster
                               2 - Left Thruster
                               3 - Right Thruster
                               4 - Horizontal velocity sensor
                               5 - Vertical velocity sensor
                               6 - Horizontal position sensor
                               7 - Vertical position sensor
                               8 - Angle sensor
                               9 - Sonar

        e.g.

             Lander_Control easy.ppm 3 1 5 8

             Launches the program on the 'easy.ppm' map, and disables the main thruster,
             vertical velocity sensor, and angle sensor.

		* Note - while running. Pressing 'q' on the keyboard terminates the 
			program.

        * Be sure to complete the attached REPORT.TXT and submit the report as well as
          your code by email. Subject should be 'C85 Safe Landings, name_of_your_team'

	Have fun! try not to crash too many landers, they are expensive!

  	Credits: Lander image and rocky texture provided by NASA
		 Per Parker spent some time making sure you will have fun! thanks Per!
*/

/*
  Standard C libraries
*/
#include <math.h>
#include <iostream>
// #include <ctime>
#include <stdlib.h>
// #include <unistd.h>
using namespace std;


#include "Lander_Control.h"

int rotate_flag = 0;
int rotate_flag_safety = 0;
int safety = 0;
int done = 0;
int rotation_count = 0;
int angle_flag = 1;
double angle = 0.0;
double prev_angle = 0.0;
double power_ratio = 0.71428571428;
double velocity [2];
double velx, vely, posx, posy;
double position [2];
double position_v [2];
int first_loop = 1;
double dest_angle = 0.0;
double main_power;
double left_power;
double right_power;
double power;
double bs_const = 0.02359;
double bs_const_h = 0.0283;
int landing_flag = 1;


void Right_Thruster_robust(double power);
void Working_Thruster_On(double power);
int Working_Thruster();
void Set_Rotate(double angle);
void Left_Thruster_robust(double power);
void Main_Thruster_robust(double power);
int Is_OK();
void update_param();
double Velocity_X_robust();
double Velocity_Y_robust();
double get_Y_acc();
double get_X_acc();
double Position_X_robust();
double Position_Y_robust();

void Lander_Control(void)
{
 /*
   This is the main control function for the lander. It attempts
   to bring the ship to the location of the landing platform
   keeping landing parameters within the acceptable limits.

   How it works:

   - First, if the lander is rotated away from zero-degree angle,
     rotate lander back onto zero degrees.
   - Determine the horizontal distance between the lander and
     the platform, fire horizontal thrusters appropriately
     to change the horizontal velocity so as to decrease this
     distance
   - Determine the vertical distance to landing platform, and
     allow the lander to descend while keeping the vertical
     speed within acceptable bounds. Make sure that the lander
     will not hit the ground before it is over the platform!

   As noted above, this function assumes everything is working
   fine.
*/

/*************************************************
 TO DO: Modify this function so that the ship safely
        reaches the platform even if components and
        sensors fail!

        Note that sensors are noisy, even when
        working properly.

        Finally, YOU SHOULD provide your own
        functions to provide sensor readings,
        these functions should work even when the
        sensors are faulty.

        For example: Write a function Velocity_X_robust()
        which returns the module's horizontal velocity.
        It should determine whether the velocity
        sensor readings are accurate, and if not,
        use some alternate method to determine the
        horizontal velocity of the lander.

        NOTE: Your robust sensor functions can only
        use the available sensor functions and control
        functions!
	DO NOT WRITE SENSOR FUNCTIONS THAT DIRECTLY
        ACCESS THE SIMULATION STATE. That's cheating,
        I'll give you zero.
**************************************************/

 double VXlim;
 double VYlim;
 int p;

  


  // Get good angle measurment
  double sum = 0;
  for (p = 0; p < 3000; p++) {
   sum += Angle();
  }
  angle = sum/3000;



  // ret the lander rotate before turning one another truster. 
  if (rotate_flag) {
   update_param();
   rotation_count++;
   if (rotation_count > 10) {
    rotation_count = 0;
    rotate_flag = 0;
    rotate_flag_safety = 0;
   } else {
    rotate_flag_safety = 1;
    return;
   }
  }



   // Set velocity limits depending on distance to platform.
   // If the module is far from the platform allow it to
   // move faster, decrease speed limits as the module
   // approaches landing. You may need to be more conservative
   // with velocity limits when things fail.
   if (fabs(Position_X_robust()-PLAT_X)>200) VXlim=25;
   else if (fabs(Position_X_robust()-PLAT_X)>100) VXlim=15;
   else VXlim=5;

   if (PLAT_Y-Position_Y_robust()>300 && fabs(Position_X_robust()-PLAT_X) > 100) VYlim=0.0;
   else if (PLAT_Y-Position_Y_robust() < 200 && fabs(Position_X_robust()-PLAT_X) > 100) VYlim=5.0;
   else if (PLAT_Y-Position_Y_robust()>200) VYlim=-10;
   else if (PLAT_Y-Position_Y_robust()>100) VYlim=-6;  // These are negative because they
   else VYlim=-4;				       // limit descent velocity

   // Ensure we will be OVER the platform when we land
   if ( fabs(PLAT_X-Position_X_robust())/fabs(Velocity_X_robust()) > 
    1.25*fabs(PLAT_Y-Position_Y_robust())/fabs(Velocity_Y_robust()) ) VYlim=0.0;

   //update_param();

   if (done) {
    Set_Rotate(0.0);
    Main_Thruster(0.2);
    return;
   }

   if (Velocity_Y_robust() < VYlim) { 
    safety = 1;
   }

   // turn on the main truster if the desent velocity is too high
   if (safety) {
    if (Velocity_Y_robust() < VYlim + fmin(3, 0.3*VYlim)) {
     Main_Thruster_robust(0.7);
     return;
   } else
     safety = 0;
   }

   if (Position_X_robust()>PLAT_X)
   {
    // Lander is to the LEFT of the landing platform, use Right thrusters to move
    // lander to the left.
    // Make sure we're not fighting ourselves here!
    if (Velocity_X_robust()>(-VXlim)) 
      Right_Thruster_robust((VXlim+fmin(0,Velocity_X_robust()))/VXlim);
    else
    {
     // Exceeded velocity limit, brake
     Left_Thruster_robust(fabs(VXlim-Velocity_X_robust()));
    }

   } else {
    // Lander is to the RIGHT of the landing platform, opposite from above
    if (Velocity_X_robust()<VXlim) 
     Left_Thruster_robust((VXlim-fmax(0,Velocity_X_robust()))/VXlim);
    else
    {
     Right_Thruster_robust(fabs(VXlim-Velocity_X_robust()));
    }
   }

   // if the lander is close enough to the platform prepare to land.
   if (fabs(Position_X_robust() - PLAT_X) < 50 && (PLAT_Y - Position_Y_robust()) < 30) {
    Left_Thruster(0);
    Right_Thruster(0);
    Main_Thruster(0);
    Set_Rotate(0.0);
    done = 1;
    return;
   }  

} 


/*
THE MOST IMPORTANT FUNCTION, DO NOT REMOVE!!!

Overrite the Lander_Control commands to ensure the
safty of all passengers.
*/
void Safety_Override(void)
{
   //TRUST!!! it works.
   return;
}





void Main_Thruster_robust(double set_power) {
 /**
	Function that adjust the angle of the lander, if the main 
	thruster is not wroking, so that the right or the left 
	thruster can be used instead.
 */
 if (RT_OK) {
  Set_Rotate(90.0);
  Main_Thruster(0.0);
  Left_Thruster(0.0);
  Right_Thruster(set_power);
  main_power = 0.0;
  left_power = 0.0;
  right_power = set_power;
 } else if (LT_OK) {
  Set_Rotate(270.0);
  Right_Thruster(0.0);
  Main_Thruster(0.0);
  Left_Thruster(set_power);
  right_power = 0.0;
  main_power = 0.0;
  left_power = set_power;
 } else {
  Set_Rotate(0.0);
  Right_Thruster(0.0);
  Left_Thruster(0.0);
  Main_Thruster(set_power * power_ratio);
  right_power = 0.0;
  left_power = 0.0;
  main_power = set_power;
 }
 rotate_flag = 1;
}



void Right_Thruster_robust(double set_power) {
 /**
 Function that adjust the angle of the lander, if the right 
 thruster is not wroking, so that the main or the left 
 thruster can be used instead.
 */
 if (RT_OK) {
  Set_Rotate(0.0);
  Main_Thruster(0.0);
  Left_Thruster(0.0);
  Right_Thruster(set_power);
  main_power = 0.0;
  left_power = 0.0;
  right_power = set_power;
 } else if (LT_OK) {
  Set_Rotate(180.0);
  Right_Thruster(0.0);
  Main_Thruster(0.0);
  Left_Thruster(set_power);
  right_power = 0.0;
  main_power = 0.0;
  left_power = set_power;
 } else {
  Set_Rotate(270.0);
  Right_Thruster(0.0);
  Left_Thruster(0.0);
  Main_Thruster(set_power * power_ratio);
  right_power = 0.0;
  left_power = 0.0;
  main_power = set_power;
 }
 rotate_flag = 1;
}

void Left_Thruster_robust(double set_power) {
 /**
 Function that adjust the angle of the lander, if the left 
 thruster is not wroking, so that the main or the right 
 thruster can be used instead.
 */
 if (RT_OK) {
  Set_Rotate(180.0);
  Main_Thruster(-10);
  Left_Thruster(0.0);
  Right_Thruster(set_power);
  main_power = 0.0;
  left_power = 0.0;
  right_power = set_power;
 } else if (LT_OK) {
  Set_Rotate(0.0);
  Right_Thruster(0.0);
  Main_Thruster(0.0);
  Left_Thruster(set_power);
  right_power = 0.0;
  main_power = 0.0;
  left_power = set_power;
 } else {
  Set_Rotate(90.0);
  Right_Thruster(0.0);
  Left_Thruster(0.0);
  Main_Thruster(set_power * power_ratio);
  right_power = 0.0;
  left_power = 0.0;
  main_power = set_power;
 }
 rotate_flag = 1;
}


// Turns on the main thruster with power power
void Working_Thruster_On(double power) {
 (MT_OK) ? Main_Thruster(power) : ((RT_OK) ? Right_Thruster(power) : Left_Thruster(power));
}

/* Returns a number for the working thruster:
   1 - if the main thruster is working
   2 - if the left thruster is working
   3 - if the right thruster is working
*/
int Working_Thruster() { 
 return (RT_OK) ? 1 : ((LT_OK) ? 3 : 2);
}


// Rotate the langer such that the angle of the lander is 
// angle from the vertical clock wise
void Set_Rotate(double des_angle) {
 (fabs(angle - des_angle) > 180.0) ? 
             (((des_angle - angle) > 0.0) ? Rotate(-(360.0 - (des_angle - angle))) : Rotate((360.0 + (des_angle - angle)))) 
                            : Rotate(-(angle - des_angle));
}

// Returns 1 of all thrusters are working
int Is_OK() {
  return (MT_OK && RT_OK && LT_OK) ? 0 : 0;
}




/*
 Update the model parameters: position and velocity from 
 current acceleration and velocity
*/
void update_param() {

 double sum1 = 0, sum2 = 0, sum3 = 0, sum4 = 0;
 int p;
 for (p = 0; p < 5000; p++) {
  sum1 += Velocity_X();
  sum2 += Position_X();
  sum3 += Velocity_Y();
  sum4 += Position_Y();
 }
 velx = sum1/5000;
 vely = sum3/5000;
 posx = sum2/5000;
 posy = sum4/5000;

 double a_x = get_X_acc();
 double a_y = get_Y_acc();

 if (first_loop < 20) {
   int l, n = 1000;
   double sum1 = 0, sum2 = 0, sum3 = 0, sum4 = 0, sum_a = 0;
   for (l = 0; l < n; l++) {
    sum1 += Velocity_X();
    sum2 += Velocity_Y();
    sum3 += Position_X();
    sum4 += Position_Y();
   }
   velocity[0] = sum1/n;
   velocity[1] = sum2/n;
   position[0] = sum3/n;
   position_v[0] = sum3/n;
   position[1] = sum4/n;
   position_v[1] = sum4/n;
   first_loop++;
   Left_Thruster(0.0);
   Right_Thruster(0.0);
   Main_Thruster(0.0);
   

  } else {
   velocity[0] += T_STEP*a_x;
   velocity[1] += T_STEP*a_y;
   position[0] += velocity[0]*T_STEP*S_SCALE + ((double)rand()/RAND_MAX - 0.5);
   position[1] -= velocity[1]*T_STEP*S_SCALE - ((double)rand()/RAND_MAX - 0.5);
   position_v[0] += velx*T_STEP*S_SCALE;
   position_v[1] -= vely*T_STEP*S_SCALE;

  }
}



/*
Calculate the acceleration in the vertical direction 
*/
double get_Y_acc() {
    double a_y = -cos(((angle/90.0 + 1))*(PI/2)) * fmin(((right_power+RT_OK * (right_power*-0.06 + bs_const_h))*RT_ACCEL),RT_ACCEL);
    a_y += -cos(((angle/90.0) + 2)*(PI/2)) * fmin(((main_power+MT_OK * (main_power*-0.053 + bs_const))*RT_ACCEL),RT_ACCEL);
    a_y += -cos(((angle/90.0) + 3)*(PI/2)) * fmin(((left_power+LT_OK*(left_power*-0.06 + bs_const_h))*LT_ACCEL),LT_ACCEL);
    return a_y - G_ACCEL;
}


/*
Calculate the acceleration in the horizontal direction
*/
double get_X_acc() {
    double a_x = -sin(((angle/90.0 + 1))*(PI/2)) * fmin(((right_power+RT_OK * (right_power*-0.06 + bs_const_h))*RT_ACCEL),RT_ACCEL);
    a_x += -sin(((angle/90.0) + 2)*(PI/2)) * fmin(((main_power+MT_OK * (main_power*-0.053 + bs_const))*RT_ACCEL),RT_ACCEL);
    return a_x - sin(((angle/90.0) + 3)*(PI/2)) * fmin(((left_power+ LT_OK * (left_power*-0.06 + bs_const_h))*LT_ACCEL),LT_ACCEL);
}

double Velocity_X_robust() {
 double vel_threshold = 6;
 if (fabs(Velocity_X() - Velocity_X()) < vel_threshold 
      && fabs(Velocity_X() - Velocity_X()) < vel_threshold 
          && fabs(Velocity_X() - Velocity_X()) < vel_threshold) {
  return velocity[0];
 } else {
  return velocity[0];
 }
}


double Velocity_Y_robust() {
 double vel_threshold = 6;
 if (fabs(Velocity_Y() - Velocity_Y()) < vel_threshold 
     && fabs(Velocity_Y() - Velocity_Y()) < vel_threshold 
        && fabs(Velocity_Y() - Velocity_Y()) < vel_threshold) {
  return velocity[1];
 } else {
  return velocity[1];
 }
}



double Position_X_robust() {
  double vel_threshold = 6, pos_threshold = 65;
  if (fabs(Position_X() - Position_X()) < pos_threshold 
      && fabs(Position_X() - Position_X()) < pos_threshold 
          && fabs(Position_X() - Position_X()) < pos_threshold) {
    return posx;
  } else if (fabs(Velocity_X() - Velocity_X()) < vel_threshold 
      && fabs(Velocity_X() - Velocity_X()) < vel_threshold 
          && fabs(Velocity_X() - Velocity_X()) < vel_threshold) {
    return position_v[0];
  } else {
    return position[0];
  }
}


double Position_Y_robust() {
  double vel_threshold = 6, pos_threshold = 65;
  if (fabs(Position_Y() - Position_Y()) < pos_threshold 
       && fabs(Position_Y() - Position_Y()) < pos_threshold 
           && fabs(Position_Y() - Position_Y()) < pos_threshold) {
    return posy;
  } else if (fabs(Velocity_Y() - Velocity_Y()) < vel_threshold 
       && fabs(Velocity_Y() - Velocity_Y()) < vel_threshold 
           && fabs(Velocity_Y() - Velocity_Y()) < vel_threshold) {
    return position_v[1];
  } else {
    return position[1];
  }
}
