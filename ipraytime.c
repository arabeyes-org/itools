/************************************************************************
 * $Id: ipraytime.c,v 1.31 2004/08/13 16:29:02 thamer Exp $
 *
 * ------------
 * Description:
 * ------------
 *  Copyright (c) 2004, Arabeyes, Nadim Shaikli
 *
 *  An Islamic Prayer time indicator/calculator which uses the
 *  PrayerEngine (ITL) library.
 *
 *  (*) Based on demo by Thamer Mahmoud.
 *
 * -----------------
 * Revision Details:    (Updated by Revision Control System)
 * -----------------
 *  $Date: 2004/08/13 16:29:02 $
 *  $Author: thamer $
 *  $Revision: 1.31 $
 *  $Source: /home/arabeyes/cvs/projects/itl/programs/itools/ipraytime.c,v $
 *
 * (www.arabeyes.org - under GPL license)
 ************************************************************************/

/* TODO:
   - Add am/pm option instead of military time
   - Add proper input grokking so unique inputs (like -m/-mo/-mon/etc)
   all equate to the longer form of option
   - Add a proper 'man' page and documentation
*/

/* Figure out why time of tm structure we're dealing with */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>             /* for strlen/strcat/etc */
#include <unistd.h>             /* for getuid */
#include <pwd.h>                /* for getpwuid */

/* For time_t */
#ifdef TM_IN_SYS_TIME
#include <sys/time.h>
#else
#include <time.h>
#endif

#include <itl/prayer.h>

#define PROG_NAME       "ipraytime";
#define PROG_RCFILE     ".iprayrc";

typedef struct
{
   char *city;         /* City name */
   double lat;         /* Latitude  in decimal degree */
   double lon;         /* Longitude in decimal degree */
   double utc;         /* UTC difference */
   double sealevel;    /* Height above Sea level (in meters) */
   double pressure;    /* Atmospheric pressure in millibars  */
   double temperature; /* Temperature in Celsius degree */
   int dst;            /* Daylight savings time switch */
   int angle_method;   /* Angle settings/method of calculation */
   double fajrangle;   /* Fajr angle */
   double ishaaangle;  /* Ishaa angle */
   double imsaakangle; /* Imsaak and fajr angle differance */
   double nearestLat;  /* Nearest latitude */
   int fajrinv;        /* Fajr Interval 90 or 120 (0:not used) */
   int ishaainv;       /* Ishaa Interval 90 or 120 (0:not used) */
   int imsaakinv;      /* If set, Imsaak Interval from fajr is 10min */
   int mathhab;        /* 1:Shaf'i, 2:Hanafi */
   int round;          /* rounding method */
   int extreme;        /* Extreme latitude calcs engaged */
   int hformat;        /* 24, 12 */
   char dst_start[8];  /* Start date of DST period in yyyymmdd */
   char dst_end[8];    /* Ending date of DST period yyyymmdd */
} sPref;


/**
   Print out the command-line usage of this application
**/
void
usage(int leave)
{ 
   char *pspaces;
   char *pname = PROG_NAME;

   pspaces = malloc(strlen(pname));
   strncpy(pspaces, "                      ", strlen(pname));

   fprintf(stderr, "%s [--date yyyymmdd] [--latitude  real_num]\n", pname);
   fprintf(stderr, "%s                   [--longitude real_num]\n", pspaces);
   fprintf(stderr, "%s                   [--utcdiff   real_num]\n", pspaces);
   fprintf(stderr, "%s                   [--method   [int_num]]\n", pspaces);
   fprintf(stderr, "%s                   [--dst   [int_num]]\n", pspaces);
   fprintf(stderr, "%s                   [--dst-start   yyyymmdd]\n", pspaces);
   fprintf(stderr, "%s                   [--dst-end   yyyymmdd]\n", pspaces);
   fprintf(stderr, "%s                   [--round   [int_num]]\n", pspaces);
   fprintf(stderr, "%s                   [--extreme   [int_num]]\n", pspaces);
   fprintf(stderr, "%s                   [--sea-level   real_num]\n", pspaces);
   fprintf(stderr, "%s                   [--month [int_num]]\n", pspaces);
   fprintf(stderr, "%s                   [--year  [int_num]]\n", pspaces);
   fprintf(stderr, "%s                   [--file  pref_file]\n", pspaces);
   fprintf(stderr, "%s                   [--regular-hour]\n", pspaces);
   fprintf(stderr, "%s                   [--end]\n", pspaces);
   fprintf(stderr, "%s                   [--brief]\n", pspaces);
   fprintf(stderr, "%s                   [--help]\n", pspaces);

   free(pspaces);

   if (leave)
      exit(10);
}


