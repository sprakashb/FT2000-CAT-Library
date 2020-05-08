
// Yaesu FT2000 CAT simulation : This is perhaps the first implementaion of FT2000 cmds in Arduino and could be improved significantly
// Not all commands are needed to have control from wsjtx, js8call, fldigi
// Therefore currently only essential commands are included. (May 2020 )
// to add more commands add the new 2 letter commands in the list below at : char* cmd_list[] = ..., and
// add call to related function in : switch (index) with case indicating the sequence in cmd_list (beginning 0)
// Related function can be added below ( we try to maintain sequence of commands in cmd_list)

// HOWTO: use this library by manipulating functions below to adjust rig paramenters in Rig Control Program
// Comments are placed appropriately to change rig-control program
// All rig-control examples below are from the TFT/Touch screen VFO/BFO program for uBitx
// more details - vu2spf.blogspot.in / github.com/sprakashb
// LICENSE : GPL3
// Author: VU2SPF, SP Bhatnagar,  (first release May 2020), Please add your credentials here if you improve this library.
//----------------------
//
// 
boool inCatCheck = false; // When waiting for command from Serial

//Radio IDs for different Yaesu radios using new Cat commands of 2 Characters (eg FA..), add your rig,  Uncomment /use only one from following
char * Radio_id = "ID0251;"  ;   // Yaesu FT-2000
// char * Radio_id = "ID0670;" ; // Yesu FT 991
// Other radio IDs may be found from their respective CAT manuals

char Rx_buff[20];   // Receive buffer
char Tx_buff[11];   // Transmit buffer
char cmd[3];        // Command extracted fron received string
int buff_index = 0; // counter for the position in buffer
char in_char;
int cmd_len;        // length of received string
String cmd_str = "";
String reply_str = "";
int index = 0;        // general counter var
uint32_t freq_t;      // temporary frequency
char* freq_s[10];     // Frequency in sting format - one digit in each char

//**********
void CAT_check()  // checks command from Serial Port and executes it
{
  buff_index = 0;   // no of characters in Receive buffer

  while (Serial.available() > 0)
  {
    // Receive serial characters till a ";" (eoc) store in Rx_buff, Max 20 char
    cmd_len = Serial.readBytesUntil(';', Rx_buff, 20);
    inCatCheck = 1;   // we are still awaiting more chars
    decode_command(); //
  }
  inCatCheck = 0;  // cmd received and decoded
  Serial.flush();  // clear up any extra chars
}

