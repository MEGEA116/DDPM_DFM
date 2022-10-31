#include "udf.h" 

#include "dpm.h" 

DEFINE_DPM_DRAG(difelice,Re,tp) 

{real D, void_s,void_g,cd,beta; 

cell_t cell; Thread *thread_s, *mix_thread;  

cell=P_CELL(tp); 
 
mix_thread=P_CELL_THREAD(tp);  

thread_s = THREAD_SUB_THREAD(mix_thread,1);

void_s = C_VOF(cell, thread_s);  void_g =1-void_s; 

if(Re==0.0) cd=0;
else cd = pow(0.63+4.8*sqrt(1/(void_g*Re)),2);

beta = 3.7 - 0.65*exp(-0.5*pow(1.5-log10(void_g*Re),2));
 
D = 0.75 * cd * Re * void_g * void_g * pow(void_g,-beta) ;

return (D); }