/**
   Error printing (& possibly exit) apparatus.
**/
void
error(int leave,
      char* err_msg)
{
   char *prog_name = PROG_NAME;

   fprintf(stderr, "[%s]: %s", prog_name, err_msg);

   if (leave)
      exit(5);
}


/**
   Read-in passed-in file to populate passed-in structure.
**/
int
process_file(sPref *pref_data,
             char *filename)
{
   FILE *fp;
   char ins[50], inp1[50], inp2[50];

   double num;
   char *city_name;

   /* Process the RC file, if one is there */
   fp   = fopen(filename, "r");

   if (fp == NULL)
      return(1);
 
   while (fgets(ins, sizeof(ins), fp))
   {
      sscanf(ins, "%s %[^\n]", inp1, inp2);

      if (strcasecmp(inp1, "City:") == 0)
      {
         city_name = malloc(strlen(inp2)+1);
         strcpy(city_name, inp2);
         pref_data->city = city_name;
      }
      if (strcasecmp(inp1, "Latitude:") == 0)
      {
         num = atof(inp2);
         pref_data->lat = num;
      }
      if (strcasecmp(inp1, "Longitude:") == 0)
      {
         num = atof(inp2);
         pref_data->lon = num;
      }
      if (strcasecmp(inp1, "UTC:") == 0)
      {
         num = atof(inp2);
         pref_data->utc = num;
      }
      if (strcasecmp(inp1, "AngleMethod:") == 0)
      {
         num = atoi(inp2);
         pref_data->angle_method = num;
      }
      if (strcasecmp(inp1, "Pressure:") == 0)
      {
         num = atof(inp2);
         pref_data->pressure = num;
      }
      if (strcasecmp(inp1, "Temperature:") == 0)
      {
         num = atof(inp2);
         pref_data->temperature = num;
      }
      if (strcasecmp(inp1, "SeaLevel:") == 0)
      {
         num = atof(inp2);
         pref_data->sealevel = num;
      }
      if (strcasecmp(inp1, "DST:") == 0)
      {
         num = atoi(inp2);
         pref_data->dst = num;
      }
      if (strcasecmp(inp1, "FajrAngle:") == 0)
      {
         num = atof(inp2);
         pref_data->fajrangle = num;
      }
      if (strcasecmp(inp1, "IshaaAngle:") == 0)
      {
         num = atof(inp2);
         pref_data->ishaaangle = num;
      }
      if (strcasecmp(inp1, "ImsaakAngle:") == 0)
      {
         num = atof(inp2);
         pref_data->imsaakangle = num;
      }
      if (strcasecmp(inp1, "FajrInterval:") == 0)
      {
         num = atoi(inp2);
         pref_data->fajrinv = num;
      }
      if (strcasecmp(inp1, "IshaaInterval:") == 0)
      {
         num = atoi(inp2);
         pref_data->ishaainv = num;
      }
      if (strcasecmp(inp1, "ImsaakInterval:") == 0)
      {
         num = atoi(inp2);
         pref_data->imsaakinv = num;
      }
      if (strcasecmp(inp1, "Mathhab:") == 0)
      {
         num = atoi(inp2);
         pref_data->mathhab = num;
      }
      if (strcasecmp(inp1, "ExtremeMethod:") == 0)
      {
         num = atoi(inp2);
         pref_data->extreme = num;
      }
      if (strcasecmp(inp1, "RoundMethod:") == 0)
      {
         num = atoi(inp2);
         pref_data->round = num;
      }
      if (strcasecmp(inp1, "HourFormat:") == 0)
      {
         num = atoi(inp2);
         pref_data->hformat = num;
      }

      if (strcasecmp(inp1, "DST-Start:") == 0)
         strcpy(pref_data->dst_start, inp2);

      if (strcasecmp(inp1, "DST-End:") == 0)
         strcpy(pref_data->dst_end, inp2);

      if (strcasecmp(inp1, "NearestLatitude:") == 0)
      {
         num = atof(inp2);
         pref_data->nearestLat = num;
      }
   }

   fclose(fp);

   return(0);
}

