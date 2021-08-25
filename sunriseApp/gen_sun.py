import astral
import os
import math
import datetime
import pytz
import moon
import ephem

city_name = "Hartford"

fname = "sun.h"
f = open(fname, 'w')
headerText = """
// This header contains the sunrise, sunset, moonrise and moonset times for """ + city_name
headerText  = headerText + ".\n\n\n"
f.write(headerText)



a = astral.Astral()

a.solar_depression = "civil"

city = a[city_name]
#import pdb;pdb.set_trace()
latitude = str(city.latitude)
longitude = str(city.longitude)

ephemCity = ephem.Observer()
ephemCity.lat, ephemCity.lon = latitude, longitude
#ephemCity.pressure = 0
#ephemCity.horizon = '-0.34'


#Going to generate a list. We are going to calculate the date in th form if day of th e year. 
#To do this we will add each of the number of days of the month for whichever month that we are in. 
#We will also account for the leap years directly. Which means we will need to rewrite this every so often. 

#Sunrises always happen between the hours of 0500 and 0800. 3 hours, 180 minutes. 
#Sunrise will be the number of minutes after 0500 each day. 


NYC_TIME = pytz.timezone('America/New_York')

numDays = [31,28,31,30,31,30,31,31,30,31,30,31]
leapYears = [2016,2020,2024,2030]
firstYear = 2016
numYears = 7
# Lets print out the number of days in each month and what the leap years to
# the file, as well as the starting year
numDaysStr = "static const uint8_t month_days[12] = {" + ", ".join(str(num) for num in numDays) + "};\n"
leapYearsStr = "#define NUM_LEAP_YEARS " + str(len(leapYears)) + "\n"
leapYearsStr = leapYearsStr + "static const int leap_years[" + str(len(leapYears)) + "] = {" + ", ".join(str(num) for num in leapYears) + "};\n"

sunriseStr = "// The sunrise times are noted in minutes past 0500 for that day. \n"
sunriseStr = sunriseStr + "// These times take into account daylight savings time. \n "
sunriseStr = sunriseStr + "static const uint8_t sunrise_times[] = {"

sunsetStr = "// The sunset times are noted in minutes past 1645 for that day. \n"
sunsetStr = sunsetStr + "// These times take into account daylight savings time. \n "
sunsetStr = sunsetStr + "static const uint8_t sunset_times[] = {"


moonriseStr = "// The moonrise times are noted in tenths of hours after midnight\n"
moonriseStr = moonriseStr + "// These times take into account daylight savings time. \n "
moonriseStr = moonriseStr + "static const uint8_t moonrise_times[] = {"

moonsetStr = "// The moonset times are noted in tenths of hours after midnight\n"
moonsetStr = moonsetStr + "// These times take into account daylight savings time. \n "
moonsetStr = moonsetStr + "static const uint8_t moonset_times[] = {"

moonIllumFracStr = "// The moon illumination percentages are the percent that the moon is illuminated\n"
moonIllumFracStr = moonIllumFracStr + "// If the percentage is > 100, then the phase is increasing, < 100, decreasing. Just subtract 101 if increasing\n "
moonIllumFracStr = moonIllumFracStr + "static const uint8_t moon_illum_frac[] = {"
previousMoonPhase = 0

#Set up the first moonset data that we save
ephemCity.date = ephem.Date(datetime.datetime(firstYear, 1,1))
#print(ephemCity.date)
moon = ephem.Moon(ephemCity)
previousMoonset = ephemCity.next_setting(moon)
previousMoonset = ephem.localtime(previousMoonset)
previousSixMinutes = (previousMoonset.time().hour * 10) + (previousMoonset.time().minute / 6)
previousSixMinutes = math.ceil(previousSixMinutes)


