#include "top.h"

struct TOP_INFO* top_info;


// initial values
void top_init (void) {
	top_info = malloc(sizeof(TOP_INFO));
}

// update values..
void top_update_values (int counter, char ip[12], int luz, int luzPercent) {
	top_info->counter = counter;
	memcpy(top_info->ip , ip,sizeof(ip));
	top_info->luz = luz;
	top_info->luzPercent = luzPercent;

}

struct TOP_INFO* getTopInfo(){
	return top_info;
}