/**
   Get user's home-dir and RC-file with complete path
**/
char *
get_rcfilename()
{

   char *rcname         = NULL;
   char *home           = NULL;
   char *filepath       = NULL;

   /* Let's see if getting user's homedir will be easy */
   rcname       = PROG_RCFILE;
   home         = getenv("HOME");

   if (!home || !*home)
   {
      /* Easy didn't cut it, break-out the big guns */
      uid_t uid;
      struct passwd *p;

      /* Get user's real user ID */
      uid = getuid ();

      /* Get password entry */
      p = getpwuid (uid);

      if (!p || !p->pw_name || !*p->pw_name)
      {
         error(1, "%s: Couldn't get user info of uid \n");
      }
      else if (!p->pw_dir || !*p->pw_dir)
      {
         error(1, "Exiting, Couldn't get home directory of user\n");
      }
      else
      {
         home = p->pw_dir;
      }
   }

   /* Allocate memory to be safe not to trample on anything */
   filepath = (char *) malloc(strlen(home) + strlen(rcname) + 2);

   /* Make a safe copy */
   strcpy(filepath, home);

   /* Add-on the slash on the end if need be */
   if (!*home || home[strlen(home)-1] != '/')
      strcat(filepath, "/");
   strcat(filepath, rcname);

   return(filepath);
}


/**
   Initizlize various variables, read-in the RC file
**/
void
do_init_file(sPref *pref_data,
             Location *loc,
             Date *date)
{
   char *filename       = NULL;

   /* Initialize some struct members */
   pref_data->city              = (char *) NULL;
   pref_data->lat               = -1000;
   pref_data->lon               = -1000;
   pref_data->utc               = -99;
   pref_data->pressure          = -9999999;
   pref_data->temperature       = -9999999;
   pref_data->sealevel          = -1000;
   pref_data->dst               = -99;
   pref_data->angle_method      = -99;
   pref_data->fajrangle         = -99;
   pref_data->ishaaangle        = -99;
   pref_data->imsaakangle       = -99;
   pref_data->fajrinv           = -99;
   pref_data->ishaainv          = -99;
   pref_data->imsaakinv         = -99;
   pref_data->mathhab           = -99;   /* 1:Shaf'i, 2:Hanafi */
   pref_data->extreme           = -99;
   pref_data->round             = -99;
   pref_data->hformat           = -99;
   pref_data->nearestLat        = -99;
   loc->degreeLat               = -1000;
   loc->degreeLong              = -1000;
   loc->gmtDiff                 = -99;
   loc->dst                     = -99;
   
   date->year                   = (int) NULL;
   date->month                  = (int) NULL;
   date->day                    = (int) NULL;

   filename = get_rcfilename();

   process_file(pref_data, filename);
   free(filename);
}


void do_getenv(sPref *user_pref)
{
   char *ipt_data        = NULL;
   char ipt_city[50];
   char *city_name;
   double ipt_lat;
   double ipt_lon;
   double ipt_utc;
   int ipt_angle_method;

   ipt_data = getenv("IPT_DATA");

   if (!ipt_data || !*ipt_data)
   {
      return;
   }

   /* Get variable in the format of
      "CityName Latitude Longitude UTCdiff AngleMethod" an example of
      which can be "Abu Dhabi 24.4833 54.35 4 2"
   */
   sscanf(ipt_data, "%49[^0123456789.-] %lf %lf %lf %d",
          ipt_city, &ipt_lat, &ipt_lon, &ipt_utc, &ipt_angle_method);

   city_name = malloc(strlen(ipt_city)+1);
   strcpy(city_name, ipt_city);

   user_pref->city              = city_name;
   user_pref->lat               = ipt_lat;
   user_pref->lon               = ipt_lon;
   user_pref->utc               = ipt_utc;
   user_pref->angle_method      = ipt_angle_method;
}


