/**************************************/
/* This file generated automatically. */
/* Do not modify.                     */
/**************************************/

#include "udf.h"
#include "prop.h"
#include "dpm.h"
#include "version.h"

extern DEFINE_DPM_DRAG(drag_ganser, Re, p);
extern DEFINE_DPM_DRAG(drag_template, Re, p);
extern DEFINE_EXECUTE_ON_LOADING(bind_edem_subrs, libname);
extern DEFINE_ON_DEMAND(connect_edem_coupling);
extern DEFINE_ON_DEMAND(get_solution_from_edem);
extern DEFINE_ON_DEMAND(update_edem_solution);
extern DEFINE_ON_DEMAND(synchronize_fluent_to_edem_time);
extern DEFINE_ON_DEMAND(disconnect_edem_coupling);
extern DEFINE_ADJUST(adjust_edem_solution, d);
extern DEFINE_EXECUTE_AT_END(update_edem_at_end);
extern DEFINE_EXECUTE_AT_EXIT(disconnect_edem_coupling_at_exit);

__declspec(dllexport) UDF_Data udf_data[] = {
  {"drag_ganser", (void (*)(void))drag_ganser, UDF_TYPE_DPM_DRAG},
  {"drag_template", (void (*)(void))drag_template, UDF_TYPE_DPM_DRAG},
  {"bind_edem_subrs", (void (*)(void))bind_edem_subrs, UDF_TYPE_EXECUTE_ON_LOADING},
  {"connect_edem_coupling", (void (*)(void))connect_edem_coupling, UDF_TYPE_ON_DEMAND},
  {"get_solution_from_edem", (void (*)(void))get_solution_from_edem, UDF_TYPE_ON_DEMAND},
  {"update_edem_solution", (void (*)(void))update_edem_solution, UDF_TYPE_ON_DEMAND},
  {"synchronize_fluent_to_edem_time", (void (*)(void))synchronize_fluent_to_edem_time, UDF_TYPE_ON_DEMAND},
  {"disconnect_edem_coupling", (void (*)(void))disconnect_edem_coupling, UDF_TYPE_ON_DEMAND},
  {"adjust_edem_solution", (void (*)(void))adjust_edem_solution, UDF_TYPE_ADJUST},
  {"update_edem_at_end", (void (*)(void))update_edem_at_end, UDF_TYPE_EXECUTE_AT_END},
  {"disconnect_edem_coupling_at_exit", (void (*)(void))disconnect_edem_coupling_at_exit, UDF_TYPE_EXECUTE_AT_EXIT}
};

__declspec(dllexport) int n_udf_data = 11;

__declspec(dllexport) void UDF_Inquire_Release(int *major, int *minor, int *revision)
{ 
  *major = RampantReleaseMajor;
  *minor = RampantReleaseMinor;
  *revision = RampantReleaseRevision;
}
