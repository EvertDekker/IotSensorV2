void bme280init(){
    while(!bme.begin())
  {
    DEBUG_PRINTLN("Could not find BME280 sensor!");
    delay(1000);
  }

  // bme.chipID(); // Deprecated. See chipModel().
  switch(bme.chipModel())
  {
     case BME280::ChipModel_BME280:
       DEBUG_PRINTLN("Found BME280 sensor! Success.");
       break;
     case BME280::ChipModel_BMP280:
       DEBUG_PRINTLN("Found BMP280 sensor! No Humidity available.");
       break;
     default:
       DEBUG_PRINTLN("Found UNKNOWN sensor! Error!");
  }
}


void acquireBME280Data()
{
  
   BME280::TempUnit tempUnit(BME280::TempUnit_Celsius);
   BME280::PresUnit presUnit(BME280::PresUnit_hPa);

   bme.read(pressure, temperature, humidity, tempUnit, presUnit);

   DEBUG_PRINT("Temp: ");
   DEBUG_PRINT(temperature);
   DEBUG_PRINT(" °"+ String(tempUnit == BME280::TempUnit_Celsius ? 'C' :'F'));
   DEBUG_PRINT("\t\tHumidity: ");
   DEBUG_PRINT(humidity);
   DEBUG_PRINT("% RH");
   DEBUG_PRINT("\t\tPressure: ");
   DEBUG_PRINT(pressure);
   DEBUG_PRINTLN(" hPa");
   delay(40);
}

/*
enum TempUnit {
   TempUnit_Celsius,
   TempUnit_Fahrenheit
};

enum PresUnit {
   PresUnit_Pa,
   PresUnit_hPa,
   PresUnit_inHg,
   PresUnit_atm,
   PresUnit_bar,
   PresUnit_torr,
   PresUnit_psi
};

enum OSR {
   OSR_X1 =  1,
   OSR_X2 =  2,
   OSR_X4 =  3,
   OSR_X8 =  4,
   OSR_X16 = 5
};

enum Mode {
   Mode_Sleep  = 0,
   Mode_Forced = 1,
   Mode_Normal = 3
};

enum StandbyTime {
   StandbyTime_500us   = 0,
   StandbyTime_62500us = 1,
   StandbyTime_125ms   = 2,
   StandbyTime_250ms   = 3,
   StandbyTime_50ms    = 4,
   StandbyTime_1000ms  = 5,
   StandbyTime_10ms    = 6,
   StandbyTime_20ms    = 7
};

enum Filter {
   Filter_Off = 0,
   Filter_2   = 1,
   Filter_4   = 2,
   Filter_8   = 3,
   Filter_16  = 4
};

enum SpiEnable{
   SpiEnable_False = 0,
   SpiEnable_True = 1
};

enum ChipModel {
    ChipModel_UNKNOWN = 0,
    ChipModel_BMP280 = 0x58,
    ChipModel_BME280 = 0x60
};
*/
