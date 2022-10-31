#include "udf.h"
#include "prop.h"
#include "dpm.h"
extern DEFINE_DPM_DRAG(Brownlawler,Re,tp);
__declspec(dllexport) UDF_Data udf_data[] = {
{"Brownlawler", (void(*)(void))Brownlawler, UDF_TYPE_DPM_DRAG},
}; 
__declspec(dllexport) int n_udf_data = sizeof(udf_data)/sizeof(UDF_Data);
#include "version.h"
__declspec(dllexport) void UDF_Inquire_Release(int *major, int *minor, int *revision)
{
*major = RampantReleaseMajor;
*minor = RampantReleaseMinor;
*revision = RampantReleaseRevision;
}
