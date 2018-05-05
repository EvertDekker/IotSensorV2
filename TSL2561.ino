void aquiretsl2561data(){
  good = tsl2561getdata ();
  
}

boolean tsl2561getdata(){
  uint16_t scaledFull = 0xffff, scaledIr = 0xffff;
  uint32_t full = 0xffffffff, ir = 0xffffffff, milliLux = 0xffffffff;
  gain = false;
  Tsl2561::exposure_t exposure = Tsl2561::EXP_OFF;

  if( Tsl2561Util::autoGain(Tsl, gain, exposure, scaledFull, scaledIr) ) {
    if( Tsl2561Util::normalizedLuminosity(gain, exposure, full = scaledFull, ir = scaledIr) ) {
      
      if( Tsl2561Util::milliLux(full, ir, milliLux, Tsl2561::packageCS(id), 5) ) {
        DEBUG_PRINT(format("Tsl2561 addr: 0x%02x, id: 0x%02x, sfull: %5u, sir: %5u, full: %5lu, ir: %5lu, gain: %d, exp: %d, lux: %5lu.%03lu\n",Tsl.address(), id, scaledFull, scaledIr, (unsigned long)full, (unsigned long)ir, gain, exposure, (unsigned long)milliLux/1000, (unsigned long)milliLux%1000));
      }
      else {
        DEBUG_PRINT(format("Tsl2561Util::milliLux(full=%lu, ir=%lu) error\n", (unsigned long)full, (unsigned long)ir));
        return false;
      }
    }
    else {
      DEBUG_PRINT(format("Tsl2561Util::normalizedLuminosity(gain=%u, exposure=%u, sfull=%u, sir=%u, full=%lu, ir=%lu) error\n", gain, exposure, scaledFull, scaledIr, (unsigned long)full, (unsigned long)ir));
      return false;
    }
  }
  else {
    DEBUG_PRINT(format("Tsl2561Util::autoGain(gain=%u, exposure=%u, sfull=%u, sir=%u) error\n", gain, exposure, scaledFull, scaledIr));
    return false;
  }
lux = milliLux;
lux= lux/1000;
expos= exposure;
return true;
}

// to mimic Serial.printf() of esp8266 core for other platforms
char *format( const char *fmt, ... ) {
  static char buf[128];
  va_list arg;
  va_start(arg, fmt);
  vsnprintf(buf, sizeof(buf), fmt, arg);
  buf[sizeof(buf)-1] = '\0';
  va_end(arg);
  return buf;
}