dayNum = -1
for year in range(firstYear, firstYear + numYears):
    for month in range(0,12):
        if month == 1 and year in leapYears:
            days = 29
        else:
            days = numDays[month]
        for day in range(0,days):
            dayNum = dayNum + 1;
            date = datetime.date(year, month + 1, day + 1)
            dateTime = datetime.datetime(year, month + 1, day +1)
            #print(str(date))
            sun = city.sun(date, local=True)
            

            ephemCity.date = ephem.Date(date)
            #print(ephemCity.date)
            moon = ephem.Moon(ephemCity)
            ephemSun = ephem.Sun(ephemCity)
            moonPhase = moon.phase
            #import pdb;pdb.set_trace()
            if moonPhase > previousMoonPhase:
                moonPhase = moonPhase + 101
            moonPhase = round(moonPhase)
            moonIllumFracStr  = moonIllumFracStr + str(moonPhase) + ", "
            previousMoonPhase = moon.phase

            moonrise = ephemCity.previous_rising(moon)
            moonrise = ephem.localtime(moonrise)

            #minutes = (delta.total_seconds()/(60*6))
            sixMinutes = (moonrise.time().hour * 10) + (moonrise.time().minute / 6)
            sixMinutes = math.ceil(sixMinutes)
            #print("moonrise in tenths: " + str(math.ceil(sixMinutes)))
            #print("moonrise: " + str(moonrise))
            riseHours = math.floor(sixMinutes / 10)
            riseMinutes = (sixMinutes % 10 ) * 6
            #print("Calced rise: " + str(riseHours) + ":" + str(riseMinutes))
            #import pdb;pdb.set_trace()
            #print("SunriseA: " + str(ephemCity.next_setting(ephemSun)))
            moonriseStr = moonriseStr + str(sixMinutes) + ", "
            #print("        dayNum " + str(dayNum) + " moonrise minutes " + str(sixMinutes) + " calced rise " + str(riseHours) + ":" + str(riseMinutes) +  " time " + str(moonrise))

            moonset = ephemCity.next_setting(moon, start=date)
            moonset = ephem.localtime(moonset)

            #minutes = (delta.total_seconds()/(60*6))
            sixMinutes = (moonset.time().hour * 10) + (moonset.time().minute / 6)
            sixMinutes = round(sixMinutes)
            #print("moonset in tenths: " + str(math.ceil(sixMinutes)))
            #print("moonset: " + str(moonset))

            # We need to be fancy with the moonset. We need to save the previous days 
            # moonrise time, and use that if the resultant calculated moonrise is 
            # more than 1 day in the future. 
            
            if ((moonset.date() - moonrise.date()).total_seconds()) > 13*60*60:
                #Check if the moonset is later than the maximum amount of time
                # the moon can be in the sky, 12.5 hours.
                # Swap yesterday into today
                sixMinutes, previousSixMinutes = previousSixMinutes, sixMinutes
                moonset, previousMoonset = previousMoonset, moonset
                setHours = math.floor(sixMinutes / 10)
                setMinutes = (sixMinutes % 10 ) * 6
                #print("swapped dayNum " + str(dayNum) + " moonset minutes " + str(sixMinutes) + " calced rise " + str(setHours) + ":" + str(setMinutes) +  " time " + str(moonset))
            else:
                setHours = math.floor(sixMinutes / 10)
                setMinutes = (sixMinutes % 10 ) * 6
                #print("noswap  dayNum " + str(dayNum) + " moonset minutes " + str(sixMinutes) + " calced rise " + str(setHours) + ":" + str(setMinutes) +  " time " + str(moonset))

            #print("Calced set: " + str(setHours) + ":" + str(setMinutes))
            #import pdb;pdb.set_trace()
            #print("SunsetA: " + str(ephemCity.next_setting(ephemSun)))

            #print("dayNum " + str(dayNum) + " moonset minutes " + str(sixMinutes) + " calced rise " + str(setHours) + ":" + str(setMinutes) +  " time " + str(moonset))
            moonsetStr = moonsetStr + str(sixMinutes) + ", "




            sunrise = sun["sunrise"]
            earlyTime = datetime.time(5,00, tzinfo = NYC_TIME)
            earliestTime = datetime.datetime.combine(date, earlyTime) - sunrise.dst()
            delta = sunrise - earliestTime
            minutes = (delta.total_seconds()/60) - 5 #Needed to add a minus 5 because I have no idea. 
            minutes = math.ceil(minutes)
            #print("Sunrise: " + str(math.ceil(minutes)))
            #print("Sunrise: " + str(sunrise))
            sunriseStr = sunriseStr + str(minutes) + ", "

            sunset = sun["sunset"]
            earlyTime = datetime.time(16,45, tzinfo = NYC_TIME)
            earliestTime = datetime.datetime.combine(date, earlyTime) - sunset.dst()
            delta = sunset - earliestTime
            minutes = (delta.total_seconds()/60) -5 #Needed to add a minus 5 because I have no idea. 
            minutes = math.ceil(minutes)
            if (minutes <0):
                minutes = 0;
            #print("Sunset: " + str(math.ceil(minutes)))
            #print("Sunset: " + str(sunset))
            sunsetStr = sunsetStr + str(minutes) + ", "
sunriseStr = sunriseStr[:-2]
f.write(sunriseStr + "};\n")

sunsetStr = sunsetStr[:-2]
f.write(sunsetStr + "};\n")

moonriseStr = moonriseStr[:-2]
f.write(moonriseStr + "};\n")

moonsetStr = moonsetStr[:-2]
f.write(moonsetStr + "};\n")
f.write(numDaysStr)
f.write(leapYearsStr)
f.write("uint16_t first_year = " + str(firstYear) + ";\n")
f.write("uint8_t num_years = " + str(numYears) + ";\n")
moonIllumFracStr = moonIllumFracStr[:-2]
#f.write(moonIllumFracStr + "};\n")

f.close()
