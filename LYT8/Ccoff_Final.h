//==============================================================================
// Ccoff_Final.h 
//==============================================================================

#include "asl.h"
#include "function.h"

void Ccoff_Final_user_init(test_function& func);

// ***********************************************************

void initialize_Ccoff_Final(short test_entry,test_function& func,char **function_name)
{
	func.dlog->set_datalog_inactive();

	Ccoff_Final_user_init(func);

	func.dlog->test_offset = test_entry + 1;
}

// ***********************************************************

void setup_Ccoff_Final(test_function &func,char **function_name,char *heading_line,unsigned short *want_print,char manual_setup)
{
}

// ***********************************************************
enum CCoff_Pt{
	A_fNum_Ccoff_Fin,
	A_CCoff_final,
	A_CCoff_Act_Chg,
	A_CCoff_final_TT
};