// - - - - - - - - - - - - - - - -  - - 
void decode_command()   // Understand command and execute its related function
{
  // separate first two chars as commands and comapre them with the commands in list (Not all CAT commands are implemented)
  cmd[0] = Rx_buff[0]; cmd[1] = Rx_buff[1]; cmd[2] = '\0';
  char* cmd_list[] = {"AI", "AB", "FA", "FB", "ID", \
                      "IF", "MD", "TX", "SV", "VS", \
                      "SH", "FT", "NA", "PC"
                     };
  // These are most needed commands for normal digital communication
  // more CAT commands may be added and their corresponding functions written below

  for (index = 0; index <= sizeof(cmd_list) / sizeof(cmd_list[0]); index++)
  {
    if (strcmp(cmd, cmd_list[index]) == 0)  // search command from above list
      break;
  }

  if (index >= sizeof(cmd_list) / sizeof(cmd_list[0]) )// no listed cmd found
    return;

  switch (index)
  {
    case 0: cat_ai(); break;       // Auto Information
    case 1: cat_a2b_vfo(); break;  // Copy A to B Vfo
    case 2: cat_freqA(); break;    // get or set freq for VFO A
    case 3: cat_freqB(); break;    // get or set freq for VFO B
    case 4: cat_send_id(); break;  // Send Radio's ID
    case 5: cat_send_info(); break;// Lot of info
    case 6: cat_mode(); break;     // Mode (LSB/USB etc)
    case 7: cat_tx_state(); break; // Tx state
    case 8: cat_swap_vfo(); break; // swap
    case 9: cat_sel_vfo(); break;  // VS VFO Select
    case 10: cat_send_sh(); break; //
    case 11: cat_fn_tx(); break;   //
    case 12: cat_set_narrow(); break;// narrow filter
    case 13: cat_pwr_level(); break; // rig power level
  } //  To Add AM, FT, MA, MC, QS, VM, BD, BS, BU, CH, FT, MC, MR, MW, SM

  //************* Functions to execut the command received
  bool ai = 0; // auto info
  void cat_ai()   // Auto reply
  {
    if (cmd_len == 3)    // cmd recvd 'AI0;' to set AI
      ai = (Rx_buff[2]);
    else                  // asking for AI value send "AI0;"
    {
      sprintf(Tx_buff, "AI%1d;", ai);
      Serial.print(Tx_buff);
    }
  }

  //*************
  void cat_a2b_vfo()  // AB cmd = copy vfo A to vfo B
  {
    vfo_B = vfo_A;
    sb_B = sb_A;
    vfo_selA();    // keep vfo A?
    return;
  }

  //*************
  void cat_freqA()   // FA = get (FAxxxxxxxx;) ot send (only FA;) current VFO A freq
  {
    freq_t = 0;
    if (cmd_len > 3) // get new and set new freq
    {
      get_adj_freq();
    }
    else   // return curent vfoA freq if only FA cmd
    {
      Serial.print("FA");
      if (vfo_A < 10000000)
        Serial.print('0');   // pre pad if less than 8 char
      if (vfo_A < 1000000)
        Serial.print('0');   // pre pad if less than 8 char
      if (vfo_A < 100000)
        Serial.print('0');   // pre pad if less than 8 char

      Serial.print(vfo_A);
      Serial.print(";");
    }
  }

//- - - - - - - - -  - - - -- - - - - - -
  void cat_freqB()   //  FB = get & set new freq (FBxxxxxxxx;) ot send (only FB;) current VFO A freq
  {
    if (cmd_len > 3) // get new freq
    {
      get_adj_freq();
    }
    else   // return curent vfoB freq
    {
      Serial.print("FB");
      if (vfo_B < 10000000)
        Serial.print('0');   // pre pad if less than 8 char
      if (vfo_B < 1000000)
        Serial.print('0');   // pre pad if less than 8 char
      if (vfo_B < 100000)
        Serial.print('0');   // pre pad if less than 8 char

      Serial.print(vfo_B);
      Serial.print(";");
    }
  }


//- -- - - - - - -  - - - - - - -- - - - - - - - - -
  void get_adj_freq()   // common fns used by both freq set commands FA & FB, Read & set freq
  {
    freq_t = 0;
    for (i = 2; i <= 8; i++)    // convert char array to number (vfo)
      freq_t = (freq_t + Rx_buff[i] - '0') * 10; // Assemble all ascii bytes into freq
    if (isdigit(Rx_buff[9]))   // if only 7 digits sent  like in wsjtx
      freq_t = freq_t + (Rx_buff[9] - '0'); // units
    else
      freq_t = freq_t / 10;
    // check if this new freq is in current band limits
    if (freq_t >= F_MIN_T[bnd_count] && freq_t <= F_MAX_T[bnd_count])   // follow band limits
    {
      vfo = freq_t;   // rig control functions below
      save_frequency();
      set_vfo();
      changed_f = 1;
      return;
    }
    else    // if different band
    {
      vfo = freq_t;   // rig control functions below
      save_frequency();
      set_band();
      display_band();
      set_vfo();
      changed_f = 1;
    }
  }

  //*****************
  void cat_send_id()
  {
    Serial.print(Radio_id);  // fixed ID for different models of Yaesu - select model in userdefs.h - using new CAT -
  }

  //********************
  // Rig control  Arduino progam sets hardware vfo frequency (long vfo)
  void cat_send_info()   // Fixed 27 byte packet : see Yaesu CAT manual
  {
    Serial.print("IF");
    Serial.print("000"); // P1  = Mem Ch no  (we may use our ch no) TBD
    if (vfo < 10000000)
      Serial.print('0');   // pre pad if vfo less than 8 char
    if (vfo < 1000000)
      Serial.print('0');   // pre pad if vfo less than 8 char
    if (vfo < 100000)
      Serial.print('0');   // pre pad if vfo less than 8 char
    Serial.print(vfo);   // P2 - VFOA - 8 char/bytes
    Serial.print("+000000"); //P3 ,P4, P5 (rig dependnt)
    Serial.print(sb_A == LSB ? 1 : 2);   //  P6 = Mode
    Serial.print("00000;"); //P7-10  (rig dependent)
    return;
  }

  //*****************
  void cat_mode()   //MD cmd M D P1 P2 , P1=0 -VFOA, P1=1 -VFOB
  {
    if (cmd_len > 3) // set MD cmd
    {
      if (Rx_buff[2] == '0') // Adj VFO A mode
      {
        sb_A = (Rx_buff[3] == '1' ? LSB : USB); // received mode
        sideband = sb_A;     // change in rig control program
      }
      else    // Adj VFO B when Rx_buff[2]=1
      {
        sb_B = (Rx_buff[3] == '1' ? LSB : USB);
        sideband = sb_B;    // change in rig control program
      }

      //rig control program functions/variables below
      bfo1 = (sideband == USB ? bfo1_USB : bfo1_LSB);
      display_sideband();
      set_bfo1();
      save_frequency();
      changed_f = 1;
    }
    else   // send mode info cmd != 4 char (only MD P1 ;) 3 char
    {
      Serial.print("MD");
      if (Rx_buff[2] == '0') // report VFO A
      {
        Serial.print('0');
        Serial.print(sb_A == LSB ? 1 : 2); // 1= LSB, 2 = USB from Rig control program
      }
      else       // vfo B data
      {
        Serial.print('1');
        Serial.print(sb_B == LSB ? 1 : 2); // from Rig control program
      }
      Serial.print(';');
    }
    return;
  }

  //************************
  void cat_swap_vfo()    // SV; commands Only sets (no read)
  {
    long temp;   // temp vfo
    int tsb;   // temp sideband
    temp = vfo_A; // all below for Rig control program
    tsb = sb_A;
    vfo_A = vfo_B;
    sb_A = sb_B;
    vfo_B = temp;
    sb_B = tsb;

    save_frequency();
    set_vfo();
    set_bfo1();
    changed_f = 1;

    return;
  }

  void cat_sel_vfo()   // VS P1; = VFO select - P1 = 0 - vfoA, 1 - vfoB
  {
    if (cmd_len > 2)  // cmd is 4 char VS P1 ;  - set VFO A or B
    { if (Rx_buff[2] == '0')   // P1 = 0 is VFO A, 1 is B
        vfo_selA();  // functions  in Rig control program
      else
        vfo_selB();
    }
    else    // it is query only recd VS; cmd
    {
      Serial.print("VS");
      Serial.print(vfo_A_sel ? 0 : 1);  // flag/var in rig control
      Serial.print(";");  // A =0, B = 1
    }
    return;
  }

  //**********************
  void cat_tx_state()  // TX set
  {
    if (cmd_len > 2)   // TX P1 ;  set Tx
    {
      if (Rx_buff[2] == '0')  // || Rx_buff[2] != 2 )   // P1=1 On Tx, 0 = Off from CAT cmd
        ptt_OFF();  // rig control function
      else
        ptt_ON();   // rig control functions

      disp_VFO_button(); // rig control functions
    }
    else      // cmd TX ;
    {
      Serial.print("TX");
      Serial.print(txstatus ? 1 : 0); // 2 = cat TX off  but problem so 0
      Serial.print(";");
    }
  }

  //**************************
  int SHA = 16;   // filter width 16=center, 00 = CCW, 31 = CW
  void cat_send_sh()    // SH = width (filter?)
  {
    if (cmd_len > 3)  // SH P1 P2 P2
    {
      if (Rx_buff[3] == '1')
        SHA = 10;
      else
        SHA = 0;
      if (Rx_buff[4] == '1')
        SHA = SHA + 1;
      else
        SHA = SHA + 0;
    }
    else if (cmd_len = 3)       // SH P1 ; cmd
    { Serial.print("SH0");
      Serial.print(SHA);
      Serial.print(";");
      return;
    }
    else     // need to set SHA
    {
      if (Rx_buff[2] == '0') // normally fixed 0
        SHA = (Rx_buff[3] - '0') * 10 + (Rx_buff[4] - '0'); // convt to int
    }
  }

  //***********************
  void cat_fn_tx()  // FT cmd - select VFO 
  {
    if (cmd_len == 3) // FT P1 ; = cmd to set Tx as P1
    {
      if (Rx_buff[2] == '0')
        vfo_selA(); // rig control functions
      else
        vfo_selB(); //// rig control functions
    }

    else  if (vfo_A_sel) // if FT enquiry -  FT; // rig control variable
      Serial.print("FT0;");
    else
      Serial.print("FT1;");
    return;
  }

  //***************************
  int NA_T;   // NA command
  void cat_set_narrow()
  {
    if (cmd_len > 3) // NA P1 P2 ;  P1=0/1 =VFOA/B, P2 = 0/1=Off/ON
    {
      if (Rx_buff[2] == '0')  // VFOA
        NA_T = 0;  // P1=0 VFOA
      else
        NA_T = 10;// P1=1 VFOB

      if (Rx_buff[3] == '0')  // 0=Off. 1 = On
        NA_T = 0;  // P1=0 P2=0
      else
        NA_T = 11;// P1=1 P2=1
    }
    else  // cmd is NA P1; query
    {
      Serial.print("NA");
      Serial.print(NA_T);
      Serial.print(";");
    }
  }

  //**************
  void cat_pwr_level()   // rf power level not measured in uBitx currently so dummy
  {
    Serial.print("PC250;");
  }

  //%%%%%%%%%%%%%%%%%%%
