#include "udf.h" 

#include "dpm.h" 

DEFINE_DPM_DRAG(Brownlawler,Re,tp) 

{real D, void_s,void_g,cd,beta; 

cell_t cell; Thread *thread_s, *mix_thread;  

cell=P_CELL(tp); 
 
mix_thread=P_CELL_THREAD(tp);  

thread_s = THREAD_SUB_THREAD(mix_thread,1);

void_s = C_VOF(cell, thread_s);  void_g =1-void_s; 

if(Re==0.0) cd=0;
else cd = 24/(void_g*Re)*(1+0.15*pow(void_g*Re,0.681))+0.407/(1+8710/(void_g*Re));

beta = 3.7 - 0.65*exp(-0.5*pow(1.5-log10(void_g*Re),2));
 
D = 0.75 * cd * Re * void_g * void_g * pow(void_g,-beta) ;

return (D); }