/* API response deserialization for esp32-weather-epd.
 * Copyright (C) 2022-2024  Luke Marzen
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <vector>
#include <cmath>
#include <algorithm>
#include <ArduinoJson.h>
#include "api_response.h"
#include "config.h"
#include "conversions.h"

DeserializationError deserializeOneCall(WiFiClient &json,
                                        owm_resp_onecall_t &r)
{
  int i;

  JsonDocument filter;
  filter["current"]  = true;
  filter["minutely"] = false;
  filter["hourly"]   = true;
  filter["daily"]    = true;
#if !DISPLAY_ALERTS
  filter["alerts"]   = false;
#else
  // description can be very long so they are filtered out to save on memory
  // along with sender_name
  for (int i = 0; i < OWM_NUM_ALERTS; ++i)
  {
    filter["alerts"][i]["sender_name"] = false;
    filter["alerts"][i]["event"]       = true;
    filter["alerts"][i]["start"]       = true;
    filter["alerts"][i]["end"]         = true;
    filter["alerts"][i]["description"] = false;
    filter["alerts"][i]["tags"]        = true;
  }
#endif

  JsonDocument doc;

  DeserializationError error = deserializeJson(doc, json,
                                         DeserializationOption::Filter(filter));
#if DEBUG_LEVEL >= 1
  Serial.println("[debug] doc.overflowed() : "
                 + String(doc.overflowed()));
#endif
#if DEBUG_LEVEL >= 2
  serializeJsonPretty(doc, Serial);
#endif
  if (error) {
    return error;
  }

  r.lat             = doc["lat"]            .as<float>();
  r.lon             = doc["lon"]            .as<float>();
  r.timezone        = doc["timezone"]       .as<const char *>();
  r.timezone_offset = doc["timezone_offset"].as<int>();

  JsonObject current = doc["current"];
  r.current.dt         = current["dt"]        .as<int64_t>();
  r.current.sunrise    = current["sunrise"]   .as<int64_t>();
  r.current.sunset     = current["sunset"]    .as<int64_t>();
  r.current.temp       = current["temp"]      .as<float>();
  r.current.feels_like = current["feels_like"].as<float>();
  r.current.pressure   = current["pressure"]  .as<int>();
  r.current.humidity   = current["humidity"]  .as<int>();
  r.current.dew_point  = current["dew_point"] .as<float>();
  r.current.clouds     = current["clouds"]    .as<int>();
  r.current.uvi        = current["uvi"]       .as<float>();
  r.current.visibility = current["visibility"].as<int>();
  r.current.wind_speed = current["wind_speed"].as<float>();
  r.current.wind_gust  = current["wind_gust"] .as<float>();
  r.current.wind_deg   = current["wind_deg"]  .as<int>();
  r.current.rain_1h    = current["rain"]["1h"].as<float>();
  r.current.snow_1h    = current["snow"]["1h"].as<float>();
  JsonObject current_weather = current["weather"][0];
  r.current.weather.id          = current_weather["id"]         .as<int>();
  r.current.weather.main        = current_weather["main"]       .as<const char *>();
  r.current.weather.description = current_weather["description"].as<const char *>();
  r.current.weather.icon        = current_weather["icon"]       .as<const char *>();

  // minutely forecast is currently unused
  // i = 0;
  // for (JsonObject minutely : doc["minutely"].as<JsonArray>())
  // {
  //   r.minutely[i].dt            = minutely["dt"]           .as<int64_t>();
  //   r.minutely[i].precipitation = minutely["precipitation"].as<float>();

  //   if (i == OWM_NUM_MINUTELY - 1)
  //   {
  //     break;
  //   }
  //   ++i;
  // }

  i = 0;
  for (JsonObject hourly : doc["hourly"].as<JsonArray>())
  {
    r.hourly[i].dt         = hourly["dt"]        .as<int64_t>();
    r.hourly[i].temp       = hourly["temp"]      .as<float>();
    r.hourly[i].feels_like = hourly["feels_like"].as<float>();
    r.hourly[i].pressure   = hourly["pressure"]  .as<int>();
    r.hourly[i].humidity   = hourly["humidity"]  .as<int>();
    r.hourly[i].dew_point  = hourly["dew_point"] .as<float>();
    r.hourly[i].clouds     = hourly["clouds"]    .as<int>();
    r.hourly[i].uvi        = hourly["uvi"]       .as<float>();
    r.hourly[i].visibility = hourly["visibility"].as<int>();
    r.hourly[i].wind_speed = hourly["wind_speed"].as<float>();
    r.hourly[i].wind_gust  = hourly["wind_gust"] .as<float>();
    r.hourly[i].wind_deg   = hourly["wind_deg"]  .as<int>();
    r.hourly[i].pop        = hourly["pop"]       .as<float>();
    r.hourly[i].rain_1h    = hourly["rain"]["1h"].as<float>();
    r.hourly[i].snow_1h    = hourly["snow"]["1h"].as<float>();
    // JsonObject hourly_weather = hourly["weather"][0];
    // r.hourly[i].weather.id          = hourly_weather["id"]         .as<int>();
    // r.hourly[i].weather.main        = hourly_weather["main"]       .as<const char *>();
    // r.hourly[i].weather.description = hourly_weather["description"].as<const char *>();
    // r.hourly[i].weather.icon        = hourly_weather["icon"]       .as<const char *>();

    if (i == OWM_NUM_HOURLY - 1)
    {
      break;
    }
    ++i;
  }

  i = 0;
  for (JsonObject daily : doc["daily"].as<JsonArray>())
  {
    r.daily[i].dt         = daily["dt"]        .as<int64_t>();
    r.daily[i].sunrise    = daily["sunrise"]   .as<int64_t>();
    r.daily[i].sunset     = daily["sunset"]    .as<int64_t>();
    r.daily[i].moonrise   = daily["moonrise"]  .as<int64_t>();
    r.daily[i].moonset    = daily["moonset"]   .as<int64_t>();
    r.daily[i].moon_phase = daily["moon_phase"].as<float>();
    JsonObject daily_temp = daily["temp"];
    r.daily[i].temp.morn  = daily_temp["morn"] .as<float>();
    r.daily[i].temp.day   = daily_temp["day"]  .as<float>();
    r.daily[i].temp.eve   = daily_temp["eve"]  .as<float>();
    r.daily[i].temp.night = daily_temp["night"].as<float>();
    r.daily[i].temp.min   = daily_temp["min"]  .as<float>();
    r.daily[i].temp.max   = daily_temp["max"]  .as<float>();
    JsonObject daily_feels_like = daily["feels_like"];
    r.daily[i].feels_like.morn  = daily_feels_like["morn"] .as<float>();
    r.daily[i].feels_like.day   = daily_feels_like["day"]  .as<float>();
    r.daily[i].feels_like.eve   = daily_feels_like["eve"]  .as<float>();
    r.daily[i].feels_like.night = daily_feels_like["night"].as<float>();
    r.daily[i].pressure   = daily["pressure"]  .as<int>();
    r.daily[i].humidity   = daily["humidity"]  .as<int>();
    r.daily[i].dew_point  = daily["dew_point"] .as<float>();
    r.daily[i].clouds     = daily["clouds"]    .as<int>();
    r.daily[i].uvi        = daily["uvi"]       .as<float>();
    r.daily[i].visibility = daily["visibility"].as<int>();
    r.daily[i].wind_speed = daily["wind_speed"].as<float>();
    r.daily[i].wind_gust  = daily["wind_gust"] .as<float>();
    r.daily[i].wind_deg   = daily["wind_deg"]  .as<int>();
    r.daily[i].pop        = daily["pop"]       .as<float>();
    r.daily[i].rain       = daily["rain"]      .as<float>();
    r.daily[i].snow       = daily["snow"]      .as<float>();
    JsonObject daily_weather = daily["weather"][0];
    r.daily[i].weather.id          = daily_weather["id"]         .as<int>();
    r.daily[i].weather.main        = daily_weather["main"]       .as<const char *>();
    r.daily[i].weather.description = daily_weather["description"].as<const char *>();
    r.daily[i].weather.icon        = daily_weather["icon"]       .as<const char *>();

    if (i == OWM_NUM_DAILY - 1)
    {
      break;
    }
    ++i;
  }

#if DISPLAY_ALERTS
  i = 0;
  for (JsonObject alerts : doc["alerts"].as<JsonArray>())
  {
    owm_alerts_t new_alert = {};
    // new_alert.sender_name = alerts["sender_name"].as<const char *>();
    new_alert.event       = alerts["event"]      .as<const char *>();
    new_alert.start       = alerts["start"]      .as<int64_t>();
    new_alert.end         = alerts["end"]        .as<int64_t>();
    // new_alert.description = alerts["description"].as<const char *>();
    new_alert.tags        = alerts["tags"][0]    .as<const char *>();
    r.alerts.push_back(new_alert);

    if (i == OWM_NUM_ALERTS - 1)
    {
      break;
    }
    ++i;
  }
#endif

  return error;
} // end deserializeOneCall

DeserializationError deserializeAirQuality(WiFiClient &json,
                                           owm_resp_air_pollution_t &r)
{
  int i = 0;

  JsonDocument doc;

  DeserializationError error = deserializeJson(doc, json);
#if DEBUG_LEVEL >= 1
  Serial.println("[debug] doc.overflowed() : "
                 + String(doc.overflowed()));
#endif
#if DEBUG_LEVEL >= 2
  serializeJsonPretty(doc, Serial);
#endif
  if (error) {
    return error;
  }

  r.coord.lat = doc["coord"]["lat"].as<float>();
  r.coord.lon = doc["coord"]["lon"].as<float>();

  for (JsonObject list : doc["list"].as<JsonArray>())
  {

    r.main_aqi[i] = list["main"]["aqi"].as<int>();

    JsonObject list_components = list["components"];
    r.components.co[i]    = list_components["co"].as<float>();
    r.components.no[i]    = list_components["no"].as<float>();
    r.components.no2[i]   = list_components["no2"].as<float>();
    r.components.o3[i]    = list_components["o3"].as<float>();
    r.components.so2[i]   = list_components["so2"].as<float>();
    r.components.pm2_5[i] = list_components["pm2_5"].as<float>();
    r.components.pm10[i]  = list_components["pm10"].as<float>();
    r.components.nh3[i]   = list_components["nh3"].as<float>();

    r.dt[i] = list["dt"].as<int64_t>();

    if (i == OWM_NUM_AIR_POLLUTION - 1)
    {
      break;
    }
    ++i;
  }

  return error;
} // end deserializeAirQuality

#if INTUITIVE_MIN_MAX_TEMPERATURES
/* Compute intuitive min/max temperatures for daily forecasts.
 * 
 * The OpenWeatherMap API returns min/max based on midnight-to-midnight, which
 * doesn't match how people typically think about daily temperatures.
 * 
 * This function recomputes:
 * - Min (overnight low): minimum temp from 4pm (16:00) to next day's sunrise
 * - Max (daytime high): maximum temp from sunrise to midnight
 * 
 * Uses hourly data when available (48 hours), falls back to daily temp values
 * (morn, day, eve, night) for days beyond hourly coverage.
 */
