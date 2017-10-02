
struct TOP_INFO {
	int counter;
	char ip[12];
	int luz;
	int luzPercent;
} TOP_INFO;


void top_init (void);
struct TOP_INFO* getTopInfo();
void top_update_values (int counter, char ip[12], int luz, int luzPercent);
