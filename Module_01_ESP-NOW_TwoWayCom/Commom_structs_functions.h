/*
* Commom_structs_functions.h
*      
* --> 
*		
*/


/*************************
** struct RTC_Data      **
*************************/
struct rtc_data{
      uint16_t dy;
      uint8_t dm;
      uint8_t dd;
      uint8_t hh;
      uint8_t hm;
      uint8_t hs;
      float t;
};

// Type 'RTC_Data'
typedef struct rtc_data RTC_Data;




/*************************
** struct BMP280_Data  **
*************************/
struct bmp280_data {
      float t;
      float p;
};

// Type 'Dados_BMP280'
typedef struct bmp280_data BMP280_Data;






/*************************
** Auxiliary Functions  **
*************************/
String get_formatted_date( RTC_Data *p_str_data ){
      String str_rtc_date = "";

      str_rtc_date += String(p_str_data->dy) + "-";

      if( p_str_data->dm < 10 ){  str_rtc_date += "0";  }
      str_rtc_date += String(p_str_data->dm) + "-";

      if( p_str_data->dd < 10 ){  str_rtc_date += "0";  }
      str_rtc_date += String(p_str_data->dd);

      return  str_rtc_date;
}


String get_formatted_hour( RTC_Data *p_str_data){
      String str_rtc_hour = "";

      if( p_str_data->hh < 10 ){  str_rtc_hour += "0";  }
      str_rtc_hour += String(p_str_data->hh) + ":";

      if( p_str_data->hm < 10 ){  str_rtc_hour += "0";  }
      str_rtc_hour += String(p_str_data->hm) + ":";

      if( p_str_data->hs < 10 ){  str_rtc_hour += "0";  }
      str_rtc_hour += String(p_str_data->hs);

      // Update the 'p_str_data->str_hour' field
      return str_rtc_hour;
}
   