void computeIntuitiveMinMax(owm_resp_onecall_t &r)
{
  // Process each day (we display 5 days, indices 0-4)
  for (int day = 0; day < 5; ++day)
  {
    owm_daily_t &today = r.daily[day];

  #if DEBUG_LEVEL >= 1
    // Save original API values before modification
    float origMin = today.temp.min;
    float origMax = today.temp.max;
  #endif
    
    // Get sunrise time for today as the boundary for daytime
    int64_t todaySunrise = today.sunrise;
    
    // Get the start of today (midnight) in local time
    // daily.dt is typically noon of that day, so we calculate midnight
    int64_t todayNoon = today.dt;
    int64_t todayMidnight = todayNoon - (12 * 3600); // approximate midnight
    int64_t todayEndOfDay = todayMidnight + (24 * 3600); // next midnight
    
    // 4pm today for overnight low calculation
    int64_t today4pm = todayMidnight + (16 * 3600);
    
    // Next day's sunrise for overnight low endpoint
    int64_t nextDaySunrise;
    if (day + 1 < OWM_NUM_DAILY)
    {
      nextDaySunrise = r.daily[day + 1].sunrise;
    }
    else
    {
      // If we don't have next day data, estimate sunrise ~same time next day
      nextDaySunrise = todaySunrise + (24 * 3600);
    }
    
    // Variables to track min/max from hourly data
    float hourlyMax = -1e9f;  // Start with very low value
    float hourlyMin = 1e9f;   // Start with very high value
    bool foundMaxHourly = false;
    bool foundMinHourly = false;
    
    // Search through hourly data
    for (int h = 0; h < OWM_NUM_HOURLY; ++h)
    {
      int64_t hourTime = r.hourly[h].dt;
      float hourTemp = r.hourly[h].temp;
      
      // Check if this hour falls in the daytime high window (sunrise to midnight)
      if (hourTime >= todaySunrise && hourTime < todayEndOfDay)
      {
        if (hourTemp > hourlyMax)
        {
          hourlyMax = hourTemp;
          foundMaxHourly = true;
        }
      }
      
      // Check if this hour falls in the overnight low window (4pm to next sunrise)
      if (hourTime >= today4pm && hourTime < nextDaySunrise)
      {
        if (hourTemp < hourlyMin)
        {
          hourlyMin = hourTemp;
          foundMinHourly = true;
        }
      }
    }
    
    // Update max temperature
    if (foundMaxHourly)
    {
      today.temp.max = hourlyMax;
    }
    else
    {
      // Fall back to daily values: max of morn, day, eve (daytime hours)
      today.temp.max = std::max({today.temp.morn, today.temp.day, today.temp.eve});
    }
    
    // Update min temperature  
    if (foundMinHourly)
    {
      today.temp.min = hourlyMin;
    }
    else
    {
      // Fall back to daily values: min of eve, night from today
      // and morn from next day if available
      float fallbackMin = std::min(today.temp.eve, today.temp.night);
      if (day + 1 < OWM_NUM_DAILY)
      {
        fallbackMin = std::min(fallbackMin, r.daily[day + 1].temp.morn);
      }
      today.temp.min = fallbackMin;
    }

#if DEBUG_LEVEL >= 1
    Serial.println("[debug] Day " + String(day) + " temp adjustments:");
    Serial.println("  API Max: " + String(kelvin_to_fahrenheit(origMax), 1) + "F / " 
                   + String(kelvin_to_celsius(origMax), 1) + "C  ->  "
                   "New Max: " + String(kelvin_to_fahrenheit(today.temp.max), 1) + "F / "
                   + String(kelvin_to_celsius(today.temp.max), 1) + "C"
                   + (foundMaxHourly ? " (hourly)" : " (fallback)"));
    Serial.println("  API Min: " + String(kelvin_to_fahrenheit(origMin), 1) + "F / "
                   + String(kelvin_to_celsius(origMin), 1) + "C  ->  "
                   "New Min: " + String(kelvin_to_fahrenheit(today.temp.min), 1) + "F / "
                   + String(kelvin_to_celsius(today.temp.min), 1) + "C"
                   + (foundMinHourly ? " (hourly)" : " (fallback)"));
#endif
  }
} // end computeIntuitiveMinMax
#endif // INTUITIVE_MIN_MAX_TEMPERATURES

