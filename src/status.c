static int subscribed = 0;
static int finished = 0;
static int disc_finished = 0;

int get_sub() { return subscribed; }
int get_fin() { return finished; }
int get_dc() { return disc_finished; }

void set_sub() { subscribed = 1; }
void set_fin() { finished = 1; }
void set_dc() { disc_finished = 1; }
