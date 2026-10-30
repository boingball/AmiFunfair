#include "detail.h"
const FairDetail fair_modes[DETAIL_COUNT]={
    {320,200,40,8000,40000,"NORMAL"},
    {640,200,80,16000,80000,"HIRES"},
    {640,400,80,32000,160000,"HI-LACE"}
};
int fair_detail=DETAIL_NORMAL,fair_requested_detail=DETAIL_NORMAL;
int fair_hardware_pointer;
unsigned fair_detail_mask=(1u<<DETAIL_COUNT)-1;
int fair_request_detail(int d)
{
    if(d<0||d>=DETAIL_COUNT||!(fair_detail_mask&(1u<<d)))return 0;
    fair_requested_detail=d;return 1;
}
int fair_cycle_detail(void)
{
    int i;
    for(i=1;i<=DETAIL_COUNT;i++){
        int d=(fair_requested_detail+i)%DETAIL_COUNT;
        if(fair_request_detail(d))return d;
    }
    return fair_detail;
}