/**
   Read-in the a user supplied input file
**/
void
do_input_file(sPref *pref_data,
              char *filename)
{
   if (filename != NULL)
   {
      /* If a user supplied a filename */
      if (process_file(pref_data, filename))
         error(1, "Exiting, Unable to process file specified with --file \n");
   }
   else
      error(1, "Exiting, invalid arugment to --file \n");
}


void
print_input_data(char *city_name,
                 Location *loc,
                 double qibla,
                 char *method_name)
{

   int deg, min;
   double sec;
   const char symb = (char)0xB0;

   printf("\n");
   printf("Prayer schedule for,\n");

   /* Print Cityname if there is something to print */
   if (city_name[0] != (char) NULL)
      printf(" City             : %s\n", city_name);

   decimal2Dms(loc->degreeLat, &deg, &min, &sec);
   printf(" Latitude         : %03d%c %02d\' %02d\" %c\n", abs(deg), symb,
          abs(min), abs(sec), (loc->degreeLat >=0 ? 'N' : 'S'));

   decimal2Dms(loc->degreeLong, &deg, &min, &sec);
   printf(" Longitude        : %03d%c %02d\' %02d\" %c\n", abs(deg), symb,
          abs(min), abs(sec), (loc->degreeLong >=0 ? 'E' : 'W'));

   printf(" Angle Method     : %s\n", method_name);

   printf(" TimeZone         : UTC%s%.1f\n",
          (loc->gmtDiff >= 0 ? "+" : ""), loc->gmtDiff);

   /* Deal with Qibla Information */
   qibla        = getNorthQibla(loc);
   decimal2Dms (qibla, &deg, &min, &sec);
   printf(" Qibla            : %03d%c %02d\' %02d\" %c of true North\n",
          abs (deg), symb, abs (min), abs (sec), (qibla >=0 ? 'W' : 'E'));

   printf("\n");

}

void
print_banner()
{
   /* Print a nice header so its all lined-up */
   printf("     Date         Fajr    Shorooq");
   printf("   Zuhr     Asr    Maghrib   Isha \n");
   printf("----------------------------------");
   printf("----------------------------------\n");
}



void
print_prayer_times(Location *loc,
                   Method *conf,
                   Date *date,
                   Prayer* ptList,
                   int print_brief,
                   int hformat)
{

   int i;

   /* Call the main function to fill the ptList array of Prayer struct */
   getPrayerTimes(loc, conf, date, ptList);

   /* Show the results */
   printf(" [%02d-%02d-%04d]", date->day, date->month, date->year);
   

#ifdef DEBUG
   for(i=0; i<6; i++)
      printf(" %2d:%02d:%02d%c", ptList[i].hour, ptList[i].minute,
             ptList[i].second, (ptList[i].isExtreme) ? '*' : ' ');
#else
   for(i=0; i<6; i++)
      printf("   %3d:%02d", (hformat == 12 && ptList[i].hour > 12) ?
             ptList[i].hour - 12 : ptList[i].hour,  ptList[i].minute);
#endif
   
   if (!print_brief)
      printf("\n");
}


int
date_in_range(Date* s, Date* e, Date* cur)
{
     
   if (cur->month > s->month && cur->month < e->month)
      return 1;
   else if (cur->month == s->month && s->month != e->month)
   {
      if (cur->day >= s->day)
         return 1;
   }
   else if (cur->month == e->month && s->month != e->month)
   {
      if (cur->day <= e->day)
         return 1;
   }
   else if (cur->month == e->month && cur->month == s->month)
   {
      /* Range in the same month! */
      if (cur->day <= e->day && cur->day >= s->day)
         return 1;
   }
   return 0;

}

int
timezone_dst(Date* date)
{
   time_t dsttime;
   struct tm tms = {0};

   tms.tm_mday = date->day;
   tms.tm_mon = date->month - 1;
   tms.tm_year = date->year - 1900;
   tms.tm_isdst = -1;

   dsttime = mktime(&tms);
   tms = *localtime(&dsttime);

   return tms.tm_isdst ? tms.tm_isdst : 0;

}

     

