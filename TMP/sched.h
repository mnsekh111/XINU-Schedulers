/*
 * sched.h
 *
 *      Author: mnsekh111
 */

#define DEFAULTSCHED 0
#define LINUXSCHED 1
#define MULTIQSCHED 2
extern int  getschedclass();
extern void setschedclass(int);
extern int  scheduleclass;
