#!/usr/bin/perl

#---
# $Id: ireminder.pl,v 1.4 2004/08/18 22:09:16 nadim Exp $
#
# ------------
# Description:
# ------------
#  Copyright (c) 2004, Arabeyes, Nadim Shaikli
#
#  This is a prayer (and other Islamic events) reminder program.
#  Its a wrapper that uses Arabeyes.org's ITL package (ipraytime
#  to be specific) to attain all its values.  The script is meant
#  to be light, simple and flexible.
#
#  Based on a concept/idea from
#  - Ahmed El-Mahmoudy (aelmahmoudy -at- users sf net)
#
# -----------------
# Revision Details:     (Updated by Revision Control System)
# -----------------
#  $Date: 2004/08/18 22:09:16 $
#  $Author: nadim $
#  $Revision: 1.4 $
#  $Source: /home/arabeyes/cvs/projects/itl/programs/itools/ireminder.pl,v $
#
#  (www.arabeyes.org - under GPL license)
#---

# TODO:
# - have it fork in the background if a similar process doesn't already
#   exist else kill the other process post a confirm (optionally)
# - have it print to the console not stdout (optionally)
# - clean-up the sound/bell stuff
# - have it be able to play reminder and Azan sounds (optionally)
# - have it spawn a GUI reminder/now dialogs (optionally)

use Getopt::Long;

use FileHandle;
STDOUT->autoflush(1);

##
# Specify global variable values and init some Variables
$in_script	= $0;
$in_script	=~ s|^.*/([^/]*)|$1|;

##
# Find how many spaces the scripts name takes up (nicety)
$in_spaces	= $in_script;
$in_spaces	=~ s/\S/ /g;

$version	= " (v - %I%)";

# Process the command line
&get_args();

# Make it simple, does user need help ?
if ($opt{help}) { &help(); }

# Account for the various ways a user might enter a list
%opt		= %{&listify_input(\%opt, "skip")};
%opt		= %{&listify_input(\%opt, "reminder")};

$ipraytime	= $opt{ipraytime}	|| "ipraytime";
@reminders	= @{$opt{reminder}} ? @{$opt{reminder}} : ( 1, 5, 10, 20);

if (!-x $ipraytime)
{
    die "$ID ERROR - can't find $ipraytime \n";
}

# Enumerate the events we'll track and report on
$ID		= "[$in_script]";
$DAY_TOT_SEC	= (24 * 60 * 60);
@EVENTS		= (qw/imsaak fajr shorooq zuhr asr maghrib isha/);

# Set some defaults
$doing_2moro	= 0;
$time_to_use	= time();

