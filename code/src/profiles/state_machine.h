void READ_EEPROM_ACTIVE_PROFILE();
void READ_EEPROM();
void heat_up(uint16_t& temp,uint16_t& time,uint16_t& timers_help, bool& cancel,bool& res);
void soak_up(uint16_t& temp,uint16_t& time,uint16_t& timers_help, bool& cancel,bool& res);
void ramp_up(uint16_t& temp,uint16_t& time, uint16_t& timers_help,bool& cancel,bool& res );
void DrawTempDisplayRef();
void DrawCancelBut();

