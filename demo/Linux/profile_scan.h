

typedef struct PROFILE_INFO {
  char usr_name[32];
  unsigned int pid;
  float ave_cpu_percent;
  float ave_mem_percent;
  unsigned long long eAveTime;
} PROFILE_INFO;

void get_profile_info (const char *exec, PROFILE_INFO *info);