# Continuously cycle through
for (;;)
{
    # Get current start time
    $ref_time_used	= &get_time($time_to_use);
    %time_used		= %{$ref_time_used};

    # Specify date to get info on
    $date_to_use	= sprintf( "%d%02d%02d",
				   $time_used{year},
				   $time_used{month},
				   $time_used{day} );

    # Run the actual program executable and process its output
    $ref_today_events	= &get_ipraytime($date_to_use);
    %today_events	= %{$ref_today_events};

    # Cycle through all the prayers (and events)
    for ($i = 0; $i <= $#EVENTS; $i++)
    {
	# Skip any events a user might have noted on the command-line
 	if ( defined $opt{skip} )
 	{
 	    foreach my $option (@{$opt{skip}})
 	    {
		# Make sure to make the compare case insensitive
 		if ( uc($option) eq uc($EVENTS[$i]) )
		{
		    $skip_event	= 1;
		    last;
		}
 	    }

	    # If a skip-hit occured, cycle out of the for loop
	    if ($skip_event)
	    {
		$skip_event	= 0;
		next;
	    }
 	}

	# Determine current time's placement within event times
	$before_last	= ( ($time_used{hour} <
			     $today_events{$EVENTS[$#EVENTS]}{hour}) ||
			    (($time_used{hour} ==
			      $today_events{$EVENTS[$#EVENTS]}{hour}) &&
			     ($time_used{min} <
			      $today_events{$EVENTS[$#EVENTS]}{min})) );
	$after_current	= ( ($time_used{hour} >
			     $today_events{$EVENTS[$i]}{hour}) ||
			    (($time_used{hour} ==
			      $today_events{$EVENTS[$i]}{hour}) &&
			     ($time_used{min} >=
			      $today_events{$EVENTS[$i]}{min})) );

	# If we are dealing with next day (upon start), cycle through
	if (!$before_last && !$doing_2moro) { last; }

	# Skip the past while making sure we didn't go over
	if ($before_last && $after_current)
	{
	    next;
	}
	else
	{
	    # If I'm here, then its immaterial which day I'm dealing with
	    $doing_2moro	= 0;

	    # See how many seconds we have left till next prayer
	    if ($before_last)
	    {
		# Get time difference between prayer and wallclock time
		$sec_diff	= ( $today_events{$EVENTS[$i]}{tot_sec} -
				    $time_used{tot_sec} );
	    }
	    else
	    {
		# Dealing with next day prayer
		# - Get portion of day left in today + tomorrow's
		$sec_diff	= ( ($DAY_TOT_SEC - $time_used{tot_sec}) +
				    $today_events{$EVENTS[$i]}{tot_sec} );
	    }		    

	    # Account for the reminder time and the time delta seconds
	    $sec_sleep	= ( $sec_diff - $time_used{sec} );

	    # Tell user what to expect next
	    $str	= "$ID NOTE - Next event '$EVENTS[$i]' is at ";
	    $str	.= "$today_events{$EVENTS[$i]}";
	    &display($str);

	    # Sort numerically descending
	    foreach $reminder (sort {$b <=> $a} @reminders)
	    {
		$sec_reminder	= ( $reminder * 60 );
		if ($sec_sleep < $sec_reminder)
		{
		    next;
		}

		$sec_sleep	-= $sec_reminder;

		# Take a nap and wait :-)
		&do_reminder($EVENTS[$i],
			     $sec_sleep,
			     \%today_events,
			     $reminder);

		$sec_sleep	= $sec_reminder;
	    }

	    # Take a nap and wait (without reminders :-)
	    &do_reminder($EVENTS[$i],
			 $sec_sleep,
			 0,
			 0);

	    &do_alarm($EVENTS[$i], \%today_events);

	    # Establish new time for next prayer
	    # - This is opted for instead of accounting for passed sec's
	    $ref_time_used	= &get_time(time());
	    %time_used		= %{$ref_time_used};
	}
    }

    # Cycle through & advance calc 1 day forward (ie. look at next day)
    $time_to_use	= ( time() + $DAY_TOT_SEC );
    $doing_2moro	= 1;
}

       ###        ###
######## Procedures ########
       ###        ###

##
# Print on screen passed-in string
sub display
{
    my ($string)	= @_;

    # Do some clean-up and backtrack per use command-line directives
    if ($opt{inplace})
    {
	print "\r";
	print " " x 78;
	print "\r";
    }
    print $string;

    if (!$opt{inplace})
    {
	print "\n";
    }
}

##
# Adjust user input to end-up with a proper array
sub listify_input
{
    my ($ref_in_opt,
	$in_str)	= @_;

    my (
	%in_opt,
	@in_arr,
	@mod_arr,
       );

    %in_opt		= %{$ref_in_opt};
    @in_arr		= @{$in_opt{$in_str}};

    # Make sure to accept for instance,
    # 1. -skip one -skip two
    # 2. -skip "one two"
    # 3. -skip "one, two"
    foreach my $string (@in_arr)
    {
	if ($string =~ /\s+/)
	{
	    my @sub_string	= split(/[,]*\s+/, $string);
	    push (@mod_arr, @sub_string);
	    next;
	}
	push (@mod_arr, $string);
    }

    $in_opt{$in_str}	= \@mod_arr;
    return(\%in_opt);
}

##
# Populate time hash/struct using the passed-in time value
sub get_time
{
    my ($in_time)	= @_;

    my (
	%tm,
       );

    # seconds, minutes, hours, day, month, year, day.week, day.year, DST
    ($tm{sec},
     $tm{min},
     $tm{hour},
     $tm{day},
     $tm{month},
     $tm{year},
     $tm{dayofweek},
     $tm{dayofyear},
     $tm{dst})		= localtime($in_time);

    # Some reability corrections
    $tm{month}		= ($tm{month} + 1);
    $tm{year}		= (1900 + $tm{year});

    # Set some needed values
    $tm{tot_min}	= ($tm{min} + (60 * $tm{hour}));
    $tm{tot_sec}	= (60 * $tm{tot_min});

    return(\%tm);
}

##
# Populate event hash/struct based on ipraytime's output using date passed-in
sub get_ipraytime
{
    my ($in_date)	= @_;

    my (
	$aux,
	@out_ipray,
	$ipray,
	@times,
       );

    if (defined $in_date)
    {
	$aux	= "--date $in_date ";
    }

    @out_ipray	= `$ipraytime $aux`;

    #     Date         Fajr    Shorooq   Zuhr     Asr    Maghrib   Isha 
    #--------------------------------------------------------------------
    # [22-07-2004]     5:22     6:50    13:27    16:43    20:04    21:34
    #
    #Today's Imsaak    :   5:17

    foreach my $line (@out_ipray)
    {
	# Remove the carriage returns
	chomp($line);

	# Find the proper line to process
	if ($line =~ /\[\d+-\d+-\d+\]/)
	{
	    @entries		= split (/\s\s+/, $line);
	    $ipray{fajr}	= $entries[1];
	    $ipray{shorooq}	= $entries[2];
	    $ipray{zuhr}	= $entries[3];
	    $ipray{asr}		= $entries[4];
	    $ipray{maghrib}	= $entries[5];
	    $ipray{isha}	= $entries[6];
	}

	# Grab today's imsaak time
	if ($line =~ /^Today.*Imsaak\s*:\s*(\d+:\d+)\s*/)
	{
	    $ipray{imsaak}	= $1;
	}
    }

    # Get the total number of seconds for each prayer and event
    for (my $i = 0; $i <= $#EVENTS; $i++)
    {
	@times				= split(/\:/, $ipray{$EVENTS[$i]});
	$ipray{$EVENTS[$i]}{hour}	= $times[0];
	$ipray{$EVENTS[$i]}{min}	= $times[1];
	$ipray{$EVENTS[$i]}{tot_min}	= ($ipray{$EVENTS[$i]}{min} +
					   (60 * $ipray{$EVENTS[$i]}{hour}));
	$ipray{$EVENTS[$i]}{tot_sec}	= (60 * $ipray{$EVENTS[$i]}{tot_min});

    }

    # Return the newly populated hash/structure
    return (\%ipray);
}

##
# Trigger a reminder
sub do_reminder
{
    my ($in_name,
	$in_time,
	$ref_event_struct,
	$in_rem)		= @_;

    my (
	$name,
	$str,
       );

    %event_struct	= %{$ref_event_struct};
    $name		= ucfirst($in_name);

    # Take a nap and wait
    sleep($in_time);
    
    # See if a reminder note is warranted
    if ($in_rem > 0)
    {

      NAME_CASE:
	{
	    if ($in_name eq "imsaak")
	    {
		$str = "Fasting is to commence ($event_struct{$in_name})";
		last NAME_CASE;
	    }
	    if ($in_name eq "shorooq")
	    {
		$str = "Salat Al-Fajr expires ($event_struct{$in_name})";
		last NAME_CASE;
	    }
	    #default
	    $str = "Salat Al-$name ($event_struct{$in_name})";
	}

	&play_sound();
	&display("$ID REMINDER - $str in $in_rem minutes !!");
    }
}

##
# Trigger an alarm
sub do_alarm
{
    my ($in_name,
	$ref_event_struct)	= @_;

    my (
	$name,
	$event_struct,
	$num_rings,
	$str,
       );

    $name		= ucfirst($in_name);
    %event_struct	= %{$ref_event_struct};
    $num_rings		= 4;

  NAME_CASE:
    {
	if ($in_name eq "imsaak")
	{
	    $str	= "to start your Fast";
	    last NAME_CASE;
	}
	if ($in_name eq "shorooq")
	{
	    $str	= "notes passing of Salat Al-Fajr";
	    last NAME_CASE;
	}
	#default
	$str		= "for Salat Al-$name";
    }

    &display("$ID $event_struct{$in_name} - Time NOW $str !!");

    for(my $i = 0; $i < $num_rings; $i++)
    {
	&play_sound();
	sleep(1);
    }
}

##
# Play a sound file (if possible) else gimme a beep :-)
sub play_sound
{
    print "\a";
}

##
# Print short usage info
sub usage
{
    my ($die_after)	= @_;

    print qq
|Usage: $in_script [-ipraytime path]
       $in_spaces [-skip event_name]
       $in_spaces [-reminder minutes]
       $in_spaces [-inplace]
       $in_spaces [-help]
|;
    if ( $die_after ) { exit(5); }
}

##
# Print one-liner help
sub help
{
    &usage(0);

    print qq|
-> remind user visually and via audio of Islamic prayer times & misc events

  Options:

    [-ipraytime path]   : Specify the executable path to program
    [-skip event_name]  : Specify name of prayer/event to skip (list ok)
    [-reminder minutes] : Specify prior to how many minutes to remind (list ok)
    [-inplace]          : Specify to print output in-place (without scrolling)
    [-help]             : Produce this help screen

|;
    exit(10);
}

##
# Get the command line arguments
sub get_args
{
    &GetOptions(
		\%opt,		# Hash to store all input options in
		"ipraytime=s",	# specify the binary ipraytime executable
		"skip=s@",	# array'ed list of skip events
		"reminder=s@",	# array'ed list of reminder times (minutes)
		"inplace",	# specify to print in-place outputs (no scroll)
		"help",		# print a brief help screen
		) || ( $? = 257, die"$ID ERROR - Invalid argument\n" );
}
