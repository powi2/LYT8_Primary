//==============================================================================
// ILIM_Pre.cpp (User function)
// 
//    void ILIM_Pre_user_init(test_function& func)
//    void ILIM_Pre(test_function& func)
//
//==============================================================================

#include "asl.h"
// Disable warning C4244 "conversion from 'const double' to 'float', possible loss of data"
#pragma warning (disable : 4244)
#pragma warning (disable : 4305)
#include "ILIM_Pre.h"

// !!!! User #includes and externs can be placed between the comments
// !!!!

// *************************************************************************

void ILIM_Pre_user_init(test_function& func)
{
	ILIM_Pre_params *ours;
    ours = (ILIM_Pre_params *)func.params;

// !!!! User initialization code below this comment (do not remove comment)

// !!!! User initialization code above this comment (do not remove comment)
}

// *************************************************************************

void ILIM_Pre(test_function& func)
{
    // The two lines below must be the first two in the function.
    ILIM_Pre_params *ours;
    ours = (ILIM_Pre_params *)func.params;

	// Increment function number //
	gFuncNum++;	

	// Datalog gFuncNum variable //
	if(gDisplay_FuncNum)
		PiDatalog(func, A_fnum_ILIM_Tr_P, gFuncNum, 9, POWER_UNIT);

	//Skip Test if AbortTest set 
	if (AbortTest)
		return;

	// Skip trimming if g_Trim_Enable_P set //
	//if (g_Trim_Enable_P == 0)	return;

	//if (g_Fn_ILIM_Pre == 0 )  return;

	// Test Time Begin //
	 if (g_TstTime_Enble_P)
	 	g_begintime = g_mytimer.GetElapsedTime();

	// Test Names //
	float ILIM_pt_P = 0;
	float ILIM_pt_didt_P = 0;
	float ILIM_pt_ton_P = 0;
	float ILIM_Pre_TT = 0;

	// Declare Variables //
	//int NObin[1] = {0}; // Place holder //
	//int NOhex = 0; // Place holder //
	float Dummy_rdson_peak = 0.0;
	float Dummy_rdson_spec;
	float PV3_Charge =0.0;
	float Vped_meas  =0.0;
	float Vpv3_meas  =0.0;
	int lcnt=0;
	float ChB_vrng=0.0;
	int vrng_b	=0.0;
	Pulse pulse; // External trigger pulse from dvi for debug //

	//Decide what Inductor pullup voltage to use per FET SIZE
	gVind_RM = 42.5;	//size x8

	PV3_CHARGE_ON(RANGE_100_V);	//Prepare PV3 before power-up
	PV3_Check_Charge(RANGE_100_V);
	pv3_4->charge_off();

	g_SAMPLE_SIZE = GAGE_POST_32K;
	BINNO_Gage_ChanAB_setup(0.15);	// Only do it if g_SAMPLE_SIZE is diff from previous. (Each input/trig control function takes 4ms)
	//vrng_b = 0;
	//ChB_vrng = 0;
	
	Setup_Resources_for_I2C_P();
	PowerUp_I2C_P();

	DSM_I2C_Write('b', g_TM_CTRL, 0x06);		//0x40, 0x06 (enable analog mode + core_en)
	DSM_I2C_Write('w', g_ANA_CTRL_1, 0x2804);	//0x46, 0x2804
	DSM_I2C_Write('w', 0x44, 0x0402);			//choose min ON time and allow flux link thru TSPIN + disable receiver output
	DSM_I2C_Write('b', 0x4C, 0x01);				//release TS pin

	DSM_set_I2C_clock_freq(DSM_CONTEXT, 300);	//Disable DSM I2C

	//--------------------------------------------------------------------------------------------
	//Setup for TS to run 100kHz
		//Disconnect DSM from Primary after releasing VPIN or TS pins
		Open_relay(K1_DSM_TB);	
		Open_relay(K3_DSM_TB);	
		delay(1);


	TS_ovi3->set_voltage(TSovi3_ch, 0.0, VOLT_10_RANGE); // OVI_3_0
	wait.delay_10_us(10);

	Close_relay(K2_TS_TB); //TS disconnect from OVI_3_0_TS_RB
	Close_relay(K3_TS_IB); //DDD7_1 to Comparator LT1719 to COMP_OUT to TS
	delay(4);

	TS_ovi->set_voltage(TSovi1_ch, 1.0, VOLT_1_RANGE); // OVI_1_5 for Comparator LT1719 Vref input
	wait.delay_10_us(10);

	//BPP_zigzag(5.5, 4.3, 5.4);

	if(0)	//Observe from RL pullup
	{
		//Drain setup to connect to Ridder board with RL load and set Drain to 5V and ready for vMeas mode
		Close_relay(K2_D_RB);
		delay(4);
			D_dvi->set_current(D_ch, 100e-3, RANGE_300_MA); 
			delay(1);
			D_dvi->set_voltage(D_ch, 45.0, VOLT_50_RANGE); // DVI_11_0
			delay(1);
	}
	else	//Observe from Inductor pullup
	{
		//Connect Drain to IM with PV3
		Close_relay(K3_D_TB);	// disconnect	Drain from RL pullup 
		Close_relay(K9_D_TB);	// Connect		Drain to CT- to Pearson to CT+ & CT2+
				//Close_relay(K1_D_RB);	// Connect		Iped to CT2+ to CT2-
		Close_relay(K1_IM_TB);	// Connect		Vind to CT2+ to CT2-
		delay(5);
	//pulse.do_pulse();	
	//pv3_4->charge_off();
	//delay(15);
		PV3_Connect_Drain_and_DriveON(gVind_RM); // Drive Voltage
		//delay(15);
	}

	BPP_zigzag(5.5, 4.3, 5.4);

	Load_100Khz_Pulses_TS();
	delay(1);

	Gage_Start_Capture(  );

pulse.do_pulse();	
	Run_100Khz_Pulses_TS();	//DE want 50ns high duty cycle ideally.
	delay(1);

	Gage_Wait_For_Capture_Complete();

	pv3_4->drive_off();	

	// Powerdown //
	Stop_100Khz_Pulses_TS();

DEBUG=1;
g_Debug=1;
	g_WAVE_NAME =  "ILIM_Pre";
	g_FIND_RDSON_Peak_FLAG = 0;
	Gage_Find_Ilim(&ILIM_pt_P, &ILIM_pt_didt_P, &ILIM_pt_ton_P, &Dummy_rdson_peak, &Dummy_rdson_spec);
DEBUG=0;
g_Debug=0;

	PiDatalog(func, A_ILIM_pt_P,		ILIM_pt_P,		ours->fail_bin, POWER_MILLI);
	PiDatalog(func, A_ILIM_pt_didt_P,	ILIM_pt_didt_P, ours->fail_bin, POWER_MILLI);
	PiDatalog(func, A_ILIM_pt_ton_P,	ILIM_pt_ton_P,	ours->fail_bin, POWER_MICRO);
	//PiDatalog(func, A_ILIM_I2f_pt_P,	ILIM_I2f_pt_P,	ours->fail_bin, POWER_UNIT);
	//PiDatalog(func, A_ILIM_Pre_TT,		ILIM_Pre_TT,	ours->fail_bin, POWER_MILLI);

	pv3_4->set_current(0.1e-9);						
	pv3_4->set_voltage(0, RANGE_100_V);
	wait.delay_10_us(10);
	pv3_4->open_switch(PV3_LOW_FORCE_2);
	pv3_4->open_switch(PV3_LOW_SENSE_2);
	pv3_4->open_switch(PV3_HIGH_FORCE_1);
	pv3_4->open_switch(PV3_HIGH_SENSE_1);
	delay(1);
	pv3_4->charge_on();	

	Power_Down_I2C_P();
	Open_relay(K2_D_RB);				//D			to RB_82uH_50ohm to K2_D to DVI-11-0		Disconnect
	Open_relay(K2_TS_TB);				//TS		to OVI_3_0_TS_RB							Connect
	Open_relay(K3_TS_IB);				//DDD7_1	to Comparator LT1719 to COMP_OUT to TS		Disconnect

	//disconnect Drain from IM
	Open_relay(K3_D_TB);	// Connect			Drain to RL pullup 
	Open_relay(K9_D_TB);	// disConnect		Drain to CT2-
	//Open_relay(K1_D_RB);	// disConnect		Iped to CT2+ to CT2-
	Open_relay(K1_IM_TB);	// disConnect		Vind to CT2+ to CT2-
	delay(1);


	/*

	

	if (!strnicmp(Part_ID_LimitSet, "INN3366", 7) || !strnicmp(Part_ID_LimitSet, "INN3376", 7))
	{
		Close_relay(KIM3); // ILIM Pedestal Use resistor Rpd2
	}
	else
		Open_relay(KIM3); // ILIM Pedestal Use resistor Rpd1

	wait.delay_10_us(300);

	// Setup Pedestal
	D_dvi->set_current(D_ch, 300e-3, RANGE_300_MA);
	D_dvi->set_voltage(D_ch, g_Vpedestal, VOLT_50_RANGE); // DVI_11_0
	D_dvi->set_meas_mode(D_ch, DVI_MEASURE_VOLTAGE);
	wait.delay_10_us(50);

	PV3_Connect_Drain_and_DriveON(gVind_RM); // Drive Voltage

	// Two pulses 30usec apart to give control to secondary. //
	ddd_7->ddd_run_pattern(Vpin_30usec_start,Vpin_30usec_stop);	
	wait.delay_10_us(10);

	// Enable RTM by switch >110kHz for >200usec. //
	ddd_7->ddd_run_pattern(Vpin_125kHz_pulse_start_X,Vpin_125kHz_pulse_stop_X);	
	wait.delay_10_us(100);

	// VBP_P drops when secondary has control //
	BPP_ovi->set_voltage(BPP_ch, g_VBPP_P_s_final+0.1, RANGE_10_V); // prevent undershoot //
	wait.delay_10_us(8);
	BPP_ovi->set_voltage(BPP_ch, g_VBPP_P_s_final, RANGE_10_V);
	wait.delay_10_us(20);

	// Start capturing data //
	Gage_Start_Capture();

	// Setup to have ILIM switch for 18pulses, then OFF 30us, ON 10us, OFF 200us and ON for 3 pulses of 100kHz
	ddd_7->ddd_run_pattern(Vpin_100kHz_ILIM_pulse_start,Vpin_100kHz_ILIM_pulse_stop);
	wait.delay_10_us(100);	

	// Finish capturing data //
	Gage_Wait_For_Capture_Complete();
	pv3_4->drive_off();	

	// Process data
	WAVE_NAME =  "ILIM_Pre";
	FIND_RDSON_Peak_FLAG = 0;
	Gage_Find_Ilim(&ILIM_pt_P, &ILIM_pt_didt_P, &ILIM_pt_ton_P, &Dummy_rdson_peak, &Dummy_rdson_spec);
	g_ILIM_I2f_pt_S = ILIM_pt_P *ILIM_pt_P * 100e3 * 1e-3; // Pass to global variable
	g_ILIM_pt_P = ILIM_pt_P;										 // Pass to global variable
	if(ILIM_pt_P > 99)
		g_ILIM_I2f_pt_S = 999;

	// Powerdown //
	pv3_4->set_current(0.1e-9);						
	pv3_4->set_voltage(0, RANGE_100_V);
	wait.delay_10_us(10);
	pv3_4->open_switch(PV3_LOW_FORCE_2);
	pv3_4->open_switch(PV3_LOW_SENSE_2);
	pv3_4->open_switch(PV3_HIGH_FORCE_1);
	pv3_4->open_switch(PV3_HIGH_SENSE_1);
	D_dvi->set_voltage(D_ch, 0.0, VOLT_50_RANGE); // DVI_11_0
	wait.delay_10_us(10);
	Open_relay(K12);
	Open_relay(KIM1);
	Open_relay(K9); // Disconnect CT2 from Drain
	Open_relay(KIM3); // Disconnect Pedestal circuit
	Open_relay(K1); // Disconnect Pedestal circuit
	BPP_ovi->set_current(BPP_ch, 30e-3, RANGE_30_MA);
	VBPP_ramp_down(3, 0.0, 0.05);	
	wait.delay_10_us(10);
	VPIN_ovi->connect(VPIN_ch);

	// DDD level //
	wait.delay_10_us(200);
	ddd_7->ddd_run_pattern(Low_pat_start, Low_pat_stop);	

	// Datalog
	PiDatalog(func, A_ILIM_pt_P, ILIM_pt_P, 9, POWER_MILLI);
	PiDatalog(func, A_ILIM_pt_didt_P, ILIM_pt_didt_P, 9, POWER_MILLI);
	PiDatalog(func, A_ILIM_pt_ton_P, ILIM_pt_ton_P, 9, POWER_MICRO);
	PiDatalog(func, A_ILIM_I2f_pt_P, g_ILIM_I2f_pt_S, 9, POWER_UNIT);

	// Test Time End //
	if (g_TstTime_Enble_P)
	{
		g_endtime = g_mytimer.GetElapsedTime();
		ILIM_Pre_TT = (g_endtime - g_begintime)*1e-6;
		PiDatalog(func, A_ILIM_Pre_TT, ILIM_Pre_TT, 9, POWER_MILLI);
	}

	// Check any test failed //
	if (PiGetAnyFailStatus())		
	{
		g_PartFailed = 1;
	}
*/
}