/**
   Main procedure
**/
int main(int argc, char *argv[])
{

   int i;
   int user_lat         = 0;
   int user_lon         = 0;
   int user_cmdline     = 0;
   int user_defined     = 0;
   int is_dst_range     = 0;
   int do_month         = 0;
   int do_rest_month    = 0;
   int do_year          = 0;
   int do_brief         = 0;
   int utc_timezone     = 0;  
   int angle_method     = 0;
   double qibla         = 0;
   /* Do we have to do this again? */
   int hformat          = -99;
   int round            = -99;
   int extreme          = -99;
   int dst              = -99;
   double utc_diff      = -99;
   double lat           = -1000;
   double lon           = -1000;
   double sea           = -1000;
   char city_name[50]   = "";
   int cal_num;
   int cal_dname[13]    = { 0, 31, 28, 31, 30, 31, 30,
                            31, 31, 30, 31, 30, 31 };
   char *cal_mname[13]  = { "skip", "January", "February", "March",
                            "April", "May", "June", "July", "August",
                            "September", "October", "November", "December" };
   char *method_name[8] = { "NONE",
                            "Egyptian General Authority of Survey",
                            "University of Islamic Sciences, Karachi (Shaf'i)",
                            "University of Islamic Sciences, Karachi (Hanafi)",
                            "Islamic Society of North America",
                            "Muslim World League (MWL)",
                            "Umm Al-Qurra University",
                            "Fixed Ishaa Angle Interval (always 90)"
   };

   time_t mytime;
   struct tm *t_ptr;

   /* Custom structures instantiation */
   Prayer       ptList[6];
   Prayer       nextImsaak;
   Prayer       nextFajr;
   Prayer       Imsaak;
   Location     loc;
   Method       conf;
   Date         date ;
   Date         dst_start;
   Date         dst_end;
   sPref        user_input;

   /* Read-in the init RC file */
   do_init_file(&user_input, &loc, &date);

   do_getenv(&user_input);

   /* Process command-line looking for -file argument */
   for (i = 1; i < argc; i++)
   {
      if (strcasecmp(argv[i], "-f") == 0 ||
          strcasecmp(argv[i], "-file") == 0 ||
          strcasecmp(argv[i], "--file") == 0)
      {
         do_input_file(&user_input, argv[i+1]);
      }
   }

   /* Process the rest of the command-line */
   for (i = 1; i < argc; i++)
   {
      if (strcasecmp(argv[i], "-h") == 0 ||
          strcasecmp(argv[i], "-help") == 0 ||
          strcasecmp(argv[i], "--help") == 0)
      {
         /* We really need a full-fledged help here */
         usage(1);
      }

      if (strcasecmp(argv[i], "-d") == 0 ||
          strcasecmp(argv[i], "--date") == 0)
      {
         if (argv[i+1] != NULL)
         {
            sscanf(&(argv[i + 1][0]), "%4d", &date.year);
            sscanf(&(argv[i + 1][4]), "%2d", &date.month);
            sscanf(&(argv[i + 1][6]), "%2d", &date.day);
         }
         else
         {
            error(1, "Exiting, invalid argument to --date \n");
         }
      }

      if (strcasecmp(argv[i], "-lat") == 0 ||
          strcasecmp(argv[i], "--latitude") == 0)
      {
         user_lat        = 1;
         if (argv[i+1] != NULL)
         {
            sscanf(&(argv[i + 1][0]), "%lf", &lat);
         }
         else
         {
            error(1, "Exiting, invalid argument to --latitude \n");
         }
      }  

      if (strcasecmp(argv[i], "-lon") == 0 ||
          strcasecmp(argv[i], "--longitude") == 0)
      {
         user_lon        = 1;
         if (argv[i+1] != NULL)
         {
            sscanf(&(argv[i + 1][0]), "%lf", &lon);
         }
         else
         {
            error(1, "[%s]: Invalid argument to --longitude \n");
         }
      }  

      if (strcasecmp(argv[i], "-u") == 0 ||
          strcasecmp(argv[i], "--utcdiff") == 0)
      {
         if (argv[i+1] != NULL)
         {
            sscanf(&(argv[i + 1][0]), "%lf", &utc_diff);
         }
         else
         {
            error(1, "Exiting, invalid argument to --utcdiff \n");
         }
      }

      if (strcasecmp(argv[i], "-a") == 0 ||
          strcasecmp(argv[i], "--anglemethod") == 0)
      {
         if (argv[i+1] != NULL)
         {
            sscanf(&(argv[i + 1][0]), "%d", &angle_method);
         }
         else
         {
            error(1, "Exiting, invalid argument to --method \n");
         }
      }  

      if (strcasecmp(argv[i], "-x") == 0 ||
          strcasecmp(argv[i], "--extreme") == 0)
      {
         if (argv[i+1] != NULL)
         {
            sscanf(&(argv[i + 1][0]), "%d", &extreme);
         }
         else
         {
            error(1, "Exiting, invalid argument to --extreme \n");
         }
      }  

      if (strcasecmp(argv[i], "-r") == 0 ||
          strcasecmp(argv[i], "--round") == 0)
      {
         if (argv[i+1] != NULL)
         {
            sscanf(&(argv[i + 1][0]), "%d", &round);
         }
         else
         {
            error(1, "Exiting, invalid argument to --round \n");
         }
      }  

      if (strcasecmp(argv[i], "-sea") == 0 ||
          strcasecmp(argv[i], "--sea-level") == 0)
      {
         if (argv[i+1] != NULL)
         {
            sscanf(&(argv[i + 1][0]), "%lf", &sea);
         }
         else
         {
            error(1, "Exiting, invalid argument to --sea \n");
         }
      }
          
      if (strcasecmp(argv[i], "-s") == 0 ||
          strcasecmp(argv[i], "--dst") == 0)
      {
         if (argv[i+1] != NULL)
         {
            sscanf(&(argv[i + 1][0]), "%d", &dst);
         }
         else
         {
            error(1, "Exiting, invalid argument to --dst \n");
         }
      }  

      if (strcasecmp(argv[i], "-ss") == 0 ||
          strcasecmp(argv[i], "--dst-start") == 0)
      {
         if (argv[i+1] != NULL)
         {
            is_dst_range = 1;
            sscanf(&(argv[i + 1][0]), "%4d", &dst_start.year);
            sscanf(&(argv[i + 1][4]), "%2d", &dst_start.month);
            sscanf(&(argv[i + 1][6]), "%2d", &dst_start.day);
         }
         else
         {
            error(1, "Exiting, invalid argument to --dst-start \n");
         }
      }

      if (strcasecmp(argv[i], "-se") == 0 ||
          strcasecmp(argv[i], "--dst-end") == 0)
      {
           
         if (argv[i+1] != NULL) 
         {
            is_dst_range = 1;
            sscanf(&(argv[i + 1][0]), "%4d", &dst_end.year);
            sscanf(&(argv[i + 1][4]), "%2d", &dst_end.month);
            sscanf(&(argv[i + 1][6]), "%2d", &dst_end.day);
         }
         else
         {
            error(1, "Exiting, invalid argument to --dst-end \n");
         }
      }

      if (strcasecmp(argv[i], "-h12") == 0 ||
          strcasecmp(argv[i], "--regular-hour") == 0)
      {
         hformat = 12;
      }  
      
      /* Do entire month, current or specified */
      if (strcasecmp(argv[i], "-m") == 0 ||
          strcasecmp(argv[i], "--month") == 0)
      {
         do_month = 1;
         if (argv[i+1] != NULL)
         {
            sscanf(&(argv[i + 1][0]), "%d", &date.month);
         }
      }

      /* Do entire year, current or specified */
      if (strcasecmp(argv[i], "-y") == 0 ||
          strcasecmp(argv[i], "--year") == 0)
      {
         do_year = 1;
         if (argv[i+1] != NULL)
         {
            sscanf(&(argv[i + 1][0]), "%d", &date.year);
         }
      }

      /* Do till end of current month */
      if (strcasecmp(argv[i], "-e") == 0 ||
          strcasecmp(argv[i], "--end") == 0)
      {
         do_rest_month = 1;
      }

      /* Do a brief print-out for single mode */
      if (strcasecmp(argv[i], "-b") == 0 ||
          strcasecmp(argv[i], "--brief") == 0)
      {
         do_brief = 1;
      }

   }

   /* Note if we had user defined location */
   user_cmdline = (user_lat || user_lon);
   user_defined = (user_cmdline ||
                   (user_input.lat != -1000) ||
                   (user_input.lon != -1000)
                   );

   /* - USER SPECIFIED SETTINGS - */
   /* Fill the city name */
   if (user_input.city && !user_cmdline)
   {
      sprintf(city_name, user_input.city);
      free(user_input.city);
   }
   else
   {
      if (user_defined)
         city_name[0] = (char) NULL;
      else
         sprintf(city_name, "Makkah");
   }

   /* Fill the Date structure - just in case */
   time(&mytime);
   
   t_ptr        = localtime(&mytime);
   
#ifdef HAVE_TM_GMTOFF
   utc_timezone = (t_ptr->tm_gmtoff / 3600);
#else
   /* Solaris doesn't seem to have this member, improvise */
   utc_timezone = (timezone / -3600);
#endif

   /* see if the date strucutre has been set to a value */
   if (date.day == (int) NULL)
      date.day   = t_ptr->tm_mday;
   if (date.month == (int) NULL)
      date.month = t_ptr->tm_mon  + 1;
   if (date.year == (int) NULL)
      date.year  = t_ptr->tm_year + 1900;
  
   /* Fill the location info. structure */
   /* If there are no command-line options, grab
      whatever user specified, else go default.
   */
   if (lat == -1000)
   {
      if (user_input.lat != -1000)
         loc.degreeLat  = user_input.lat;
      else
         loc.degreeLat  = 21.4206;
   } else loc.degreeLat = lat;
   

   if (lon == -1000)
   {
      if (user_input.lon != -1000)
         loc.degreeLong = user_input.lon;
      else
         loc.degreeLong = 39.8304;
   } else loc.degreeLong = lon;
   

   /* If there are no command-line timezone option, grab whatever
      user specified, else see if latitude/longitude were set and
      grab machine's timezone, else go default.
   */
   if (utc_diff == -99)
   {
      if (user_input.utc != -99)
         loc.gmtDiff    = user_input.utc;
      else
         loc.gmtDiff    = (user_defined ? utc_timezone : 3);
   } else loc.gmtDiff = utc_diff;

   if (hformat == -99)
   {
      if (user_input.hformat != -99)
         hformat = user_input.hformat;
      else hformat = 24;
   }


   if (dst == -99)
   {
      if (user_input.dst != -99)
         loc.dst = user_input.dst;
      else
         dst  =  2;
   } else loc.dst = dst;

   if (!is_dst_range)
   {
      if (sscanf(user_input.dst_start, "%4d%2d%2d", &dst_start.year,
                 &dst_start.month, &dst_start.day) == 3) {
         is_dst_range = 1;
      }

      if (sscanf(user_input.dst_end, "%4d%2d%2d", &dst_end.year,
                 &dst_end.month, &dst_end.day) == 3) {
         is_dst_range = 1;
      }         
   }

   if (sea == -1000)
   {
      if (user_input.sealevel != -1000)
         loc.seaLevel = user_input.sealevel;
      else
         loc.seaLevel =  0;
   } else loc.seaLevel = sea;

   if (user_input.pressure != -9999999)
         loc.pressure = user_input.pressure;
   else loc.pressure = 1010;

   if (user_input.temperature != -9999999)
         loc.temperature = user_input.temperature;
   else loc.temperature = 10;
   
   if (angle_method == 0)
   {
      if (user_input.angle_method != -99)
         angle_method   = user_input.angle_method;
      else
         angle_method   = (angle_method ? angle_method : 6);
   }
   
   /* Inact some pre-defined calculation methods/settings */
   getMethod(angle_method, &conf);
   
   /* Fill the Calculation method structure (user defined values) */
   if (user_input.fajrangle != -99)
      conf.fajrAng         = user_input.fajrangle;
   if (user_input.ishaaangle != -99)
      conf.ishaaAng        = user_input.ishaaangle;
   if (user_input.imsaakangle != -99)
      conf.imsaakAng       = user_input.imsaakangle;
   if (user_input.fajrinv != -99)
      conf.fajrInv         = user_input.fajrinv;
   if (user_input.ishaainv != -99)
      conf.ishaaInv        = user_input.ishaainv;
   if (user_input.imsaakinv != -99)
      conf.imsaakInv       = user_input.imsaakinv;
   if (user_input.mathhab != -99)
      conf.mathhab         = user_input.mathhab;
   if (user_input.extreme != -99)
      conf.extreme         = user_input.extreme;
   if (user_input.round != -99)
      conf.round         = user_input.round;
   if (user_input.nearestLat != -99)
      conf.nearestLat       = user_input.nearestLat;

   /* Override all other settings with what is passed by the command
    * line */
   if (extreme != -99)
      conf.extreme = extreme;
   if (round != -99)
      conf.round = round;

   /* Determine today's DST status and set the DST library flag
    * (loc.dst) */
   if (dst == 2)
   {
      if (is_dst_range)
         loc.dst = date_in_range(&dst_start, &dst_end, &date);
      else loc.dst = timezone_dst(&date);
   } 
   
   /* Echo back data used */
   if (!do_brief)
      print_input_data(&city_name[0], &loc, qibla, method_name[angle_method]);

   /* Determine number of days in month */
   if (do_rest_month || do_month || do_year)
   {
      int start_day;

      /* See which day/month to start on */
      start_day         = (do_year ||  do_month ? 1 : date.day);
      date.month        = (do_year && !do_month ? 1 : date.month);

      do
      {
         if (do_year && !do_month)
         {
            char *dashes;
            int len;

            /* Set length of box per month name + year + spaces */
            len = strlen(cal_mname[date.month])+8;

            dashes = malloc(len);
            strncpy(dashes, "+---------------------------", len);

            /* strncpy doesn't add the null-terminator, add it by hand */
            dashes[len] = '\0';
            strcat(dashes, "+");

            /* print-out the box */
            printf("                           %s\n", dashes);
            printf("                           | %s %04d |\n",
                   cal_mname[date.month], date.year);
            printf("                           %s\n\n", dashes);
            free(dashes);
         }

         /* Print head banner */
         print_banner();

         /* Get number of days in current month */
         cal_num = cal_dname[date.month];

         /*
           Leap year determination.
           - Rule 1: If the year is divisible by 400, it IS a leap year.
           - Rule 2: If the year is divisible by 100, it IS NOT a leap year.
           - Rule 3: If the year is divisible by 4, it IS a leap year.
         */
         if (date.month == 2)
         {      /* Good ole February - ain't you special :-) */
            cal_num += ( ((date.year % 4 == 0) && (date.year % 100)) ||
                         (date.year % 400 == 0) );
         }

         /* Do da-month Loop :-) */
         for (date.day = start_day; (date.day <= cal_num); date.day++)
         {
            /* Get dst status of today */
            if (dst == 2)
            {
               if (is_dst_range)
                  loc.dst = date_in_range(&dst_start, &dst_end, &date);
               else loc.dst = timezone_dst(&date);
            }
                
            print_prayer_times(&loc, &conf, &date, ptList, 0,  hformat);
         }

         if (do_year)
            printf("\n");

      } while (do_year && !do_month && date.month++ <= 11);
   }
   else
   {
      /* Print head banner */
      print_banner();
      
      /* Doing a single day */
      print_prayer_times(&loc, &conf, &date, ptList, do_brief, hformat);

      if (!do_brief)
      {
         getNextDayImsaak(&loc, &conf, &date, &nextImsaak);
         getNextDayFajr(&loc, &conf, &date, &nextFajr);
         getImsaak(&loc, &conf, &date, &Imsaak);
          
         printf("\n");
         printf("Today's Imsaak    : %3d:%02d\n", Imsaak.hour,
                Imsaak.minute);
         printf("Tomorrow's Imsaak : %3d:%02d\n", nextImsaak.hour,
                nextImsaak.minute);
         printf("Tomorrow's Fajr   : %3d:%02d\n", nextFajr.hour,
                nextFajr.minute);
      }
   }
 
   printf("\n");

   return(0);
}
